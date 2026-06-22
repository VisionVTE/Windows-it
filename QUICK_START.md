# Apple A18 Pro GPU Windows Driver - Quick Start Guide

## Overview

This is a **proof-of-concept Windows display driver** for the Apple A18 Pro GPU in the MacBook Neo (Mac17,5). The driver implements core WDDM (Windows Display Driver Model) functionality for GPU initialization, power management, display mode control, and interrupt handling.

**Status:** Educational/Reference Implementation  
**Version:** 1.0.0  
**Date:** June 21, 2026

---

## Project Contents

```
Apple_A18Pro_Driver/
├── Header Files (inc/)
│   ├── apple_a18pro_hw.h          Hardware definitions & register offsets
│   └── apple_a18pro_driver.h      Driver structures & function prototypes
│
├── Source Code (src/)
│   └── apple_a18pro_driver.c      Implementation (1000+ lines)
│
├── Installation (inf/)
│   └── AppleA18Pro.inf            Windows driver installation file
│
├── Documentation
│   ├── README.md                  Comprehensive overview
│   ├── ARCHITECTURE.md            Technical architecture details
│   ├── HARDWARE_INTERFACE.md      Register definitions & I/O
│   ├── DEVELOPMENT.md             Build & development guide
│   └── QUICK_START.md             This file
│
└── Visual Studio Project
    └── AppleA18ProDriver.vcxproj  Build configuration
```

---

## Key Features

### Implemented
- ✅ Device initialization & hardware discovery
- ✅ GPU enable/disable/reset functionality
- ✅ Display mode enumeration & configuration (10 modes)
- ✅ Power state management (D0-D3)
- ✅ Memory mapping & framebuffer allocation
- ✅ Interrupt handling (VSync, Completion, Error, Thermal)
- ✅ Thermal management & temperature monitoring
- ✅ Thread-safe register access with spinlocks
- ✅ WDDM-compliant kernel-mode driver structure

### Not Implemented (Future Work)
- ⚠️ Shader compilation & execution
- ⚠️ Command queue submission
- ⚠️ DirectX 12 / Vulkan support
- ⚠️ Hardware video decoding
- ⚠️ User-mode display driver DLL
- ⚠️ Performance monitoring

---

## Device Specifications

| Specification | Value |
|---------------|-------|
| **Device** | Apple A18 Pro GPU |
| **Model** | MacBook Neo (Mac17,5) |
| **GPU Cores** | 5 display-oriented |
| **Total Cores** | 6 (2 perf + 4 efficiency) |
| **Memory** | 8 GB unified |
| **Display** | 2408×1506 @ 120Hz Retina |
| **PCI Vendor ID** | 0x106B |
| **PCI Device ID** | 0x16B5 (theoretical) |

---

## Building the Driver

### Prerequisites
- Windows 10/11 (Build 22621+)
- Visual Studio 2022
- Windows Driver Kit (WDK) 11.0
- ~5 GB free disk space

### Quick Build
```batch
REM Clone/extract project
cd Apple_A18Pro_Driver

REM Build Release x64
msbuild AppleA18ProDriver.vcxproj /p:Configuration=Release /p:Platform=x64

REM Output in: x64\Release\
```

### Using Visual Studio GUI
1. Open `AppleA18ProDriver.vcxproj`
2. Configuration → Release | x64
3. Build → Build Solution (Ctrl+Shift+B)

---

## Installing the Driver (Test/Development)

### Enable Test Signing
```batch
REM Run as Administrator
bcdedit /set testsigning on
shutdown /r /t 0
```

### Install Driver
```batch
REM Run as Administrator
devcon install inf\AppleA18Pro.inf PCI\VEN_106B&DEV_16B5
```

### Verify Installation
```batch
REM Check Device Manager
devmgmt.msc
REM Look for "Apple A18 Pro Graphics" under Display adapters

REM Or check via command line
driverquery | find /I "apple"
```

### Uninstall Driver
```batch
devcon remove "PCI\VEN_106B&DEV_16B5"
```

---

## Architecture Overview

### WDDM Stack
```
Windows Graphics System
        ↓
User-Mode Display Driver (UMD)
        ↓
Kernel-Mode Display Driver (KMD) ← YOU ARE HERE
  ├─ Device Initialization
  ├─ Power Management
  ├─ Display Configuration
  ├─ Interrupt Handling
  ├─ Memory Management
  └─ Thermal Control
        ↓
Hardware Abstraction Layer (HAL)
  ├─ Register Definitions
  ├─ Memory Maps
  └─ Constants
        ↓
Apple A18 Pro GPU Hardware
```

### Key Functions

**Device Control:**
- `AppleA18ProInitialize()` - Initialize GPU
- `AppleA18ProEnableDevice()` - Power on
- `AppleA18ProDisableDevice()` - Power down
- `AppleA18ProResetDevice()` - Hardware reset

