/*
 * Apple A18 Pro HID Input Driver (SPI HID)
 * 
 * Core KMDF Input Driver Logic - Keyboard & Trackpad
 */

#include "apple_a18pro_input.h"

#define DbgPrintInput(msg, ...) DbgPrint("[Apple A18 Pro Input] " msg, ##__VA_ARGS__)

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;

    DbgPrintInput("Input Driver loading...\n");

    WDF_DRIVER_CONFIG_INIT(&config, EvtInputDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    return status;
}

NTSTATUS EvtInputDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDFDEVICE wdfDevice;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_OBJECT_ATTRIBUTES attributes;
    PINPUT_DEVICE_EXTENSION deviceExtension;

    UNREFERENCED_PARAMETER(Driver);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = EvtInputPrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = EvtInputReleaseHardware;
    WdfDeviceInitSetPnpPowerCallbacks(DeviceInit, &pnpCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, INPUT_DEVICE_EXTENSION);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &wdfDevice);
    if (!NT_SUCCESS(status)) return status;

    deviceExtension = GetInputDeviceExtension(wdfDevice);
    deviceExtension->WdfDevice = wdfDevice;
    deviceExtension->MmioBaseVirtual = NULL;
    deviceExtension->ControllerReady = FALSE;

    // Zero out last known reports
    RtlZeroMemory(&deviceExtension->LastKeyboardReport, sizeof(KEYBOARD_REPORT));
    RtlZeroMemory(&deviceExtension->LastTrackpadReport, sizeof(TRACKPAD_REPORT));

    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &deviceExtension->FifoLock);
    if (!NT_SUCCESS(status)) return status;

    DbgPrintInput("Device object added.\n");
    return STATUS_SUCCESS;
}

NTSTATUS EvtInputPrepareHardware(
    WDFDEVICE Device,
    WDFKEYBOARD Keyboard,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    PINPUT_DEVICE_EXTENSION deviceExtension = GetInputDeviceExtension(Device);
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
        deviceExtension->MmioBasePhysical.QuadPart = 0x38A00000;
        deviceExtension->MmioLength = SPI_MMIO_SIZE;
    }

    deviceExtension->MmioBaseVirtual = MmMapIoSpace(deviceExtension->MmioBasePhysical,
                                                   deviceExtension->MmioLength,
                                                   MmNonCached);
    if (deviceExtension->MmioBaseVirtual == NULL) {
        DbgPrintInput("Failed mapping physical address.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InputInitializeController(deviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS EvtInputReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    PINPUT_DEVICE_EXTENSION deviceExtension = GetInputDeviceExtension(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    if (deviceExtension->MmioBaseVirtual) {
        MmUnmapIoSpace(deviceExtension->MmioBaseVirtual, deviceExtension->MmioLength);
        deviceExtension->MmioBaseVirtual = NULL;
    }

    DbgPrintInput("Hardware released.\n");
    return STATUS_SUCCESS;
}

NTSTATUS InputInitializeController(
    PINPUT_DEVICE_EXTENSION DeviceExtension
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    DbgPrintInput("Initializing SPI Controller for Keyboard/Trackpad...\n");

    // 1. Reset SPI engine
    regs[SPI_REG_CONTROL / 4] = SPI_CTRL_RESET;
    KeStallExecutionProcessor(50);

    // 2. Enable SPI in master mode
    regs[SPI_REG_CONTROL / 4] = SPI_CTRL_ENABLE | SPI_CTRL_MASTER_MODE;

    // 3. Set SPI clock divider (target ~8 MHz for Apple SPI HID)
    regs[SPI_REG_CLOCK_DIVIDER / 4] = 0x10;

    // 4. Configure FIFO thresholds
    regs[SPI_REG_FIFO_CONTROL / 4] = 0x08; // Trigger interrupt when 8 bytes in RX FIFO

    // 5. Enable RX threshold interrupt
    regs[SPI_REG_INT_STATUS / 4] = 0xFFFFFFFF; // Clear pending
    regs[SPI_REG_INT_ENABLE / 4] = SPI_INT_RX_THRESHOLD | SPI_INT_ERROR;

    DeviceExtension->ControllerReady = TRUE;
    DbgPrintInput("SPI controller initialized - ready for input events.\n");

    return STATUS_SUCCESS;
}

