/*
 * Apple A18 Pro Audio Driver (MCA)
 * 
 * Core KMDF Audio Driver Logic
 */

#include "apple_a18pro_audio.h"

#define DbgPrintAudio(msg, ...) DbgPrint("[Apple A18 Pro Audio] " msg, ##__VA_ARGS__)

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;

    DbgPrintAudio("Audio Driver loading...\n");

    WDF_DRIVER_CONFIG_INIT(&config, EvtAudioDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE);

    return status;
}

NTSTATUS EvtAudioDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDFDEVICE wdfDevice;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
    WDF_OBJECT_ATTRIBUTES attributes;
    PAUDIO_DEVICE_EXTENSION deviceExtension;

    UNREFERENCED_PARAMETER(Driver);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDevicePrepareHardware = EvtAudioPrepareHardware;
    pnpCallbacks.EvtDeviceReleaseHardware = EvtAudioReleaseHardware;
    WdfDeviceInitSetPnpPowerCallbacks(DeviceInit, &pnpCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, AUDIO_DEVICE_EXTENSION);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &wdfDevice);
    if (!NT_SUCCESS(status)) return status;

    deviceExtension = GetAudioDeviceExtension(wdfDevice);
    deviceExtension->WdfDevice = wdfDevice;
    deviceExtension->MmioBaseVirtual = NULL;
    deviceExtension->WaveBufferVirtual = NULL;
    deviceExtension->PlaybackActive = FALSE;
    deviceExtension->CurrentVolumeL = 0x7F;
    deviceExtension->CurrentVolumeR = 0x7F;

    DbgPrintAudio("Device object added.\n");
    return STATUS_SUCCESS;
}

NTSTATUS EvtAudioPrepareHardware(
    WDFDEVICE Device,
    WDFKEYBOARD Keyboard,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated
)
{
    PAUDIO_DEVICE_EXTENSION deviceExtension = GetAudioDeviceExtension(Device);
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
        deviceExtension->MmioBasePhysical.QuadPart = 0x38E00000;
        deviceExtension->MmioLength = MCA_MMIO_SIZE;
    }

    deviceExtension->MmioBaseVirtual = MmMapIoSpace(deviceExtension->MmioBasePhysical,
                                                   deviceExtension->MmioLength,
                                                   MmNonCached);
    if (deviceExtension->MmioBaseVirtual == NULL) {
        DbgPrintAudio("Failed mapping physical address.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    AudioInitializeController(deviceExtension);

    return STATUS_SUCCESS;
}

NTSTATUS EvtAudioReleaseHardware(
    WDFDEVICE Device,
    WDFCMRESLIST ResourcesTranslated
)
{
    PAUDIO_DEVICE_EXTENSION deviceExtension = GetAudioDeviceExtension(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    if (deviceExtension->MmioBaseVirtual) {
        MmUnmapIoSpace(deviceExtension->MmioBaseVirtual, deviceExtension->MmioLength);
        deviceExtension->MmioBaseVirtual = NULL;
    }

    DbgPrintAudio("Hardware released.\n");
    return STATUS_SUCCESS;
}

NTSTATUS AudioInitializeController(
    PAUDIO_DEVICE_EXTENSION DeviceExtension
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    DbgPrintAudio("Initializing MCA Controller...\n");

    // 1. Reset controller
    regs[MCA_REG_CONTROL / 4] = MCA_CTRL_RESET;
    KeStallExecutionProcessor(50);

    // 2. Enable controller
    regs[MCA_REG_CONTROL / 4] = MCA_CTRL_ENABLE;

    // 3. Set default volume
    AudioSetVolume(DeviceExtension, DeviceExtension->CurrentVolumeL, DeviceExtension->CurrentVolumeR);

    DbgPrintAudio("MCA controller initialized.\n");
    return STATUS_SUCCESS;
}

NTSTATUS AudioConfigureStream(
    PAUDIO_DEVICE_EXTENSION DeviceExtension,
    PVOID BufferAddress,
    ULONG BufferSize
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;
    PHYSICAL_ADDRESS bufferPhys;

    DbgPrintAudio("Configuring Audio Stream buffer...\n");

    DeviceExtension->WaveBufferVirtual = BufferAddress;
    DeviceExtension->WaveBufferSize = BufferSize;

    // Get physical address of memory buffer
    bufferPhys = MmGetPhysicalAddress(BufferAddress);

    // Write buffer addresses to DMA channels
    regs[MCA_REG_DMA_TX_ADDR_L / 4] = bufferPhys.LowPart;
    regs[MCA_REG_DMA_TX_ADDR_H / 4] = bufferPhys.HighPart;
    regs[MCA_REG_DMA_TX_SIZE / 4] = BufferSize;

    return STATUS_SUCCESS;
}

NTSTATUS AudioSetVolume(
    PAUDIO_DEVICE_EXTENSION DeviceExtension,
    ULONG VolumeL,
    ULONG VolumeR
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;

    DeviceExtension->CurrentVolumeL = VolumeL;
    DeviceExtension->CurrentVolumeR = VolumeR;

    if (DeviceExtension->MmioBaseVirtual) {
        regs[MCA_REG_DAC_VOLUME_L / 4] = VolumeL;
        regs[MCA_REG_DAC_VOLUME_R / 4] = VolumeR;
        DbgPrintAudio("Volume set - Left: %d, Right: %d\n", VolumeL, VolumeR);
    }

    return STATUS_SUCCESS;
}

NTSTATUS AudioSetPlaybackState(
    PAUDIO_DEVICE_EXTENSION DeviceExtension,
    BOOLEAN Play
)
{
    volatile ULONG* regs = (volatile ULONG*)DeviceExtension->MmioBaseVirtual;

    if (!DeviceExtension->MmioBaseVirtual) return STATUS_DEVICE_NOT_READY;

    if (Play) {
        regs[MCA_REG_TX_CONTROL / 4] |= MCA_TX_ENABLE;
        regs[MCA_REG_CONTROL / 4] |= MCA_DMA_TX_START;
        DeviceExtension->PlaybackActive = TRUE;
        DbgPrintAudio("Audio playback started.\n");
    } else {
        regs[MCA_REG_CONTROL / 4] &= ~MCA_DMA_TX_START;
        regs[MCA_REG_TX_CONTROL / 4] &= ~MCA_TX_ENABLE;
        DeviceExtension->PlaybackActive = FALSE;
        DbgPrintAudio("Audio playback stopped.\n");
    }

    return STATUS_SUCCESS;
}
