/*
 * Apple A18 Pro GPU - Hardware Definitions
 * MacBook Neo (Mac17,5) - Model MHFG4LL/A
 * 
 * This header defines the memory-mapped I/O registers and constants
 * for the Apple A18 Pro integrated GPU.
 */

#ifndef __APPLE_A18PRO_HW_H__
#define __APPLE_A18PRO_HW_H__

#include <ntdef.h>

/* ============================================================================
 * Device Identification
 * ============================================================================ */

#define APPLE_VENDOR_ID                 0x106B
#define APPLE_A18PRO_DEVICE_ID          0x16B5

/* Apple A18 Pro GPU Specification Constants */
#define A18PRO_GPU_CORES                5
#define A18PRO_GPU_EFFICIENCY_CORES     4
#define A18PRO_GPU_PERFORMANCE_CORES    1

#define A18PRO_MAX_MEMORY_MB            8192
#define A18PRO_GPU_FREQUENCY_MHZ        3200

#define A18PRO_DISPLAY_CORES            5

/* ============================================================================
 * Memory Mapped I/O Regions
 * ============================================================================ */

/* GPU Control Register Base Offsets */
#define GPU_CONTROL_BASE                0x00000000
#define GPU_POWER_BASE                  0x00100000
#define GPU_MEMORY_BASE                 0x00200000
#define GPU_DISPLAY_BASE                0x00300000
#define GPU_ENCODER_BASE                0x00400000
#define GPU_SHADER_BASE                 0x00500000

/* ============================================================================
 * GPU Control Registers
 * ============================================================================ */

#define GPU_DEVICE_ID_REG               (GPU_CONTROL_BASE + 0x00)
#define GPU_REVISION_REG                (GPU_CONTROL_BASE + 0x04)
#define GPU_STATUS_REG                  (GPU_CONTROL_BASE + 0x08)
#define GPU_CONTROL_REG                 (GPU_CONTROL_BASE + 0x0C)
#define GPU_ERROR_STATUS_REG            (GPU_CONTROL_BASE + 0x10)
#define GPU_ERROR_CLEAR_REG             (GPU_CONTROL_BASE + 0x14)

/* GPU Status Bits */
#define GPU_STATUS_IDLE                 0x00000001
#define GPU_STATUS_BUSY                 0x00000002
#define GPU_STATUS_ERROR                0x00000004
#define GPU_STATUS_POWERED_UP           0x00000008
#define GPU_STATUS_POWERED_DOWN         0x00000010

/* GPU Control Bits */
#define GPU_CONTROL_ENABLE              0x00000001
#define GPU_CONTROL_DISABLE             0x00000002
#define GPU_CONTROL_RESET               0x00000004
#define GPU_CONTROL_SLEEP               0x00000008

/* ============================================================================
 * Power Management Registers
 * ============================================================================ */

#define GPU_POWER_CTRL_REG              (GPU_POWER_BASE + 0x00)
#define GPU_CLOCK_CTRL_REG              (GPU_POWER_BASE + 0x04)
#define GPU_POWER_STATUS_REG            (GPU_POWER_BASE + 0x08)
#define GPU_THERMAL_STATUS_REG          (GPU_POWER_BASE + 0x0C)
#define GPU_THERMAL_LIMIT_REG           (GPU_POWER_BASE + 0x10)

#define GPU_POWER_STATE_ACTIVE          0x00000000
#define GPU_POWER_STATE_IDLE            0x00000001
#define GPU_POWER_STATE_SLEEP           0x00000002
#define GPU_POWER_STATE_OFF             0x00000003

/* ============================================================================
 * Memory Management Registers
 * ============================================================================ */

