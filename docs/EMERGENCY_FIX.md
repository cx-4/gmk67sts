# Emergency Fix Guide

## Problem
Keyboard illumination stopped working and VIA app shows error: "Receiving incorrect response for command"

## Cause
The buggy C code (before v1.2.0) sent READ_DATA commands without the required 4-byte dummy payload, causing the keyboard to return incorrect data. When this corrupted data was written back, it wiped all lighting settings (bytes 1-8 for underglow, bytes 28-32 for LED).

## Solution

### Step 1: Physical Reset
1. **Unplug the keyboard USB cable**
2. **Wait 10 seconds** - This is critical! It lets the keyboard's microcontroller fully reset and clears any confused state
3. **Replug the keyboard**

### Step 2: Restore Factory Config
```bash
./bin/gmk67sts --restore-defaults
```

This writes the saved 48-byte factory config (dumped from a known-good keyboard
state) and updates the time to current. It restores:
- Underglow: effect=0x06 (FULL_ONE_COLOR), brightness=9, RGB(255,1,0)
- LED: mode=0x03 (FIXED_COLOR), saturation=7, color=0x03 (GREEN)
- Image display: showImage=2, image1Frames=1, image2Frames=1
- Frame duration: 100ms

### Step 3: Verify
Check that:
- Underglow LEDs are now showing an effect
- Keyboard responds to lighting changes

### Step 4: Fix VIA (if still needed)
1. Close VIA app completely
2. Unplug/replug keyboard again
3. Reopen VIA app

## Alternative: Load Different Profile

If you want different lighting than factory defaults:

```bash
# List all profiles
./bin/gmk67sts --list-profiles

# Load gaming profile (red theme)
./bin/gmk67sts --load-profile gaming

# Load matrix profile (green theme)
./bin/gmk67sts --load-profile matrix

# Load productivity profile (white theme)
./bin/gmk67sts --load-profile productivity
```

## What If It Still Doesn't Work?

### Check USB Connection
```bash
# Verify device is detected
lsusb | grep -i "320f"

# Check HID device exists
ls -la /dev/hidraw* | grep plugdev
```

## Prevention

The C code (v1.5.1+) now:
- Sends 4-byte dummy payload with READ_DATA (matches Python/Node.js)
- Validates config after read (detects corruption)
- Uses read-modify-write for profile loading (preserves all settings)
- `--restore-defaults` writes a saved full 48-byte factory config
- `--dump` provides read-only config inspection (no writes)
- The broken `--reset` command has been removed entirely

## Technical Details

### Factory Config (saved 2026-07-23)
```c
static const uint8_t FACTORY_CONFIG[48] = {
    0x00, 0x06, 0x09, 0x04, 0x01, 0x00, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x02, 0x00, 0x03, 0x02, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x01, 0x00
};
```

Key bytes:
- Byte 1: underglow effect = 0x06 (FULL_ONE_COLOR)
- Byte 2: underglow brightness = 0x09
- Bytes 6-8: underglow RGB = (0xff, 0x01, 0x00)
- Byte 20: 0xff (reserved)
- Byte 28: LED mode = 0x03 (FIXED_COLOR)
- Byte 29: LED saturation = 0x07
- Byte 30: 0x02
- Byte 32: LED color = 0x03 (GREEN)
- Byte 33: showImage = 0x02 (show slot 1)
- Byte 34: image1Frames = 0x01
- Bytes 35-41: time (overwritten on restore)
- Bytes 43-44: frameDuration = 100ms (little-endian)
- Byte 46: image2Frames = 0x01