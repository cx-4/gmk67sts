# Installation Guide

## Quick Install

### Ubuntu/Debian

```bash
# Install dependencies (hidraw backend - recommended)
sudo apt-get update
sudo apt-get install -y libhidapi-hidraw0 libhidapi-dev build-essential

# Alternative: libusb backend
# sudo apt-get install -y libhidapi-libusb0 libhidapi-dev build-essential

# Build and install
make
sudo make install
```

### Fedora

```bash
# Install dependencies
sudo dnf install hidapi hidapi-devel gcc make

# Build and install
make
sudo make install
```

### Arch Linux

```bash
# Install dependencies
sudo pacman -S hidapi gcc make

# Build and install
make
sudo make install
```

### macOS

```bash
# Install dependencies
brew install hidapi

# Build and install
make
sudo make install
```

## Manual Installation (if package manager fails)

### Building hidapi from source

```bash
# Clone hidapi repository
git clone https://github.com/libusb/hidapi.git
cd hidapi

# Build and install
./bootstrap
./configure
make
sudo make install
sudo ldconfig
```

## USB Permissions (Linux)

### Option 1: Use sudo (quick test)

```bash
sudo gmk67sts --sync
```

### Option 2: Install udev rule (recommended)

Create a udev rule granting the `plugdev` group access to `/dev/hidraw*`:

```bash
echo 'KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0660", GROUP="plugdev"' \
    | sudo tee /etc/udev/rules.d/50-gmk67.rules

# Reload udev rules
sudo udevadm control --reload-rules
sudo udevadm trigger

# Unplug and replug the keyboard, then run without sudo
gmk67sts --sync
```

### Option 3: Add user to plugdev group

```bash
# Add your user to plugdev group
sudo usermod -aG plugdev $USER

# Log out and log back in, then test
gmk67sts --sync
```

## Verify Installation

```bash
# Check if executable is in PATH
which gmk67sts

# Check version
gmk67sts --version

# Test sync (keyboard must be connected)
gmk67sts --sync
```

## Troubleshooting

### "hidapi.h: No such file or directory"

Install the hidapi development package:
- Ubuntu/Debian: `sudo apt-get install libhidapi-dev`
- Fedora: `sudo dnf install hidapi-devel`
- Arch: `sudo pacman -S hidapi`

### "cannot find -lhidapi-libusb"

The linker can't find hidapi library. Try:
1. Install hidapi (see above)
2. Run `sudo ldconfig` to update library cache
3. Check library path: `pkg-config --libs hidapi-libusb`

### "Permission denied" when accessing USB device

See "USB Permissions" section above. The easiest solution is to install the udev rule.

### "Device not found"

1. Ensure keyboard is connected via USB cable (not wireless/Bluetooth)
2. Try a different USB port
3. Check if device is detected: `lsusb | grep -i gmk`
4. Verify VID/PID: should be 320f:5055

## Uninstall

```bash
sudo make uninstall
```

Or manually:
```bash
sudo rm /usr/local/bin/gmk67sts
```