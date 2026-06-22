/*
 * Apple A18 Pro Audio Driver (MCA)
 * 
 * Driver Structures, Declarations, and Prototypes
 */

#ifndef _APPLE_A18PRO_AUDIO_H_
#define _APPLE_A18PRO_AUDIO_H_

#include <ntddk.h>
#include <wdf.h>
#include "apple_a18pro_audio_hw.h"

#define AUDIO_DRIVER_TAG               'auAP'

typedef struct _AUDIO_DEVICE_EXTENSION {
    WDFDEVICE WdfDevice;
    
    // Memory mapped registers
    PVOID MmioBaseVirtual;
    PHYSICAL_ADDRESS MmioBasePhysical;
    ULONG MmioLength;
    
    // Audio WaveRT buffer details
    PVOID WaveBufferVirtual;
    PHYSICAL_ADDRESS WaveBufferPhysical;
    ULONG WaveBufferSize;
    
    // Playback state
    BOOLEAN PlaybackActive;
    ULONG CurrentVolumeL;
    ULONG CurrentVolumeR;
} AUDIO_DEVICE_EXTENSION, *PAUDIO_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(AUDIO_DEVICE_EXTENSION, GetAudioDeviceExtension)

// Prototypes
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtAudioDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtAudioPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtAudioReleaseHardware;

NTSTATUS AudioInitializeController(PAUDIO_DEVICE_EXTENSION DeviceExtension);
NTSTATUS AudioConfigureStream(PAUDIO_DEVICE_EXTENSION DeviceExtension, PVOID BufferAddress, ULONG BufferSize);
NTSTATUS AudioSetVolume(PAUDIO_DEVICE_EXTENSION DeviceExtension, ULONG VolumeL, ULONG VolumeR);
NTSTATUS AudioSetPlaybackState(PAUDIO_DEVICE_EXTENSION DeviceExtension, BOOLEAN Play);

#endif // _APPLE_A18PRO_AUDIO_H_
