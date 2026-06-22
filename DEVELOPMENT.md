# Apple A18 Pro Driver Suite - Build & Install Guide

---

## 1. Prerequisites

| Software | Purpose |
|----------|---------|
| Windows 10/11 (Build 22621+) | Target OS (x64 or ARM64) |
| Visual Studio 2022 | Compiler (C++ Desktop workload) |
| Windows Driver Kit (WDK) 11.0 | Kernel headers, linker, Inf2Cat |
| DebugView or WinDbg | Kernel debug log capture |

---

## 2. Building All Drivers

Each driver is an independent `.vcxproj`. Build them individually or add them to a shared `.sln`.

```cmd
:: Graphics
msbuild graphics\AppleA18ProDriver.vcxproj /p:Configuration=Release /p:Platform=x64

:: Chipset
msbuild chipset\AppleA18ProChipset.vcxproj /p:Configuration=Release /p:Platform=x64

:: Storage
msbuild storage\AppleA18ProStorage.vcxproj /p:Configuration=Release /p:Platform=x64

:: Audio
msbuild audio\AppleA18ProAudio.vcxproj /p:Configuration=Release /p:Platform=x64

:: USB
msbuild usb\AppleA18ProUsb.vcxproj /p:Configuration=Release /p:Platform=x64

:: Input
msbuild input\AppleA18ProInput.vcxproj /p:Configuration=Release /p:Platform=x64
```

Replace `x64` with `ARM64` for ARM builds.

---

## 3. Installing Drivers

Each driver folder has an `installer/` subdirectory. Right-click the appropriate `install.bat` and choose **Run as Administrator**.

| Driver | Installer Path |
|--------|---------------|
| Graphics | `graphics/installer/install.bat` |
| Chipset | `chipset/installer/install.bat` |
| Storage | `storage/installer/install.bat` |
| Audio | `audio/installer/install.bat` |
| USB | `usb/installer/install.bat` |
| Input | `input/installer/install.bat` |

All installers share a single self-signed certificate (`CN=AppleA18ProDriver`). The first installer you run will generate it; subsequent installers will reuse it.

**Reboot** the system after the first installation to activate test-signing mode.

---

## 4. Verifying Installation

Confirm all six drivers are registered in the Windows Driver Store:

```powershell
Get-WindowsDriver -Online -All | Where-Object { $_.OriginalFileName -match "AppleA18Pro" }
```

Expected output shows entries for:
- `AppleA18Pro.inf` (Display)
- `AppleA18ProChipset.inf` (System)
- `AppleA18ProStorage.inf` (SCSIAdapter)
- `AppleA18ProAudio.inf` (MEDIA)
- `AppleA18ProUsb.inf` (USB)
- `AppleA18ProInput.inf` (HIDClass)

---

## 5. Creating Virtual Devices (for testing without hardware)

Use WDK's `devcon.exe` to force-create virtual device nodes:

```cmd
devcon.exe install AppleA18Pro.inf         PCI\VEN_106B&DEV_16B5
devcon.exe install AppleA18ProChipset.inf  PCI\VEN_106B&DEV_16B0
devcon.exe install AppleA18ProStorage.inf  PCI\VEN_106B&DEV_16B1
devcon.exe install AppleA18ProAudio.inf    PCI\VEN_106B&DEV_16B2
devcon.exe install AppleA18ProUsb.inf      PCI\VEN_106B&DEV_16B3
devcon.exe install AppleA18ProInput.inf    PCI\VEN_106B&DEV_16B4
```

---

## 6. Debugging & Kernel Logs

All drivers emit debug messages with the prefix `[Apple A18 Pro ...]`.

1. Run **DebugView** (`dbgview.exe`) as Administrator.
2. Enable **Capture → Capture Kernel** and **Capture → Enable Verbose Kernel Output**.
3. Filter for `Apple A18 Pro` to see initialization and state change messages from all drivers.

Example log output:
```
[Apple A18 Pro Storage] Storage Driver loading...
[Apple A18 Pro Storage] Device object added.
[Apple A18 Pro Storage] Initializing ANS Controller...
[Apple A18 Pro Storage] ANS controller ready.
[Apple A18 Pro Audio] Audio Driver loading...
[Apple A18 Pro Audio] Initializing MCA Controller...
[Apple A18 Pro USB] USB Driver loading...
[Apple A18 Pro USB] Initializing USB PHY layer...
[Apple A18 Pro Input] Input Driver loading...
[Apple A18 Pro Input] SPI controller initialized - ready for input events.
```

---

## 7. Uninstalling Drivers

Each driver folder includes an `uninstall.bat`. Run as Administrator:

| Driver | Uninstaller Path |
|--------|-----------------|
| Graphics | `graphics/installer/uninstall.bat` |
| Chipset | `chipset/installer/uninstall.bat` |
| Storage | `storage/installer/uninstall.bat` |
| Audio | `audio/installer/uninstall.bat` |
| USB | `usb/installer/uninstall.bat` |
| Input | `input/installer/uninstall.bat` |

The shared certificate is only removed when the **last** suite driver is uninstalled.
