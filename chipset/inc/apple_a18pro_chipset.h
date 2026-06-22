/*
 * Apple A18 Pro Chipset / System Controller Driver
 * 
 * Driver Structures, Declarations, and Prototypes
 */

#ifndef _APPLE_A18PRO_CHIPSET_H_
#define _APPLE_A18PRO_CHIPSET_H_

#include <ntddk.h>
#include <wdf.h>
#include "apple_a18pro_chipset_hw.h"

// Version Definitions
#define CHIPSET_DRIVER_MAJOR           1
#define CHIPSET_DRIVER_MINOR           0
#define CHIPSET_DRIVER_BUILD           0
#define CHIPSET_DRIVER_REVISION        0

// Device extension memory tag
#define A18PRO_CHIPSET_TAG             'pc1A'

// Chipset Power States
typedef enum _CHIPSET_POWER_STATE {
    ChipsetPowerActive = 0,            // Normal operation (D0)
    ChipsetPowerStandby = 1,           // Idle state (D1/D2)
    ChipsetPowerSleep = 2              // Deep sleep state (D3)
} CHIPSET_POWER_STATE;

// Driver context structure stored on WDFDEVICE
typedef struct _CHIPSET_DEVICE_EXTENSION {
    WDFDEVICE WdfDevice;               // WDF Device handle
    
    // Memory mapped registers
    PVOID MmioBaseVirtual;             // Virtual address of mapped MMIO space
    PHYSICAL_ADDRESS MmioBasePhysical; // Physical address of MMIO region
    ULONG MmioLength;                  // Length of MMIO space (192KB)
    
    // Mapped Subsystem Virtual Addresses
    volatile ULONG* PmgrBase;          // PMGR Virtual Address
    volatile ULONG* AicBase;           // AIC Virtual Address
    volatile ULONG* GpioBase;          // GPIO Virtual Address
    
    // Synchronization locks
    WDFSPINLOCK RegisterLock;          // Spinlock for thread-safe register writes
    
    // State Tracking
    CHIPSET_POWER_STATE PowerState;    // Current power state
    BOOLEAN GpuPowerEnabled;           // Is GPU domain powered on
    ULONG ActiveInterruptMask[4];      // Track enabled interrupts in AIC (128 bits)
    ULONG GpioDirectionMask;           // Direction of 32 GPIO pins (1 = output)
} CHIPSET_DEVICE_EXTENSION, *PCHIPSET_DEVICE_EXTENSION;

// Retrieve the context from WDFDEVICE
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CHIPSET_DEVICE_EXTENSION, GetDeviceExtension)

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

// 1. WDF Driver Lifecycle
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtChipsetDeviceAdd;

// 2. Hardware Resource Management
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtChipsetPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtChipsetReleaseHardware;

// 3. Power Management State Events
EVT_WDF_DEVICE_D0_ENTRY EvtChipsetD0Entry;
EVT_WDF_DEVICE_D0_EXIT EvtChipsetD0Exit;

// 4. Subsystem Management Functions

// Thread-safe register IO functions
ULONG ChipsetReadRegister(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Offset);
VOID ChipsetWriteRegister(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Offset, ULONG Value);

// PMGR Subsystem
NTSTATUS ChipsetInitializePmgr(PCHIPSET_DEVICE_EXTENSION DeviceExtension);
NTSTATUS ChipsetSetGpuPower(PCHIPSET_DEVICE_EXTENSION DeviceExtension, BOOLEAN Enable);
NTSTATUS ChipsetConfigurePowerState(PCHIPSET_DEVICE_EXTENSION DeviceExtension, CHIPSET_POWER_STATE State);

// AIC Subsystem
NTSTATUS ChipsetInitializeAic(PCHIPSET_DEVICE_EXTENSION DeviceExtension);
NTSTATUS ChipsetConfigureInterrupt(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Vector, BOOLEAN Enable, ULONG Affinity);
NTSTATUS ChipsetClearInterrupt(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Vector);

// GPIO Subsystem
NTSTATUS ChipsetConfigureGpio(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Pin, ULONG Direction, ULONG PullState);
NTSTATUS ChipsetSetGpioPin(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Pin, BOOLEAN Value);
BOOLEAN ChipsetGetGpioPin(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Pin);
NTSTATUS ChipsetConfigureGpioInterrupt(PCHIPSET_DEVICE_EXTENSION DeviceExtension, ULONG Pin, BOOLEAN Enable, ULONG TriggerMode);

#endif // _APPLE_A18PRO_CHIPSET_H_
