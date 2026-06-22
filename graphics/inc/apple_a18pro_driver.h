/*
 * Apple A18 Pro GPU Windows Driver - Main Header
 * 
 * Defines the driver context, device objects, and callback function prototypes
 */

#ifndef __APPLE_A18PRO_DRIVER_H__
#define __APPLE_A18PRO_DRIVER_H__

#include <ntdef.h>
#include <ntddk.h>
#include <wdm.h>
#include <dispmprt.h>

#include "apple_a18pro_hw.h"

/* ============================================================================
 * Driver Version
 * ============================================================================ */

#define DRIVER_MAJOR_VERSION            1
#define DRIVER_MINOR_VERSION            0
#define DRIVER_BUILD_NUMBER             0
#define DRIVER_REVISION                 0

/* ============================================================================
 * Device Context Structure
 * ============================================================================ */

typedef struct _APPLE_A18PRO_DEVICE_EXTENSION {
    /* Device identification */
    PHYSICAL_ADDRESS PhysicalMemoryBase;
    ULONG PhysicalMemoryLength;
    PVOID VirtualMemoryBase;
    
    /* Hardware info */
    GPU_DEVICE_INFO HwInfo;
    GPU_CAPABILITIES Capabilities;
    
    /* Register mapping */
    PVOID ControlRegs;
    PVOID PowerRegs;
    PVOID MemoryRegs;
    PVOID DisplayRegs;
    PVOID ShaderRegs;
    
    /* Current display state */
    ULONG CurrentWidth;
    ULONG CurrentHeight;
    ULONG CurrentBitsPerPixel;
    ULONG CurrentRefreshRate;
    
    /* Framebuffer information */
    PHYSICAL_ADDRESS FramebufferPhysical;
    PVOID FramebufferVirtual;
    ULONG FramebufferSize;
    
    /* Power state */
    DEVICE_POWER_STATE PowerState;
    SYSTEM_POWER_STATE SystemPowerState;
    
    /* Interrupt handling */
    PKINTERRUPT InterruptObject;
    BOOLEAN InterruptConnected;
    
    /* Synchronization */
    KSPIN_LOCK DeviceLock;
    KEVENT HardwareInitialized;
    
    /* Thermal management */
    ULONG CurrentTemperature;
    ULONG TemperatureLimit;
    
} APPLE_A18PRO_DEVICE_EXTENSION, *PAPPLE_A18PRO_DEVICE_EXTENSION;

/* ============================================================================
 * Display Information Structure
 * ============================================================================ */

typedef struct _DISPLAY_MODE_INFO {
    ULONG Width;
    ULONG Height;
    ULONG RefreshRate;
    ULONG BitsPerPixel;
    ULONG BytesPerPixel;
    ULONG PixelClock;
} DISPLAY_MODE_INFO, *PDISPLAY_MODE_INFO;

/* ============================================================================
 * Function Declarations
 * ============================================================================ */

/* Driver Entry and Cleanup */
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
);

VOID DriverUnload(
    PDRIVER_OBJECT DriverObject
);

/* Device Initialization */
NTSTATUS AppleA18ProInitialize(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS AppleA18ProQueryCapabilities(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PGPU_CAPABILITIES Capabilities
);

/* Hardware Control */
NTSTATUS AppleA18ProEnableDevice(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS AppleA18ProDisableDevice(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS AppleA18ProResetDevice(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

/* Display Mode Handling */
NTSTATUS AppleA18ProSetDisplayMode(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PDISPLAY_MODE_INFO DisplayMode
);

NTSTATUS AppleA18ProGetDisplayMode(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PDISPLAY_MODE_INFO DisplayMode
);

NTSTATUS AppleA18ProEnumerateDisplayModes(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG ModeNumber,
    PDISPLAY_MODE_INFO DisplayMode
);

/* Power Management */
NTSTATUS AppleA18ProSetPowerState(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    DEVICE_POWER_STATE PowerState
);

DEVICE_POWER_STATE AppleA18ProGetPowerState(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

/* Interrupt Handling */
BOOLEAN AppleA18ProInterruptHandler(
    PKINTERRUPT Interrupt,
    PVOID ServiceContext
);

NTSTATUS AppleA18ProEnableInterrupts(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS AppleA18ProDisableInterrupts(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

/* Memory Management */
NTSTATUS AppleA18ProMapMemory(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

VOID AppleA18ProUnmapMemory(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

NTSTATUS AppleA18ProAllocateFramebuffer(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG Width,
    ULONG Height,
    ULONG BitsPerPixel
);

VOID AppleA18ProFreeFramebuffer(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

/* Thermal Management */
NTSTATUS AppleA18ProReadTemperature(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PULONG Temperature
);

NTSTATUS AppleA18ProSetThermalLimit(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG TemperatureLimit
);

/* Utility Functions */
NTSTATUS AppleA18ProWaitForHardware(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG TimeoutMs
);

VOID AppleA18ProFlushCache(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
);

#endif /* __APPLE_A18PRO_DRIVER_H__ */
