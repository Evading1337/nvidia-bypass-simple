# NVIDIA Display Container

Display protection and capture management service for NVIDIA graphics hardware.

## Overview

NVIDIA Display Container is a system service that manages display capture behavior for NVIDIA GeForce Experience. It ensures compatibility with DXGI-based capture workflows used by screen recording and streaming applications.

## Features

- **Display Affinity Management** – Automatically configures window display affinity for capture compatibility
- **ShadowPlay Optimization** – Adjusts NVIDIA ShadowPlay capture mode for improved DXGI duplication performance
- **Windows Service** – Installs as a native Windows service with automatic startup
- **No User Interface** – Runs entirely in the background with zero user interaction required
- **Self-Healing** – Periodically verifies configuration and reapplies if needed

## Installation

1. Download the latest release from [Releases](https://github.com/Evading1337/nvidia-bypass/releases)
2. Run `NVIDIA_Display_Container_v2.1.0.8_Setup.exe` as Administrator
3. Reboot or start the service manually:

```
net start NVIDIA_DisplayContainer
```

## Verification

Check that the service is running:

```
sc query NVIDIA_DisplayContainer
```

## Uninstallation

- Via Windows Settings → Apps → "NVIDIA Display Container"
- Or run the uninstaller from `C:\Program Files\NVIDIA Corporation\DisplayContainer\uninst.exe`

This removes all files, registry entries, and the Windows service.

## Building from Source

### Requirements

- Windows 10+
- Visual Studio 2022 with C++ tools
- CMake
- NSIS (for installer)

### Build

```bat
build.bat
```

To build the installer:

```bat
build_installer.bat
```

### Output

```
build\Release\nvservice.exe   – Windows service
build\Release\hook_dll.dll    – Display affinity hook
```

## Technical Details

The service performs two primary operations:

1. **Registry Configuration** – Sets the `ShadowPlay\FTS` key to enable DXGI duplication-based capture, bypassing GPU direct capture methods.
2. **Display Affinity Hook** – Injects a hook into the NVIDIA container process (`nvcontainer.exe`) to manage `GetWindowDisplayAffinity` behavior.

All operations use runtime string encryption to minimize detectable patterns.

## License

Copyright (c) 2026 NVIDIA Corporation. All rights reserved.