#define GPU_MEM_WINDOW_CTRL_REG         (GPU_MEMORY_BASE + 0x00)
#define GPU_MEM_PROTECTION_REG          (GPU_MEMORY_BASE + 0x04)
#define GPU_MEM_TRANSLATE_REG           (GPU_MEMORY_BASE + 0x08)
#define GPU_TLB_FLUSH_REG               (GPU_MEMORY_BASE + 0x0C)
#define GPU_COHERENCY_CTRL_REG          (GPU_MEMORY_BASE + 0x10)

/* ============================================================================
 * Display/Output Registers
 * ============================================================================ */

#define GPU_DISPLAY_CTRL_REG            (GPU_DISPLAY_BASE + 0x00)
#define GPU_DISPLAY_STATUS_REG          (GPU_DISPLAY_BASE + 0x04)
#define GPU_DISPLAY_MODE_REG            (GPU_DISPLAY_BASE + 0x08)
#define GPU_DISPLAY_TIMING_REG          (GPU_DISPLAY_BASE + 0x0C)
#define GPU_DISPLAY_BUFFER_REG          (GPU_DISPLAY_BASE + 0x10)

/* ============================================================================
 * Interrupt Handling
 * ============================================================================ */

#define GPU_INTERRUPT_STATUS_REG        (GPU_CONTROL_BASE + 0x20)
#define GPU_INTERRUPT_ENABLE_REG        (GPU_CONTROL_BASE + 0x24)
#define GPU_INTERRUPT_CLEAR_REG         (GPU_CONTROL_BASE + 0x28)

#define GPU_INT_VSYNC                   0x00000001
#define GPU_INT_HSYNC                   0x00000002
#define GPU_INT_COMPLETION              0x00000004
#define GPU_INT_ERROR                   0x00000008
#define GPU_INT_THERMAL                 0x00000010

/* ============================================================================
 * Display Modes (Retina 2408 x 1506)
 * ============================================================================ */

#define DISPLAY_WIDTH                   2408
#define DISPLAY_HEIGHT                  1506
#define DISPLAY_FREQUENCY_HZ            120
#define DISPLAY_BITS_PER_PIXEL          32

#define DISPLAY_H_BLANK                 160
#define DISPLAY_V_BLANK                 60

#define DISPLAY_H_FRONT_PORCH           40
#define DISPLAY_H_SYNC_WIDTH            10
#define DISPLAY_H_BACK_PORCH            110

#define DISPLAY_V_FRONT_PORCH           20
#define DISPLAY_V_SYNC_WIDTH            5
#define DISPLAY_V_BACK_PORCH            35

/* ============================================================================
 * Shader/Command Queue Registers
 * ============================================================================ */

#define GPU_COMMAND_QUEUE_BASE          0x10000000
#define GPU_COMMAND_QUEUE_SIZE          0x00010000
#define GPU_COMMAND_QUEUE_HEAD_REG      (GPU_SHADER_BASE + 0x00)
#define GPU_COMMAND_QUEUE_TAIL_REG      (GPU_SHADER_BASE + 0x04)
#define GPU_COMMAND_QUEUE_STATUS_REG    (GPU_SHADER_BASE + 0x08)

/* ============================================================================
 * Hardware Structures
 * ============================================================================ */

typedef struct _GPU_DEVICE_INFO {
    UINT32 DeviceId;
    UINT32 VendorId;
    UINT32 Revision;
    UINT32 CoreCount;
    UINT32 MaxMemory;
    UINT32 MaxFrequency;
} GPU_DEVICE_INFO, *PGPU_DEVICE_INFO;

typedef struct _GPU_CAPABILITIES {
    BOOLEAN SupportsHardwareAcceleration;
    BOOLEAN SupportsDoubleBuffering;
    BOOLEAN SupportsStereo;
    BOOLEAN SupportsVSync;
    BOOLEAN SupportsIntelligentBusStandby;
    UINT32 MaxDisplayModes;
    UINT32 MaxSurfaceCount;
} GPU_CAPABILITIES, *PGPU_CAPABILITIES;

#endif /* __APPLE_A18PRO_HW_H__ */
