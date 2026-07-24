# Changelog

All notable changes to gmk67sts are documented in this file.

## [1.5.1] - 2026-07-23

### Documentation
- Documented the required udev rule inline (`KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0660", GROUP="plugdev"`) in README and INSTALL instead of referencing an external rules file
- Removed all references to the sibling `gmk87-node` repo, `reference.py`, and `src/lib/device.js`
- Fixed "GMK87" → "GMK67" in protocol notes
- Synced the stale `GMK67STS_VERSION` in `include/gmk67sts.h` with the real `VERSION` in `src/cli/main.c`

---

## [1.2.0] - 2026-07-23

### Bug Fixes
- **FIXED: Configuration read failure causing settings wipe** - The READ_DATA command (0x05) now correctly sends 4 bytes of dummy data as required by the keyboard protocol. Previously, sending NULL with 0 length caused the device to return incorrect data, resulting in all user settings (LED effects, underglow, winlock, etc.) being reset to zero.

### Improvements
- Added `config_validate()` function to detect corrupted config reads
- Config validation checks for excessive zero bytes (>80%)
- Config validation verifies frame count bytes are not both zero
- Automatic validation after every config read operation
- Hex dump printed if validation fails (debug mode)

### Technical Details
- The GMK67 keyboard's READ_DATA command requires a 4-byte dummy payload even though the data is unused
- Without the dummy payload, bytes 37-47 (LED/underglow settings) were read as zeros

### Files Changed
- `src/core/config.c` - Fixed READ_DATA payload, added validation
- `src/core/config.h` - Added `config_validate()` declaration

---

## [1.1.0] - 2026-07-23

### New Features
- **Daemon mode** - Run as a long-running service with periodic time sync
- **systemd service** - Install as a system service with `make install-service`
- Configurable sync interval (60s to 7 days, default 12 hours)
- Syslog logging for daemon mode

### CLI Changes
- Added `-d, --daemon` flag for daemon mode
- Added `-i, --interval SEC` flag to set sync interval
- Updated help text with daemon mode documentation

### Files Added
- `src/core/daemon.c/h` - Daemon implementation with signal handling
- `deploy/gmk67sts.service` - systemd unit file

---

## [1.0.0] - 2026-07-23

### Initial Release
- One-shot time synchronization for GMK67 keyboard
- Read-modify-write pattern preserves all keyboard settings
- BCD time encoding (matches keyboard protocol)
- CLI with `--sync`, `--help`, `--version` flags
- Modular code structure (device, protocol, config, timesync layers)
- Uses hidapi-hidraw backend for Linux udev compatibility

### Build System
- Makefile with dependency checking
- systemd service installation target
- Debug and release build configurations