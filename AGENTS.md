# AGENTS.md

## Project

`gmk67sts` — C11 CLI for time sync and lighting control on the GMK67 mechanical
keyboard, talking HID via hidapi. Target device: VID `0x320f`, PID `0x5055`,
HID interface `3` (config interface; falls back to any matching device).

## Build & run

- `make` — builds `bin/gmk67sts` (gcc `-Wall -Wextra -pedantic -std=c11 -O2`).
  Hidapi is auto-detected via pkg-config (`hidapi-hidraw` → `hidapi-libusb` →
  bare `hidapi`); the build fails early in the `dirs` step if none are found.
- `make debug` — `-g -DDEBUG` build; runs `clean all` first.
- `make run` — builds, then runs `--sync` once.
- `make install` → `/usr/local/bin/gmk67sts`. `make install-service` also
  installs `deploy/gmk67sts.service` and runs `systemctl daemon-reload`.
  Also: `make uninstall-service`, `make clean`.
- `build/` and `bin/` are gitignored build outputs — safe to delete.

Hardware access needs either `sudo` or a udev rule granting the `plugdev`
group access to `/dev/hidraw*`:

```
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0660", GROUP="plugdev"
```

Install it at `/etc/udev/rules.d/50-gmk67.rules`, then
`udevadm control --reload-rules && udevadm trigger` and replug.

## Tests

There are **no automated tests**. The "Testing Checklist" in
`docs/ARCHITECTURE.md` is a manual smoke test that needs a physical GMK67
connected. `--dump` is the safe read-only way to exercise the read path.

## Protocol quirks (critical)

- **READ_DATA (cmd `0x05`) MUST send a 4-byte zero payload**, not NULL/0
  length. Sending NULL made the device return bad data; writing that back
  wiped all user settings (the v1.2.0 bug — see `docs/BUGFIX.md`). This is
  non-standard HID behavior; follow it exactly in `src/core/config.c`.
- Read sequence: `INIT → PREP_READ ×9 (pos 0..32, 4 bytes) + PREP_READ ×1
  (pos 36, 1 byte) → COMMIT → READ_DATA ×12 (pos 0..44, 4 bytes each)`.
  Write sequence: `INIT → WRITE_CONFIG (cmd 0x06, full 48 bytes) → COMMIT`.
- Every `config_read()` result is passed through `config_validate()`
  (rejects all-zeros, invalid BCD time, <5 non-zero bytes). Do not bypass.

## Config buffer (48 bytes) — read-modify-write is mandatory

Time/profile writes must **read current config, modify only the target bytes,
write the full buffer back**. Never write a partial or freshly-zeroed buffer —
it wipes unrelated settings. On write failure `profiles.c` attempts
`rollback_write()` with the original buffer; if that also fails the user must
replug and run `--restore-defaults`.

Layout:

- `[1-8]` underglow (effect, brightness, speed, orient, rainbow, R, G, B)
- `[28-32]` LED (mode, saturation, byte30, rainbow, color)
- `[33]` showImage, `[34]` image1Frames, `[46]` image2Frames
- `[35-41]` time, BCD-encoded (sec, min, hour, dow, date, month, year)
- `[43-44]` frame duration, little-endian (ms)

Factory defaults are a hardcoded 48-byte `FACTORY_CONFIG` in
`src/core/profiles.h`. `--restore-defaults` writes it and overlays current
time. To refresh it: `./bin/gmk67sts --dump` prints a paste-ready C array.

## Architecture

Layered, all under `src/`:

- `cli/main.c` — `getopt_long` parsing. Enforces **exactly one action**
  (sync / daemon / restore / load-profile / list-profiles / dump); any other
  count is a hard error.
- `core/timesync.c` — sync orchestration, owns device lifecycle.
- `core/config.c` — read/write/validate/update-time of the 48-byte config.
- `core/profiles.c` — profile table, `--restore-defaults`, `--load-profile`.
- `core/daemon.c` — `--daemon` loop (interval 60..604800s, default 43200).
- `core/device.c` — hidapi init/enumerate/open/close. `device_find_path()`
  returns a `strdup`'d path the caller must `free()` — the only dynamic
  allocation in core.
- `core/protocol.c` — 64-byte HID report (report ID `0x04`, checksum over
  bytes 3-63), send + ACK loop (30s total timeout).
- `include/gmk67sts.h` — public umbrella header. Note: its
  `GMK67STS_VERSION` is stale (`"1.2.0"`); the real version is `VERSION` in
  `main.c` (currently `"1.5.1"`).

Resource cleanup uses `goto cleanup`; errors go to stderr with context.

## Recovery

If lighting gets wiped: unplug, wait 10s, replug, then
`./bin/gmk67sts --restore-defaults`. Full procedure in `docs/EMERGENCY_FIX.md`.
The broken `--reset` command was removed — do not reintroduce it.
