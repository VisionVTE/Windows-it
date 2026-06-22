/*
 * Apple A18 Pro Storage Driver (ANS)
 * 
 * Driver Structures, Contexts, and Prototypes
 */

#ifndef _APPLE_A18PRO_STORAGE_H_
#define _APPLE_A18PRO_STORAGE_H_

#include <ntddk.h>
#include <wdf.h>
#include "apple_a18pro_storage_hw.h"

#define STORAGE_DRIVER_TAG             'tsAP'

typedef struct _STORAGE_DEVICE_EXTENSION {
    WDFDEVICE WdfDevice;
    
    // Memory mapped registers
    PVOID MmioBaseVirtual;
    PHYSICAL_ADDRESS MmioBasePhysical;
    ULONG MmioLength;
    
    // Command queues structures (simulated rings)
    PVOID QueueBufferVirtual;
    PHYSICAL_ADDRESS QueueBufferPhysical;
    
    // Locks
    WDFSPINLOCK IoLock;
    
    // Transfer Tracking
    BOOLEAN ControllerReady;
    ULONG ActiveTransfers;
} STORAGE_DEVICE_EXTENSION, *PSTORAGE_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(STORAGE_DEVICE_EXTENSION, GetStorageDeviceExtension)

// Prototypes
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtStorageDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtStoragePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtStorageReleaseHardware;

NTSTATUS StorageInitializeController(PSTORAGE_DEVICE_EXTENSION DeviceExtension);
NTSTATUS StorageExecuteDmaTransfer(PSTORAGE_DEVICE_EXTENSION DeviceExtension, PHYSICAL_ADDRESS SystemMemoryAddr, ULONG Length, BOOLEAN WriteToFlash);

#endif // _APPLE_A18PRO_STORAGE_H_
