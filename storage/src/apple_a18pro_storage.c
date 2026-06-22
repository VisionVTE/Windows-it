/*
 * Apple A18 Pro Storage Driver (ANS)
 * 
 * Core KMDF Storage Driver Logic
 */

#include "apple_a18pro_storage.h"

#define DbgPrintStorage(msg, ...) DbgPrint("[Apple A18 Pro Storage] " msg, ##__VA_ARGS__)

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;

    DbgPrintStorage("Storage Driver loading...\n");

    WDF_DRIVER_CONFIG_INIT(&config, EvtStorageDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    return status;
}

NTSTATUS EvtStorageDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDFDEVICE wdfDevice;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_OBJECT_ATTRIBUTES attributes;
    PSTORAGE_DEVICE_EXTENSION deviceExtension;

    UNREFERENCED_PARAMETER(Driver);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = EvtStoragePrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = EvtStorageReleaseHardware;
    WdfDeviceInitSetPnpPowerCallbacks(DeviceInit, &pnpCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, STORAGE_DEVICE_EXTENSION);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &wdfDevice);
    if (!NT_SUCCESS(status)) return status;

    deviceExtension = GetStorageDeviceExtension(wdfDevice);
    deviceExtension->WdfDevice = wdfDevice;
    deviceExtension->MmioBaseVirtual = NULL;
    deviceExtension->ControllerReady = FALSE;
    deviceExtension->ActiveTransfers = 0;

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &deviceExtension->IoLock);

    DbgPrintStorage("Device object added.\n");
    return status;
}

NTSTATUS EvtStoragePrepareHardware(
    WDFDEVICE Device,
    WDFKEYBOARD Keyboard,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    PSTORAGE_DEVICE_EXTENSION deviceExtension = GetStorageDeviceExtension(Device);
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
        deviceExtension->MmioBasePhysical.QuadPart = 0x38F000000;
        deviceExtension->MmioLength = ANS_MMIO_SIZE;
    }

    deviceExtension->MmioBaseVirtual = MmMapIoSpace(deviceExtension->MmioBasePhysical,
                                                   deviceExtension->MmioLength,
                                                   MmNonCached);
    if (deviceExtension->MmioBaseVirtual == NULL) {
        DbgPrintStorage("Failed mapping physical address.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    StorageInitializeController(deviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS EvtStorageReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    PSTORAGE_DEVICE_EXTENSION deviceExtension = GetStorageDeviceExtension(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    if (deviceExtension->MmioBaseVirtual) {
        MmUnmapIoSpace(deviceExtension->MmioBaseVirtual, deviceExtension->MmioLength);
        deviceExtension->MmioBaseVirtual = NULL;
    }

    DbgPrintStorage("Hardware released.\n");
    return STATUS_SUCCESS;
}

NTSTATUS StorageInitializeController(
    PSTORAGE_DEVICE_EXTENSION DeviceExtension
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    DbgPrintStorage("Initializing ANS Controller...\n");

    // 1. Reset controller
    regs[ANS_REG_CONTROL / 4] = ANS_CTRL_RESET;
    KeStallExecutionProcessor(50);

    // 2. Enable controller
    regs[ANS_REG_CONTROL / 4] = ANS_CTRL_ENABLE;

    // 3. Clear pending interrupts
    regs[ANS_REG_INT_STATUS / 4] = 0xFFFFFFFF;
    regs[ANS_REG_INT_ENABLE / 4] = ANS_INT_CMD_COMPLETE | ANS_INT_DMA_COMPLETE | ANS_INT_ERROR;

    DeviceExtension->ControllerReady = TRUE;
    DbgPrintStorage("ANS controller ready.\n");

    return STATUS_SUCCESS;
}

NTSTATUS StorageExecuteDmaTransfer(
    PSTORAGE_DEVICE_EXTENSION DeviceExtension,
    PHYSICAL_ADDRESS SystemMemoryAddr,
    ULONG Length,
    BOOLEAN WriteToFlash
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    ULONG dmaCtrl = ANS_DMA_START;

    if (!DeviceExtension->ControllerReady) return STATUS_DEVICE_NOT_READY;

    WdfSpinLockAcquire(DeviceExtension->IoLock);

    DeviceExtension->ActiveTransfers++;

    // Write physical system memory addresses to DMA registers
    regs[ANS_REG_DMA_PHYS_ADDR_L / 4] = SystemMemoryAddr.LowPart;
    regs[ANS_REG_DMA_PHYS_ADDR_H / 4] = SystemMemoryAddr.HighPart;
    regs[ANS_REG_DMA_LENGTH / 4] = Length;

    if (WriteToFlash) {
        dmaCtrl |= ANS_DMA_DIR_WRITE;
    }

    DbgPrintStorage("Triggering DMA transfer (Len: %d bytes, Direction: %s)\n", 
                    Length, WriteToFlash ? "WRITE" : "READ");

    regs[ANS_REG_DMA_CONTROL / 4] = dmaCtrl;

    // Wait for transfer complete (emulated polling loop)
    ULONG timeout = 5000;
    while (timeout > 0) {
        ULONG status = regs[ANS_REG_INT_STATUS / 4];
        if (status & ANS_INT_DMA_COMPLETE) {
            regs[ANS_REG_INT_STATUS / 4] = ANS_INT_DMA_COMPLETE; // Clear flag
            break;
        }
        KeStallExecutionProcessor(10);
        timeout--;
    }

    DeviceExtension->ActiveTransfers--;

    WdfSpinLockRelease(DeviceExtension->IoLock);

    return STATUS_SUCCESS;
}
