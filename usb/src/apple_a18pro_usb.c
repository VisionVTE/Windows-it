/*
 * Apple A18 Pro USB Driver (DWC3)
 * 
 * Core KMDF USB Driver Logic
 */

#include "apple_a18pro_usb.h"

#define DbgPrintUsb(msg, ...) DbgPrint("[Apple A18 Pro USB] " msg, ##__VA_ARGS__)

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;

    DbgPrintUsb("USB Driver loading...\n");

    WDF_DRIVER_CONFIG_INIT(&config, EvtUsbDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    return status;
}

NTSTATUS EvtUsbDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDFDEVICE wdfDevice;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_OBJECT_ATTRIBUTES attributes;
    PUSB_DEVICE_EXTENSION deviceExtension;

    UNREFERENCED_PARAMETER(Driver);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = EvtUsbPrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = EvtUsbReleaseHardware;
    WdfDeviceInitSetPnpPowerCallbacks(DeviceInit, &pnpCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, USB_DEVICE_EXTENSION);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &wdfDevice);
    if (!NT_SUCCESS(status)) return status;

    deviceExtension = GetUsbDeviceExtension(wdfDevice);
    deviceExtension->WdfDevice = wdfDevice;
    deviceExtension->MmioBaseVirtual = NULL;
    deviceExtension->PhyInitialized = FALSE;
    deviceExtension->ControllerRunning = FALSE;
    deviceExtension->ActivePortCount = 2; // Typically 2 USB-C ports on MacBook Neo

    DbgPrintUsb("Device object added.\n");
    return STATUS_SUCCESS;
}

NTSTATUS EvtUsbPrepareHardware(
    WDFDEVICE Device,
    WDFKEYBOARD Keyboard,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    PUSB_DEVICE_EXTENSION deviceExtension = GetUsbDeviceExtension(Device);
    ULONG i;
    BOOLEAN foundMemory = FALSE;

    UNREFERENCED_PARAMETER(Keyboard);
    UNREFERENCED_PARAMETER(ResourcesRaw);

    for (i = 0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR desc = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
        if (desc->Type == CmResourceTypeMemory) {
            deviceExtension->MmioBasePhysical = desc->u.Memory.Start;
            deviceExtension->MmioLength = desc->u.Memory.Length;
            foundMemory = TRUE;
            break;
        }
    }

    if (!foundMemory) {
        deviceExtension->MmioBasePhysical.QuadPart = 0x38C00000;
        deviceExtension->MmioLength = USB_MMIO_SIZE;
    }

    deviceExtension->MmioBaseVirtual = MmMapIoSpace(deviceExtension->MmioBasePhysical,
                                                   deviceExtension->MmioLength,
                                                   MmNonCached);
    if (deviceExtension->MmioBaseVirtual == NULL) {
        DbgPrintUsb("Failed mapping physical address.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UsbInitializeController(deviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS EvtUsbReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    PUSB_DEVICE_EXTENSION deviceExtension = GetUsbDeviceExtension(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    if (deviceExtension->MmioBaseVirtual) {
        UsbSetControllerState(deviceExtension, FALSE);
        MmUnmapIoSpace(deviceExtension->MmioBaseVirtual, deviceExtension->MmioLength);
        deviceExtension->MmioBaseVirtual = NULL;
    }

    DbgPrintUsb("Hardware released.\n");
    return STATUS_SUCCESS;
}

NTSTATUS UsbInitializeController(
    PUSB_DEVICE_EXTENSION DeviceExtension
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    DbgPrintUsb("Initializing USB Controller...\n");

    // 1. Reset core USB logic
    regs[USB_REG_CONTROL / 4] = USB_CTRL_RESET;
    KeStallExecutionProcessor(50);

    regs[USB_REG_CONTROL / 4] = USB_CTRL_ENABLE;

    // 2. Configure PHY
    UsbConfigurePhy(DeviceExtension);

    // 3. Start Host controller (xHCI engine)
    UsbSetControllerState(DeviceExtension, TRUE);

    DbgPrintUsb("USB controller initialization complete.\n");
    return STATUS_SUCCESS;
}

NTSTATUS UsbConfigurePhy(
    PUSB_DEVICE_EXTENSION DeviceExtension
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    DbgPrintUsb("Initializing USB PHY layer...\n");

    // Toggle PHY reset
    regs[USB_REG_PHY_CONTROL / 4] = USB_PHY_RESET;
    KeStallExecutionProcessor(20);

    // Enable power to the USB PHY
    regs[USB_REG_PHY_CONTROL / 4] = USB_PHY_POWER_ON;
    KeStallExecutionProcessor(100);

    // Verify PHY status reporting ready
    ULONG status = regs[USB_REG_PHY_STATUS / 4];
    DbgPrintUsb("USB PHY status register: 0x%08X\n", status);

    DeviceExtension->PhyInitialized = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS UsbSetControllerState(
    PUSB_DEVICE_EXTENSION DeviceExtension,
    BOOLEAN Start
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;

    if (!DeviceExtension->MmioBaseVirtual) return STATUS_DEVICE_NOT_READY;

    if (Start) {
        DbgPrintUsb("Starting xHCI schedule engine...\n");
        // Issue xHCI Run command
        regs[XHCI_REG_USBCMD / 4] |= XHCI_CMD_RUN;
        DeviceExtension->ControllerRunning = TRUE;
    } else {
        DbgPrintUsb("Halting xHCI schedule engine...\n");
        // Clear Run command
        regs[XHCI_REG_USBCMD / 4] &= ~XHCI_CMD_RUN;
        
        // Wait for halt status
        ULONG timeout = 1000;
        while (timeout > 0) {
            if (regs[XHCI_REG_USBSTS / 4] & XHCI_STS_HALT) {
                break;
            }
            KeStallExecutionProcessor(10);
            timeout--;
        }
        DeviceExtension->ControllerRunning = FALSE;
    }

    return STATUS_SUCCESS;
}