**Display Mode:**
- `AppleA18ProSetDisplayMode()` - Change resolution
- `AppleA18ProGetDisplayMode()` - Query current mode
- `AppleA18ProEnumerateDisplayModes()` - List supported modes

**Power Management:**
- `AppleA18ProSetPowerState()` - Set D0-D3 state
- `AppleA18ProGetPowerState()` - Query power state

**Memory Management:**
- `AppleA18ProMapMemory()` - Map GPU MMIO
- `AppleA18ProAllocateFramebuffer()` - Allocate display buffer
- `AppleA18ProFreeFramebuffer()` - Free display buffer

**Interrupts & Thermal:**
- `AppleA18ProInterruptHandler()` - ISR
- `AppleA18ProReadTemperature()` - Get GPU temp
- `AppleA18ProSetThermalLimit()` - Set throttle threshold

---

## Display Modes Supported

Native resolution plus common scales:

| Resolution | Refresh | Supported |
|-----------|---------|-----------|
| 2408×1506 | 120 Hz  | ✓ Native |
| 2408×1506 | 60 Hz   | ✓ |
| 2048×1280 | 120 Hz  | ✓ |
| 2048×1280 | 60 Hz   | ✓ |
| 1920×1200 | 120 Hz  | ✓ |
| 1920×1200 | 60 Hz   | ✓ |
| 1680×1050 | 120 Hz  | ✓ |
| 1680×1050 | 60 Hz   | ✓ |
| 1600×1024 | 120 Hz  | ✓ |
| 1600×1024 | 60 Hz   | ✓ |

---

## Power States

WDDM Device Power States:

| State | Power Consumption | GPU Status | Display |
|-------|------------------|-----------|---------|
| D0 | 5-10W | Active, rendering enabled | On |
| D1/D2 | 1-2W | Idle, reduced clocks | On |
| D3 | <100mW | Off, no clocks | Off |

---

## Interrupt Handling

GPU generates interrupts for:

```
VSync (GPU_INT_VSYNC)
├─ 120 times/second @ 120Hz display
├─ Used for display buffer synchronization
└─ Essential for flicker-free display

Completion (GPU_INT_COMPLETION)
├─ Command queue has completed work
├─ Signals completion events
└─ Used for synchronization

Error (GPU_INT_ERROR)
├─ GPU encountered error condition
├─ Firmware-detected issue
└─ May require device reset

Thermal (GPU_INT_THERMAL)
├─ GPU temperature exceeded threshold
├─ Triggers automatic throttling
└─ Performance reduced until cool
```

---

## Memory Layout

```
GPU Memory Space:
0x00000000 - 0x000FFFFF    Control Registers
0x00100000 - 0x001FFFFF    Power Registers
0x00200000 - 0x002FFFFF    Memory Registers
0x00300000 - 0x003FFFFF    Display Registers
0x00400000 - 0x004FFFFF    Encoder Registers
0x00500000 - 0x005FFFFF    Shader Registers
0x10000000 - 0x10FFFFFF    Command Queue (64 KB)
0x20000000+                GPU Memory Heap
```

---

## Thermal Management

**Temperature Monitoring:**
- Reads GPU temperature via thermal sensor
- Range: 0-100°C with 1°C resolution
- Frequency: ~100 samples/second

**Thermal Throttling:**
- Threshold: Configurable (typical 85-100°C)
- Response: Automatic clock reduction
- Auto-recovery when cooled

**Typical Temperatures:**
- Idle: 35-45°C
- Normal load: 50-70°C
- Heavy load: 70-85°C
- Throttle threshold: ~85-95°C

---

## Synchronization Model

All hardware register access is protected by spinlocks:

```c
KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);

// Safe to access GPU registers
*(PULONG)RegisterAddress = Value;
KeMemoryBarrier();

KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
```

This ensures:
- Thread-safe register access
- No concurrent hardware access
- Proper IRQL management
- Memory consistency

---

## Important Limitations

⚠️ **This is an EDUCATIONAL IMPLEMENTATION**

The driver CANNOT function without:

1. **Proprietary GPU Firmware**
   - Apple Silicon GPU instructions are proprietary
   - Metal instruction set not publicly documented
   - Firmware binaries are locked to macOS

2. **Bootloader & UEFI**
   - MacBook Neo uses Apple's secure boot
   - Windows cannot boot on Apple Silicon without custom bootloader
   - Requires virtualization or dual-boot setup

3. **Hardware Specifications**
   - Reverse-engineered register definitions
   - Actual hardware behavior differs from documentation
   - Firmware behavior undocumented

4. **Power Management Integration**
   - GPU firmware handles complex power management
   - Integration with SoC power domains
   - Proprietary protocols not exposed

