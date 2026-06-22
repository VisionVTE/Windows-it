# Apple A18 Pro Driver Suite for Windows
## MacBook Neo (Mac17,5) Complete SoC Driver Package

**Version:** 1.0.0  
**Date:** June 21, 2026  
**Status:** Proof of Concept / Educational Reference Implementation  

---

## Executive Summary

This repository contains a complete theoretical driver suite for running Windows on the Apple A18 Pro SoC (System on Chip) found in the MacBook Neo (Mac17,5). The project is divided into six core kernel-mode components:

| # | Driver | Class | Description |
|---|--------|-------|-------------|
| 1 | **Graphics (WDDM)** | Display | GPU control, rendering, thermal throttling, resolution enumeration |
| 2 | **Chipset (KMDF)** | System | Power Manager (PMGR), Interrupt Controller (AIC), GPIO |
| 3 | **Storage (KMDF)** | SCSIAdapter | NVM Storage (ANS) controller, DMA block transfers |
| 4 | **Audio (KMDF)** | MEDIA | Multichannel Audio (MCA), DMA ring buffers, DAC volume |
| 5 | **USB (KMDF)** | USB | DWC3 xHCI host controller, USB-C PHY initialization |
| 6 | **Input (KMDF)** | HIDClass | SPI keyboard/trackpad, HID report generation |

> [!IMPORTANT]
> **Educational Reference Disclaimer:** This code serves as an architectural blueprint demonstrating modern driver design. Running this on actual Apple Silicon Macs requires proprietary firmware, secure boot bypasses, and custom virtualization layers that are not part of this repository.

---

## Directory Structure

```
Apple_A18Pro_Driver/
├── README.md                           # This unified suite guide
├── DEVELOPMENT.md                      # Build, install, and debug manual
├── ARCHITECTURE.md                     # Technical architecture details
├── HARDWARE_INTERFACE.md               # Hardware registers reference map
│
├── graphics/                           # Graphics Driver (WDDM Display)
│   ├── inc/                            # Headers
│   ├── src/                            # Source
│   ├── inf/                            # INF
│   ├── installer/                      # Scripts & Inno Setup
│   └── AppleA18ProDriver.vcxproj
│
├── chipset/                            # Chipset Driver (PMGR / AIC / GPIO)
│   ├── inc/                            # Headers
│   ├── src/                            # Source
│   ├── inf/                            # INF
│   ├── installer/                      # Scripts
│   └── AppleA18ProChipset.vcxproj
│
├── storage/                            # Storage Driver (ANS NVMe)
│   ├── inc/                            # Headers
│   ├── src/                            # Source
│   ├── inf/                            # INF
│   ├── installer/                      # Scripts
│   └── AppleA18ProStorage.vcxproj
│
├── audio/                              # Audio Driver (MCA)
│   ├── inc/                            # Headers
│   ├── src/                            # Source
│   ├── inf/                            # INF
│   ├── installer/                      # Scripts
│   └── AppleA18ProAudio.vcxproj
│
├── usb/                                # USB Driver (DWC3 xHCI)
│   ├── inc/                            # Headers
│   ├── src/                            # Source
│   ├── inf/                            # INF
│   ├── installer/                      # Scripts
│   └── AppleA18ProUsb.vcxproj
│
└── input/                              # Input Driver (SPI HID)
    ├── inc/                            # Headers
    ├── src/                            # Source
    ├── inf/                            # INF
    ├── installer/                      # Scripts
    └── AppleA18ProInput.vcxproj
```

---

## Driver Specifications

### 1. Graphics Driver (WDDM)
- **PCI HW ID**: `PCI\VEN_106B&DEV_16B5`
- **Binary**: `AppleA18ProDisplay.dll`
- **Features**: GPU state reset, 10 resolution presets, D0-D3 power management, VSync interrupts, temperature monitoring

### 2. Chipset Driver (KMDF)
- **PCI HW ID**: `PCI\VEN_106B&DEV_16B0`
- **Binary**: `AppleA18ProChipset.sys`
- **Features**: Clock gating, voltage scaling, AIC interrupt routing, 32-pin GPIO control

### 3. Storage Driver (KMDF)
- **PCI HW ID**: `PCI\VEN_106B&DEV_16B1`
- **Binary**: `AppleA18ProStorage.sys`
- **Features**: ANS controller reset/enable, NVMe command queue submission, DMA read/write transfers

### 4. Audio Driver (KMDF)
- **PCI HW ID**: `PCI\VEN_106B&DEV_16B2`
- **Binary**: `AppleA18ProAudio.sys`
- **Features**: MCA serial interface, TX/RX DMA ring buffers, stereo DAC volume control, playback start/stop

### 5. USB Driver (KMDF)
- **PCI HW ID**: `PCI\VEN_106B&DEV_16B3`
- **Binary**: `AppleA18ProUsb.sys`
- **Features**: DWC3 core initialization, USB PHY reset/power-on, xHCI run/halt control

### 6. Input Driver (KMDF)
- **PCI HW ID**: `PCI\VEN_106B&DEV_16B4`
- **Binary**: `AppleA18ProInput.sys`
- **Features**: SPI master mode, FIFO-based packet reading, keyboard HID reports (6KRO), trackpad relative motion reports

---

## Hardware Memory Map

| Controller | PCI Device ID | MMIO Base | Size | Description |
|------------|---------------|-----------|------|-------------|
| **PMGR** | `16B0` | `0x30E000000` | 64KB | Power Management |
| **AIC** | `16B0` | `0x30B00000` | 64KB | Interrupt Controller |
| **GPIO** | `16B0` | `0x30C00000` | 64KB | General Purpose I/O |
| **GPU** | `16B5` | `0x30D000000` | 256KB | Graphics Registers |
| **ANS** | `16B1` | `0x38F000000` | 64KB | NVM Storage |
| **MCA** | `16B2` | `0x38E00000` | 64KB | Multichannel Audio |
| **DWC3** | `16B3` | `0x38C00000` | 64KB | USB Controller |
| **SPI** | `16B4` | `0x38A00000` | 64KB | Keyboard/Trackpad |

---

## Getting Started

To compile and test the drivers, please refer to **[DEVELOPMENT.md](file:///Users/visionvt/Documents/graphics%20driver/Apple_A18Pro_Driver/DEVELOPMENT.md)**.

Each driver includes automated command-line installers that handle:
- Test-signing mode activation
- Self-signed certificate generation and trust enrollment
- Security catalog compilation (via WDK `Inf2Cat`)
- Binary signing
- Driver store registration (via `pnputil`)
