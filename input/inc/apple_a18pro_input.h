/*
 * Apple A18 Pro HID Input Driver (SPI HID)
 * 
 * Driver Structures, Declarations, and Prototypes
 */

#ifndef _APPLE_A18PRO_INPUT_H_
#define _APPLE_A18PRO_INPUT_H_

#include <ntddk.h>
#include <wdf.h>
#include "apple_a18pro_input_hw.h"

#define INPUT_DRIVER_TAG               'inAP'

// HID Report Types
#define HID_REPORT_ID_KEYBOARD         0x01
#define HID_REPORT_ID_TRACKPAD         0x02

// Keyboard report structure (8 bytes standard USB HID)
typedef struct _KEYBOARD_REPORT {
    UCHAR Modifiers;       // Ctrl/Shift/Alt/GUI bitmask
    UCHAR Reserved;
    UCHAR KeyCodes[6];     // Up to 6 simultaneous key presses
} KEYBOARD_REPORT, *PKEYBOARD_REPORT;

// Trackpad report structure
typedef struct _TRACKPAD_REPORT {
    UCHAR ButtonState;     // Button pressed bitmask
    SHORT DeltaX;          // Relative X movement
    SHORT DeltaY;          // Relative Y movement
    SHORT Pressure;        // Touch pressure (0-255)
} TRACKPAD_REPORT, *PTRACKPAD_REPORT;

// Raw SPI packet from the Apple keyboard/trackpad
#define SPI_PACKET_HEADER_MAGIC        0xA1B2
#define SPI_PACKET_MAX_PAYLOAD         64

typedef struct _SPI_PACKET {
    USHORT Magic;          // Should match SPI_PACKET_HEADER_MAGIC
    UCHAR DeviceId;        // 0x01 = Keyboard, 0x02 = Trackpad
    UCHAR PayloadLength;
    UCHAR Payload[SPI_PACKET_MAX_PAYLOAD];
} SPI_PACKET, *PSPI_PACKET;

// Device extension
typedef struct _INPUT_DEVICE_EXTENSION {
    WDFDEVICE WdfDevice;
    
    // Memory mapped registers
    PVOID MmioBaseVirtual;
    PHYSICAL_ADDRESS MmioBasePhysical;
    ULONG MmioLength;
    
    // Locks
    WDFSPINLOCK FifoLock;
    
    // Input state
    BOOLEAN ControllerReady;
    KEYBOARD_REPORT LastKeyboardReport;
    TRACKPAD_REPORT LastTrackpadReport;
} INPUT_DEVICE_EXTENSION, *PINPUT_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INPUT_DEVICE_EXTENSION, GetInputDeviceExtension)

// Prototypes
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtInputDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtInputPrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtInputReleaseHardware;

NTSTATUS InputInitializeController(PINPUT_DEVICE_EXTENSION DeviceExtension);
NTSTATUS InputReadSpiPacket(PINPUT_DEVICE_EXTENSION DeviceExtension, PSPI_PACKET Packet);
NTSTATUS InputProcessPacket(PINPUT_DEVICE_EXTENSION DeviceExtension, PSPI_PACKET Packet);

#endif // _APPLE_A18PRO_INPUT_H_
