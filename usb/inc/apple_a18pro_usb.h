/*
 * Apple A18 Pro USB Driver (DWC3)
 * 
 * Driver Structures, Declarations, and Prototypes
 */

#ifndef _APPLE_A18PRO_USB_H_
#define _APPLE_A18PRO_USB_H_

#include <ntddk.h>
#include <wdf.h>
#include "apple_a18pro_usb_hw.h"

#define USB_DRIVER_TAG                 'sbUP'

typedef struct _USB_DEVICE_EXTENSION {
    WDFDEVICE WdfDevice;
    
    // Memory mapped registers
    PVOID MmioBaseVirtual;
    PHYSICAL_ADDRESS MmioBasePhysical;
    ULONG MmioLength;
    
    // Host Controller status
    BOOLEAN PhyInitialized;
    BOOLEAN ControllerRunning;
    ULONG ActivePortCount;
} USB_DEVICE_EXTENSION, *PUSB_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_DEVICE_EXTENSION, GetUsbDeviceExtension)

// Prototypes
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtUsbDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtUsbPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtUsbReleaseHardware;

NTSTATUS UsbInitializeController(PUSB_DEVICE_EXTENSION DeviceExtension);
NTSTATUS UsbConfigurePhy(PUSB_DEVICE_EXTENSION DeviceExtension);
NTSTATUS UsbSetControllerState(PUSB_DEVICE_EXTENSION DeviceExtension, BOOLEAN Start);

#endif // _APPLE_A18PRO_USB_H_