NTSTATUS InputReadSpiPacket(
    PINPUT_DEVICE_EXTENSION DeviceExtension,
    PSPI_PACKET Packet
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    ULONG fifoStatus;
    ULONG bytesRead = 0;
    PUCHAR rawBuffer = (PUCHAR)Packet;

    if (!DeviceExtension->ControllerReady) return STATUS_DEVICE_NOT_READY;

    WdfSpinLockAcquire(DeviceExtension->FifoLock);

    // Check if RX FIFO has data available
    fifoStatus = regs[SPI_REG_FIFO_STATUS / 4];
    if (fifoStatus & SPI_STATUS_RX_EMPTY) {
        WdfSpinLockRelease(DeviceExtension->FifoLock);
        return STATUS_NO_MORE_ENTRIES;
    }

    // Read raw bytes from FIFO into the packet buffer
    while (bytesRead < sizeof(SPI_PACKET)) {
        fifoStatus = regs[SPI_REG_FIFO_STATUS / 4];
        if (fifoStatus & SPI_STATUS_RX_EMPTY) {
            break;
        }

        ULONG word = regs[SPI_REG_RX_DATA / 4];
        
        // Unpack 32-bit word into byte buffer
        if (bytesRead < sizeof(SPI_PACKET))     rawBuffer[bytesRead++] = (UCHAR)(word & 0xFF);
        if (bytesRead < sizeof(SPI_PACKET))     rawBuffer[bytesRead++] = (UCHAR)((word >> 8) & 0xFF);
        if (bytesRead < sizeof(SPI_PACKET))     rawBuffer[bytesRead++] = (UCHAR)((word >> 16) & 0xFF);
        if (bytesRead < sizeof(SPI_PACKET))     rawBuffer[bytesRead++] = (UCHAR)((word >> 24) & 0xFF);
    }

    WdfSpinLockRelease(DeviceExtension->FifoLock);

    // Validate magic header
    if (Packet->Magic != SPI_PACKET_HEADER_MAGIC) {
        DbgPrintInput("Invalid SPI packet magic: 0x%04X\n", Packet->Magic);
        return STATUS_DATA_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS InputProcessPacket(
    PINPUT_DEVICE_EXTENSION DeviceExtension,
    PSPI_PACKET Packet
)
{
    switch (Packet->DeviceId) {
        case HID_REPORT_ID_KEYBOARD: {
            if (Packet->PayloadLength >= sizeof(KEYBOARD_REPORT)) {
                PKEYBOARD_REPORT kbReport = (PKEYBOARD_REPORT)Packet->Payload;
                
                RtlCopyMemory(&DeviceExtension->LastKeyboardReport,
                              kbReport,
                              sizeof(KEYBOARD_REPORT));

                DbgPrintInput("Keyboard: Modifiers=0x%02X Keys=[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]\n",
                              kbReport->Modifiers,
                              kbReport->KeyCodes[0], kbReport->KeyCodes[1],
                              kbReport->KeyCodes[2], kbReport->KeyCodes[3],
                              kbReport->KeyCodes[4], kbReport->KeyCodes[5]);
            }
            break;
        }

        case HID_REPORT_ID_TRACKPAD: {
            if (Packet->PayloadLength >= sizeof(TRACKPAD_REPORT)) {
                PTRACKPAD_REPORT tpReport = (PTRACKPAD_REPORT)Packet->Payload;

                RtlCopyMemory(&DeviceExtension->LastTrackpadReport,
                              tpReport,
                              sizeof(TRACKPAD_REPORT));

                DbgPrintInput("Trackpad: Btn=0x%02X dX=%d dY=%d Pressure=%d\n",
                              tpReport->ButtonState,
                              tpReport->DeltaX,
                              tpReport->DeltaY,
                              tpReport->Pressure);
            }
            break;
        }

        default:
            DbgPrintInput("Unknown SPI device ID: 0x%02X\n", Packet->DeviceId);
            return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}
