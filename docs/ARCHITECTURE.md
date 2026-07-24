# Architecture

## Overview

GMK67STS is a modular C application that synchronizes system time to the GMK67 keyboard's internal RTC. The codebase is organized into logical layers with clear separation of concerns.

## Layer Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        CLI Layer                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ src/cli/main.c                                      │    │
│  │ - Argument parsing (getopt_long)                    │    │
│  │ - Help/version display                              │    │
│  │ - Entry point orchestration                         │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼ calls timesync_sync()
┌─────────────────────────────────────────────────────────────┐
│                   Application Layer                          │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ src/core/timesync.c                                 │    │
│  │ - High-level sync orchestration                     │    │
│  │ - Device lifecycle management                       │    │
│  │ - Error handling                                    │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            │
              ┌─────────────┴─────────────┐
              ▼                           ▼
┌──────────────────────────┐  ┌──────────────────────────────┐
│     Device Layer         │  │      Config Layer            │
│  ┌────────────────────┐  │  │  ┌────────────────────────┐  │
│  │ src/core/device.c  │  │  │  │ src/core/config.c      │  │
│  │ - HID enumeration  │  │  │  │ - Read config (0x05)   │  │
│  │ - Open/close       │  │  │  │ - Write config (0x06)  │  │
│  │ - HID API init     │  │  │  │ - Time field updates   │  │
│  └────────────────────┘  │  │  │ - BCD conversion       │  │
└──────────────────────────┘  │  └────────────────────────┘  │
                              └──────────────────────────────┘
                                          │
                                          ▼ uses
                              ┌──────────────────────────────┐
                              │     Protocol Layer           │
                              │  ┌────────────────────────┐  │
                              │  │ src/core/protocol.c    │  │
                              │  │ - Report building      │  │
                              │  │ - Checksum calculation │  │
                              │  │ - Send/receive         │  │
                              │  │ - Command matching     │  │
                              │  └────────────────────────┘  │
                              └──────────────────────────────┘
                                          │
                                          ▼
                              ┌──────────────────────────────┐
                              │   External: libhidapi        │
                              └──────────────────────────────┘
```

## Module Details

### CLI Layer (`src/cli/`)

**main.c**
- Parses command-line arguments using `getopt_long`
- Handles `-s/--sync`, `-h/--help`, `-v/--version` flags
- Calls `timesync_sync()` and returns appropriate exit codes
- No business logic, only orchestration

### Application Layer (`src/core/timesync.c`)

**timesync.c**
- High-level orchestration of the sync process
- Manages device lifecycle (init → find → open → close → exit)
- Coordinates config read → update → write sequence
- Handles cleanup on errors (goto-based resource management)

### Device Layer (`src/core/device.c`)

**device.c**
- `device_find_path()` - Enumerates HID devices, finds GMK67 by VID/PID/interface
- `device_open()` - Opens HID device by path
- `device_close()` - Safely closes device
- `device_init_hidapi()` / `device_exit_hidapi()` - Library lifecycle

**Key design decisions:**
- Returns `char*` path that caller must `free()`
- Interface 3 is preferred (config interface), falls back to any matching device
- Error messages include helpful hints (sudo, udev rules)

### Configuration Layer (`src/core/config.c`)

**config.c**
- `config_read()` - Reads 48-byte config using Python protocol (INIT→PREP_READ×10→COMMIT→READ×12)
- `config_write()` - Writes config (INIT→WRITE→COMMIT)
- `config_update_time()` - Updates time fields with current system time
- `to_bcd()` - Converts decimal to BCD format (internal static function)

**Protocol sequence for read:**
```
CMD_INIT (0x01)
CMD_PREP_READ (0x03) × 9  [positions 0, 4, 8, ..., 32]
CMD_PREP_READ (0x03) × 1  [position 36, 1 byte]
CMD_COMMIT (0x02)
CMD_READ_DATA (0x05) × 12 [positions 0, 4, 8, ..., 44]
```

### Protocol Layer (`src/core/protocol.c`)

**protocol.c**
- `protocol_calc_checksum()` - Sums bytes 3-63, returns 16-bit value
- `protocol_send_command()` - Builds 64-byte HID report, sends, waits for ACK

**HID Report Format (64 bytes):**
```
Byte 0:     Report ID (0x04)
Byte 1-2:   Checksum (little-endian, bytes 3-63)
Byte 3:     Command ID
Byte 4:     Data length
Byte 5-7:   24-bit position offset (little-endian)
Byte 8-63:  Data payload (max 56 bytes)
```

**Response handling:**
- Loops reading until command byte (byte 3) matches sent command
- Discards non-matching responses (handles stale data)
- 30-second total timeout (60 attempts × 5000ms read timeout)

## Data Flow: Time Sync

```
1. main.c
   └─> timesync_sync()

2. timesync_sync()
   ├─> device_init_hidapi()
   ├─> device_find_path()
   ├─> device_open()
   ├─> config_read()
   │    └─> protocol_send_command(CMD_INIT)
   │    └─> protocol_send_command(CMD_PREP_READ) × 10
   │    └─> protocol_send_command(CMD_COMMIT)
   │    └─> protocol_send_command(CMD_READ_DATA) × 12
   ├─> config_update_time()
   │    └─> localtime()
   │    └─> to_bcd() × 7
   ├─> config_write()
   │    └─> protocol_send_command(CMD_INIT)
   │    └─> protocol_send_command(CMD_WRITE_CONFIG)
   │    └─> protocol_send_command(CMD_COMMIT)
   ├─> device_close()
   └─> device_exit_hidapi()
```

## Error Handling Strategy

- **CLI layer**: Returns `EXIT_SUCCESS` (0) or `EXIT_FAILURE` (1)
- **Application layer**: Returns 0 on success, non-zero on failure
- **Core modules**: Return `true`/`false` or specific error codes
- **Resource cleanup**: Uses `goto cleanup` pattern for guaranteed cleanup
- **Error messages**: Printed to `stderr` with context

## Memory Management

- **Device path**: `strdup()` in `device_find_path()`, caller must `free()`
- **HID device**: Opened by `device_open()`, closed by `device_close()`
- **Config buffer**: Stack-allocated (48 bytes), passed by pointer
- **No dynamic allocation** in core modules (except device path string)

## Build System

**Makefile features:**
- Automatic dependency detection via `pkg-config`
- Separate compilation of modules
- Debug build target (`make debug`)
- Install/uninstall targets
- Dependency check before build

**Output:**
```
build/
├── core/
│   ├── config.o
│   ├── device.o
│   ├── protocol.o
│   └── timesync.o
└── cli/
    └── main.o

bin/
└── gmk67sts (linked executable)
```

## Testing Checklist

- [ ] `make` builds without warnings
- [ ] `./bin/gmk67sts --help` shows usage
- [ ] `./bin/gmk67sts --version` shows version
- [ ] `./bin/gmk67sts --sync` with keyboard connected syncs time
- [ ] `./bin/gmk67sts --sync` without keyboard returns error
- [ ] All other settings preserved after sync (lighting, images, etc.)

## Future Enhancements

- [ ] Add verbose/debug mode (`-d` flag)
- [ ] Add config dump mode (`--dump-config`)
- [ ] Add lighting configuration CLI
- [ ] Add image upload functionality
- [ ] Unit tests for BCD conversion
- [ ] Integration tests with mock HID device