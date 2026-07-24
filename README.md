# gmk67sts v1.5.1

Time synchronization and lighting control for GMK67S mechanical keyboard.

**Latest version:** 1.5.1 | [Changelog](docs/CHANGELOG.md)

## Features

- **Time sync** - Sync system time to keyboard's internal clock
- **Daemon mode** - Run as background service with periodic sync
- **Restore defaults** - Restore saved factory config (full 48-byte config dump)
- **Load profiles** - Apply preset lighting configurations (read-modify-write)
- **Config dump** - Read-only dump of current keyboard config as C array
- **Preserves settings** - Profile loading uses read-modify-write to keep all other settings

## Quick Start

### Restore Factory Config

Restores the saved factory default config (all 48 bytes, including lighting,
image settings, and display config). Time is updated to current.

```bash
gmk67sts --restore-defaults
```

### Dump Current Config (Read-Only)

Reads the current config from the keyboard and prints it as a C array.
Does NOT write anything to the keyboard — safe to run anytime.

```bash
gmk67sts --dump
```

### Time Sync

```bash
gmk67sts --sync
```

### Load Lighting Profile

```bash
# List available profiles
gmk67sts --list-profiles

# Load a profile
gmk67sts --load-profile gaming
gmk67sts --load-profile matrix
gmk67sts --load-profile productivity
```

### Daemon Mode

```bash
# Run as daemon (12-hour sync interval)
gmk67sts --daemon

# Custom interval (1 hour)
gmk67sts --daemon --interval 3600

# Install as systemd service
sudo make install-service
sudo systemctl enable --now gmk67sts

# View logs
journalctl -u gmk67sts -f
```

## Available Profiles

| Profile | Description |
|---------|-------------|
| `gaming` | Aggressive red theme with fast effects |
| `relaxed` | Calm cyan breathing effect for typing |
| `party` | Fast rainbow effects for maximum visual impact |
| `minimal` | All lighting off for minimal distraction |
| `productivity` | Subtle white backlight for focused work |
| `purple-wave` | Purple wave effect with moderate brightness |
| `matrix` | Green rain effect for Matrix-style aesthetics |
| `sunset` | Warm orange/yellow sunset colors |

Use `--restore-defaults` to restore the full saved factory config (not just lighting).

## CLI Options

```
-s, --sync             Sync current system time to keyboard
-d, --daemon           Run as daemon with periodic sync
-i, --interval SEC     Sync interval (default: 43200, min: 60, max: 604800)
-r, --restore-defaults Restore saved factory config (full 48-byte config)
-l, --load-profile NAME Load lighting profile (read-modify-write)
    --list-profiles    List available lighting profiles
    --dump             Read-only: dump current config as C array
-h, --help             Show help message
-v, --version          Show version information
```

## Installation

### Dependencies (Ubuntu/Debian)

```bash
sudo apt-get install libhidapi-hidraw0 libhidapi-dev
```

### Build

```bash
make
sudo make install
```

### USB Permissions

Access to the keyboard's HID interface requires either `sudo` or a udev rule
granting the `plugdev` group access to `/dev/hidraw*` devices. Create the rule:

```bash
echo 'KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0660", GROUP="plugdev"' \
    | sudo tee /etc/udev/rules.d/50-gmk67.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Then add your user to the `plugdev` group (if not already a member), log out
and back in, and replug the keyboard:

```bash
sudo usermod -aG plugdev $USER
```

Alternatively, run with `sudo` for a one-off:

```bash
sudo gmk67sts --sync
```

## Emergency Recovery

If your keyboard's lighting is not working:

1. **Unplug the keyboard USB cable**
2. **Wait 10 seconds** (lets controller fully reset)
3. **Replug the keyboard**
4. **Restore factory config:**
   ```bash
   gmk67sts --restore-defaults
   ```
5. **Verify illumination works**
6. **If VIA still errors:** Close VIA, unplug/replug keyboard, reopen VIA

## Technical Details

### Factory Config (v1.5.1)

The `--restore-defaults` command writes a saved 48-byte factory config that was
dumped from a known-good keyboard state. This includes lighting, LED, image
display settings, and frame duration. Time is updated to current on restore.

Use `--dump` to read and inspect the current config at any time (read-only).

### Protocol Fix (v1.2.0)

Fixed critical bug where READ_DATA command was sent without required 4-byte dummy payload, causing config corruption. The device returns bad data without the dummy payload; writing that back wiped all user settings.

### Config Layout

- Bytes 1-8: Underglow settings (effect, brightness, speed, RGB)
- Bytes 28-32: LED settings (mode, saturation, color)
- Bytes 35-41: Time (BCD encoded: sec, min, hour, dow, date, month, year)
- Bytes 33-34, 46: Image display settings