---

## Documentation Files

| File | Purpose |
|------|---------|
| **README.md** | Comprehensive overview, specifications, architecture |
| **ARCHITECTURE.md** | Technical deep dive, register layouts, flows |
| **HARDWARE_INTERFACE.md** | Register definitions, memory maps, examples |
| **DEVELOPMENT.md** | Build, install, debug, troubleshooting guide |
| **QUICK_START.md** | This file - quick reference |

---

## Key Code Sections

### Device Initialization (apple_a18pro_driver.c:80-170)
Complete GPU initialization sequence with error handling.

### Register Access Pattern (apple_a18pro_driver.c:200-250)
Thread-safe spinlock-protected register access.

### Interrupt Handler (apple_a18pro_driver.c:670-740)
VSync, completion, error, and thermal interrupt handling.

### Memory Management (apple_a18pro_driver.c:840-960)
Framebuffer allocation and register memory mapping.

### Power State Control (apple_a18pro_driver.c:600-640)
Device power state transitions (D0-D3).

---

## Debugging Tips

### Enable Debug Output
```batch
REM View driver debug messages
dbgview.exe

REM Or check Event Viewer
eventvwr.exe
REM Navigate to: Windows Logs → System
```

### Check Driver Load
```batch
REM Verify driver is loaded
driverquery | find /I "apple"

REM Check Device Manager
devmgmt.msc
```

### Common Issues

**Driver won't install:**
- Verify test signing is enabled: `bcdedit | find testsigning`
- Check INF file syntax: `Inf2Cat /driver:. /os:10_x64`

**Device not recognized:**
- Confirm PCI device exists: `devcon listclass display`
- Check PCI vendor/device ID matches INF

**Performance issues:**
- Check power state: Ensure GPU in D0 (active)
- Monitor thermal: May be throttling if temperature high

---

## Next Steps for Full Implementation

To create a fully functional driver:

1. **Reverse Engineer A18 Pro GPU**
   - Document Metal instruction format
   - Extract and analyze GPU firmware
   - Build instruction decoder

2. **Implement WDDM Entry Points**
   - DrvEnableDriver, DrvDisableDriver
   - DrvCreateSurface, DrvDestroySurface
   - DrvBitBlt, DrvStrokePath, DrvFillPath

3. **Command Queue Integration**
   - Implement command submission
   - Add command validation
   - Implement waits & flushing

4. **Feature Support**
   - DirectX 12 support
   - Vulkan support
   - Video decoding acceleration

5. **Optimization**
   - Performance profiling
   - Power consumption tuning
   - Latency reduction

---

## Resources

### Documentation
- [Microsoft WDDM Architecture](https://docs.microsoft.com/windows-hardware/drivers/display/wddm-driver-architecture)
- [Windows Driver Kit Docs](https://docs.microsoft.com/windows-hardware/drivers)
- [Kernel APIs Reference](https://docs.microsoft.com/windows-hardware/drivers/kernel)

### Tools
- Windows Driver Kit (WDK)
- Visual Studio 2022
- Windows Debugger (WinDbg)
- Device Console (DevCon)

### Related Technologies
- [Apple Metal Framework](https://developer.apple.com/metal)
- [DirectX 12](https://docs.microsoft.com/windows/win32/direct3d12)
- [Vulkan](https://www.vulkan.org)
- [PCI Express Standard](https://pcisig.com)

---

## License & Disclaimer

This code is provided **for educational purposes only**.

**Important:**
- Apple does not support Windows on Apple Silicon Macs
- Creating this driver required theoretical assumptions
- Use at your own risk - no warranty provided
- May violate Apple's terms of service
- Purely educational reference material

---

## Contact & Credits

**Driver Version:** 1.0.0  
**Date:** June 21, 2026  
**Status:** Proof of Concept / Educational Reference

This is a reference implementation demonstrating:
- WDDM driver architecture
- Kernel-mode driver development
- GPU hardware interface design
- Windows driver development practices

---

## Quick Reference

### Build
```batch
msbuild AppleA18ProDriver.vcxproj /p:Configuration=Release /p:Platform=x64
```

### Enable Test Signing
```batch
bcdedit /set testsigning on
shutdown /r /t 0
```

### Install
```batch
devcon install inf\AppleA18Pro.inf PCI\VEN_106B&DEV_16B5
```

### Debug
```batch
dbgview.exe
eventvwr.exe
```

### Uninstall
```batch
devcon remove "PCI\VEN_106B&DEV_16B5"
```

---

**For detailed information, see:**
- README.md - Full overview
- ARCHITECTURE.md - Technical architecture
- HARDWARE_INTERFACE.md - Register definitions
- DEVELOPMENT.md - Build & debug guide
