# Bug Fix: Configuration Read Failure

## Issue

Keyboard settings (illumination, LED effects, etc.) were being wiped after running time sync.

## Root Cause

The `config_read()` function in `src/core/config.c` was sending **NULL with 0 length** for the READ_DATA command (0x05):

```c
/* BUGGY CODE - DO NOT USE */
protocol_send_command(dev, CMD_READ_DATA, NULL, 0, position, chunk, 4)
```

However, the device expects **4 bytes of dummy data** with this command, even though the data is unused:

```python
# Reference implementation
buffer.extend(self.send_command(command_id=5, data=[0x00] * 4, pos=i*4))
```

## Impact

When READ_DATA was sent without the required 4-byte dummy payload:
- The device returned incorrect/incomplete data
- Bytes 37-47 (LED settings, winlock, underglow, etc.) were read as zeros
- These zeros were written back to the device, **wiping all user settings**

## Fix

Changed `config_read()` to send 4 bytes of dummy data with each READ_DATA command:

```c
/* FIXED CODE */
for (int i = 0; i < 12; i++) {
    uint8_t read_request[4] = {0};  /* 4 bytes of dummy data required */
    if (!protocol_send_command(dev, CMD_READ_DATA, read_request, 4, 
                               (uint32_t)i * 4, chunk, 4)) {
        /* ... error handling ... */
    }
    memcpy(&config[i * 4], chunk, 4);
}
```

## Safety Checks Added

Added `config_validate()` function to detect corrupted config reads:

1. **Zero-byte check**: Rejects config if >80% of bytes are zero
2. **Frame count check**: Rejects if both frame counts (bytes 34, 46) are zero AND many bytes are zero
3. **Validation on read**: `config_read()` now validates the buffer before returning success

## Files Modified

| File | Changes |
|------|---------|
| `src/core/config.c` | Fixed READ_DATA to send 4 dummy bytes, added `config_validate()` |
| `src/core/config.h` | Added `config_validate()` declaration |

## Testing

Before fix:
```bash
# Settings wiped after sync
```

After fix:
```bash
# Settings preserved, only time updated
./bin/gmk67sts --sync
```

## Prevention

To prevent similar issues in the future:

1. **Always match reference implementations** - When in doubt, check both Python and Node.js code
2. **Validate config buffers** - New `config_validate()` catches read failures
3. **Document protocol quirks** - Added comments explaining why 4 dummy bytes are required
4. **Test with visible settings** - Always verify lighting/image settings after sync

## Protocol Note

The GMK67 keyboard's READ_DATA command (0x05) **requires** a 4-byte dummy payload, even though the data is not used. This is a non-standard HID behavior that must be followed precisely.

```
Command: 0x05 (READ_DATA)
Position: 0, 4, 8, ..., 44 (12 chunks)
Payload: 4 bytes of 0x00 (REQUIRED)
Response: 4 bytes of config data
```

## Verification

To verify the fix is working:

1. Set custom lighting settings on the keyboard
2. Run `gmk67sts --sync`
3. Verify lighting settings are **preserved** (not reset to default)
4. Check logs show successful config read/write

```bash
# Enable verbose output
./bin/gmk67sts --sync

# Should see:
# - Configuration read (48 bytes)
# - Time updated: YYYY-MM-DD HH:MM:SS
# - Configuration written successfully
# - Time synchronized successfully
```

## Related

- Config buffer layout: `src/core/config.h` lines 11-18