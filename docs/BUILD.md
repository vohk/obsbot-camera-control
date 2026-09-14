# Building OBSBOT Control

Complete build instructions for compiling and installing OBSBOT Control on Linux.

## Table of Contents

- [Quick Start](#quick-start)
- [Dependencies](#dependencies)
- [Build Script (Recommended)](#build-script-recommended)
- [Manual Build](#manual-build)
- [Installation](#installation)
- [Uninstallation](#uninstallation)
- [Troubleshooting](#troubleshooting)

## Quick Start

The fastest way to get started:

```bash
# Clone the repository
git clone https://github.com/aaronsb/obsbot-camera-control.git
cd obsbot-camera-control

# Build and install with the automated script
./build.sh install --confirm
```

The build script automatically:
- Checks all dependencies
- Shows missing packages with install commands for your distro
- Builds the application
- Installs to `~/.local/bin`
- Adds desktop launcher to your application menu
- Optionally updates your PATH

## Dependencies

### Required Build Dependencies

#### Arch Linux / Manjaro
```bash
sudo pacman -S base-devel cmake qt6-base qt6-multimedia pkgconf
```

#### Debian / Ubuntu
```bash
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev pkg-config
```

#### Fedora / RHEL / CentOS
```bash
sudo dnf groupinstall 'Development Tools'
sudo dnf install cmake qt6-qtbase-devel qt6-qtmultimedia-devel pkgconfig
```

### Optional Runtime Dependencies

#### Camera Usage Detection (Recommended)
```bash
# Arch
sudo pacman -S lsof

# Debian/Ubuntu
sudo apt install lsof

# Fedora
sudo dnf install lsof
```

This enables the application to detect when other programs are using the camera.

#### Virtual Camera Output (Optional)
The project ships a systemd unit and modprobe configuration, but they are not enabled automatically. Enable them only if you want a persistent virtual camera device.

Install the kernel module for your distribution:
```bash
# Arch
sudo pacman -S v4l2loopback-dkms v4l-utils

# Debian/Ubuntu
sudo apt install v4l2loopback-dkms v4l2loopback-utils v4l-utils

# Fedora
sudo dnf install v4l2loopback v4l-utils
```

Load the module when you want to expose the virtual camera:
```bash
# using the provided systemd unit (installs the module options from /usr/lib/modprobe.d/)
sudo systemctl enable --now obsbot-virtual-camera.service

# or load it manually
sudo modprobe v4l2loopback video_nr=42 card_label="OBSBOT Virtual Camera" exclusive_caps=1
```

Adjust `video_nr` or `card_label` as needed. Ensure the device path matches the value configured inside the application (default: `/dev/video42`).

### Minimum Versions
- **CMake**: 3.16 or later
- **Qt6**: 6.2 or later
- **Compiler**: GCC 7+ or Clang 5+ (C++17 support required)

## Build Script (Recommended)

The `build.sh` script provides an easy, safe way to build and install the application.

### Features

- **Dependency Checking**: Automatically detects missing dependencies
- **Multi-Distribution Support**: Provides correct install commands for Arch, Debian, Fedora
- **Dry-Run Mode**: Shows what will happen without making changes
- **Safe Installation**: Requires explicit `--confirm` flag
- **Smart PATH Detection**: Checks and optionally updates shell configuration
- **Desktop Integration**: Installs application menu launcher and icon

### Available Commands

#### Build Only

```bash
# See what will happen (dry run)
./build.sh build

# Actually build the project
./build.sh build --confirm
```

Binaries will be in `build/obsbot-gui` and `build/obsbot-cli`

#### Build and Install

```bash
# See what will be installed (dry run)
./build.sh install

# Build and install
./build.sh install --confirm
```

During installation, you'll be prompted:
1. Whether to install the CLI tool (optional)
2. Whether to add `~/.local/bin` to your PATH (if not already present)

#### Clean Build Directory

```bash
# Remove build artifacts
./build.sh clean --confirm
```

#### Get Help

```bash
./build.sh help
```

### What Gets Installed

- **Binaries**: `~/.local/bin/obsbot-gui` (and optionally `obsbot-cli`)
- **Desktop Launcher**: `~/.local/share/applications/obsbot-control.desktop`
- **Icon**: `~/.local/share/icons/hicolor/scalable/apps/obsbot-control.svg`

After installation, the application will appear in your system's application menu.

## Manual Build

If you prefer to build manually without the script:

### Step 1: Install Dependencies

Install the dependencies for your distribution (see [Dependencies](#dependencies) section).

### Step 2: Create Build Directory

```bash
mkdir build
cd build
```

### Step 3: Configure with CMake

```bash
cmake ..
```

CMake will check for required dependencies and configure the build.

### Step 4: Compile

```bash
# Use all available CPU cores
make -j$(nproc)
```

### Step 5: Run

```bash
# Run directly from build directory
./obsbot-gui
```

Or install manually:

```bash
# Copy to local bin
cp obsbot-gui ~/.local/bin/
cp obsbot-cli ~/.local/bin/
chmod +x ~/.local/bin/obsbot-meet2-{gui,cli}

# Install desktop launcher
mkdir -p ~/.local/share/applications
cp ../obsbot-control.desktop ~/.local/share/applications/

# Install icon
mkdir -p ~/.local/share/icons/hicolor/scalable/apps
cp ../resources/icons/camera.svg ~/.local/share/icons/hicolor/scalable/apps/obsbot-control.svg

# Update desktop database
update-desktop-database ~/.local/share/applications
```

## Installation

### Using Build Script

The recommended installation method:

```bash
./build.sh install --confirm
```

This handles everything automatically, including:
- Building the application
- Installing binaries
- Installing desktop launcher
- Checking PATH configuration
- Offering to update shell configuration files

### Install Directory

By default, binaries install to `~/.local/bin` (XDG-compliant).

You can override this by setting the `XDG_BIN_HOME` environment variable:

```bash
export XDG_BIN_HOME="$HOME/bin"
./build.sh install --confirm
```

### Adding to PATH

If `~/.local/bin` is not in your PATH, the installer will offer to add it.

To add manually:

**Bash** (~/.bashrc):
```bash
export PATH="$HOME/.local/bin:$PATH"
```

**Zsh** (~/.zshrc):
```bash
export PATH="$HOME/.local/bin:$PATH"
```

After adding, restart your shell or run:
```bash
source ~/.bashrc  # or ~/.zshrc
```

## Uninstallation

### Using Uninstall Script

```bash
# See what will be removed (dry run)
./uninstall.sh

# Actually uninstall
./uninstall.sh --confirm
```

The uninstall script removes:
- Application binaries
- Desktop launcher
- Application icon
- Configuration file
- Configuration directory (if empty)

**What it does NOT remove:**
- Shell configuration changes (PATH additions)
- The config directory if it contains unexpected files

### Manual Uninstallation

```bash
rm -f ~/.local/bin/obsbot-gui
rm -f ~/.local/bin/obsbot-cli
rm -f ~/.local/share/applications/obsbot-control.desktop
rm -f ~/.local/share/icons/hicolor/scalable/apps/obsbot-control.svg
rm -rf ~/.config/obsbot-control
update-desktop-database ~/.local/share/applications
```

## Troubleshooting

### Build Errors

#### CMake can't find Qt6

**Problem**: CMake reports "Could not find Qt6"

**Solution**:
```bash
# Arch
sudo pacman -S qt6-base qt6-multimedia

# Debian/Ubuntu
sudo apt install qt6-base-dev qt6-multimedia-dev

# Fedora
sudo dnf install qt6-qtbase-devel qt6-qtmultimedia-devel
```

#### Linking error: "cannot find -ldev"

**Problem**: Linker can't find the OBSBOT SDK library

**Solution**: This means the SDK libraries weren't included in your git clone. They should be in `sdk/lib/`. If missing:

1. Check if `sdk/lib/libdev.so*` files exist
2. If not, the repository may have been cloned before the SDK was added
3. Pull the latest changes: `git pull`

#### Compiler version too old

**Problem**: Errors about C++17 features not being supported

**Solution**: Update your compiler:
```bash
# Check current version
g++ --version

# Should be GCC 7+ or Clang 5+
# Update if needed through your distribution's package manager
```

### Runtime Issues

#### Application doesn't start

1. **Check if installed correctly**:
   ```bash
   which obsbot-gui
   ls -l ~/.local/bin/obsbot-gui
   ```

2. **Try running from build directory**:
   ```bash
   cd build
   ./obsbot-gui
   ```

3. **Check for error messages**:
   ```bash
   ./obsbot-gui 2>&1 | tee error.log
   ```

#### Desktop launcher doesn't appear

1. **Update desktop database**:
   ```bash
   update-desktop-database ~/.local/share/applications
   ```

2. **Check desktop environment**:
   - Some minimal window managers don't support .desktop files
   - Try running from terminal: `obsbot-gui`

3. **Verify installation**:
   ```bash
   ls -l ~/.local/share/applications/obsbot-control.desktop
   ```

#### Permission denied errors

Ensure the binary is executable:
```bash
chmod +x ~/.local/bin/obsbot-gui
```

### Build Script Issues

#### Colors showing as escape codes

This shouldn't happen anymore (fixed in recent versions), but if you see literal `\033[0;32m` codes:

```bash
# Update to latest version
git pull
```

#### Dependency checker shows wrong install commands

The script detects your distribution automatically. If detection fails:

1. Check `/etc/os-release`:
   ```bash
   cat /etc/os-release
   ```

2. Report the issue with your distribution details

## Development Builds

### Debug Build

```bash
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Release Build with Debug Symbols

```bash
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j$(nproc)
```

## Getting Help

- **Build Issues**: Check [GitHub Issues](https://github.com/aaronsb/obsbot-camera-control/issues)
- **Usage Questions**: See main [README](../README.md)
- **Report Bugs**: [Open an Issue](https://github.com/aaronsb/obsbot-camera-control/issues/new)

## Build in Distrobox, run on Bazzite

Build a relocatable directory containing the app, OBSBOT SDK, Qt libraries,
plugins, and their userspace dependencies. Kernel modules and glibc remain on
the host. Use a build distribution with glibc no newer than the host: Fedora 44
matches Bazzite 44; a rolling Arch container may be too new.

Inside a Fedora 44 Distrobox:

```bash
sudo dnf install cmake gcc-c++ make qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwayland-devel patchelf
cd /path/to/obsbot-camera-control
./scripts/build-bundle.sh
```

Bundling requires Qt 6.10+. It creates `build-bundle/obsbot-control/` and a tarball
beside it. Nothing is installed on the host. An alternative build directory can
be set with `OBSBOT_BUILD_DIR=/absolute/path` (use separate directories per distro).

From a **host terminal**, run the shared directory directly:

```bash
/path/to/obsbot-camera-control/build-bundle/obsbot-control/obsbot-control --check-runtime
/path/to/obsbot-camera-control/build-bundle/obsbot-control/obsbot-control
```

The first command checks Qt, platform plugins, and camera enumeration without
applying camera settings. The bundle can also be copied or extracted to a stable
location such as `~/.local/opt/obsbot-control`; run its `obsbot-control` launcher.
Do not use `distrobox-export` for this bundle: that launches inside the container.
This is a bundle for a compatible host, not a universal Linux binary. Rebuild it
after major distribution upgrades. GPU drivers still come from the host.

Virtual camera output is optional; hardware controls and HDR do not require
v4l2loopback. If the host already has an OBS virtual camera, select its existing
`/dev/videoN` path in the app rather than loading another module. On the tested
Bazzite host this was `/dev/video0`; `/dev/video1` was the physical Meet SE.
Device numbers can change after reconnecting or rebooting.

### Read-only hardware regression check

With an SDK-supported OBSBOT attached:

```bash
cmake -S . -B build-test -DOBSBOT_BUILD_HARDWARE_TESTS=ON
cmake --build build-test --target camera-discovery-test
./bin/camera-discovery-test
./bin/camera-discovery-test --cancel
```

This does not apply settings. It succeeds only if asynchronous discovery selects
the SDK backend, and fails on V4L2 fallback or a 12-second detection timeout.

### Existing loopback module, missing video42

`modprobe` does not reconfigure a module that is already loaded. For example,
Bazzite may already expose an OBS virtual camera at `/dev/video0`. Use the app's
**Detect** button to choose an existing loopback node; physical webcams are not
accepted as virtual outputs. Avoid having OBS and this app produce output to the
same node simultaneously.

If a separate `/dev/video42` is wanted, a host with `v4l2loopback-ctl` can add it
without unloading the module or disrupting existing nodes:

```bash
sudo v4l2loopback-ctl add -n "OBSBOT Virtual Camera" -x 1 /dev/video42
```

This is a runtime addition, not persistent boot configuration. The app does not
execute module-management commands automatically.

### Preview hardware checks

With preview closed in other applications, build the optional checks above and
run these on the host (adjust the physical camera path):

```bash
cmake --build build-test --target mjpeg-capture-test preview-pipeline-test
./bin/mjpeg-capture-test /dev/video1 60
./bin/mjpeg-capture-test /dev/video1 30
QT_QPA_PLATFORM=wayland ./bin/preview-pipeline-test /dev/video1
```

The first two verify the negotiated rate and decoded frame delivery. The last
opens a temporary preview window, checks HDR-compatible choices, switches from
1080p60 to 1080p30, and verifies GPU-processed images. Images stay in memory;
these checks do not write recordings or apply saved camera settings.
