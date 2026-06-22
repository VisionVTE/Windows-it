# Apple A18 Pro GPU Driver - Technical Architecture

## 1. Driver Architecture Overview

### 1.1 WDDM Model Components

```
┌─────────────────────────────────────────────────────────────┐
│                   Windows OS Kernel                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ↓
        ┌───────────────────────────────────────┐
        │     Display Driver (UMD + KMD)        │
        │                                       │
        │  ┌─────────────────────────────────┐  │
        │  │  User Mode Display Driver       │  │
        │  │  (AppleA18ProDisplay.dll)       │  │
        │  └─────────────────────────────────┘  │
        │              │                         │
        │              ↓                         │
        │  ┌─────────────────────────────────┐  │
        │  │  Kernel Mode Display Driver     │  │
        │  │  (KMD - Core Implementation)    │  │
        │  │                                 │  │
        │  │ ┌───────────────────────────┐   │  │
        │  │ │ Device Context (Ext.)     │   │  │
        │  │ │ ├─ Hardware State         │   │  │
        │  │ │ ├─ Memory Mappings        │   │  │
        │  │ │ ├─ Register Pointers      │   │  │
        │  │ │ └─ Power State            │   │  │
        │  │ └───────────────────────────┘   │  │
        │  │                                 │  │
        │  │ ┌───────────────────────────┐   │  │
        │  │ │ Function Implementations: │   │  │
        │  │ │ ├─ Initialize             │   │  │
        │  │ │ ├─ EnableDevice           │   │  │
        │  │ │ ├─ SetDisplayMode         │   │  │
        │  │ │ ├─ SetPowerState          │   │  │
        │  │ │ ├─ InterruptHandler       │   │  │
        │  │ │ └─ MemoryManagement       │   │  │
        │  │ └───────────────────────────┘   │  │
        │  │                                 │  │
        │  │ ┌───────────────────────────┐   │  │
        │  │ │ Synchronization:          │   │  │
        │  │ │ ├─ Spinlocks              │   │  │
        │  │ │ ├─ Events                 │   │  │
        │  │ │ └─ Memory Barriers        │   │  │
        │  │ └───────────────────────────┘   │  │
        │  └─────────────────────────────────┘  │
        │                                       │
        └───────────────────────────────────────┘
                            │
                            ↓
    ┌────────────────────────────────────────────┐
    │   Hardware Abstraction Layer (HAL)         │
    │   (apple_a18pro_hw.h)                      │
    │                                            │
    │  ├─ Register Definitions                   │
    │  ├─ Memory-Mapped I/O Constants            │
    │  ├─ Device Capabilities Structures         │
    │  └─ Hardware Constants                     │
    └────────────────────────────────────────────┘
                            │
                            ↓
    ┌────────────────────────────────────────────┐
    │    Apple A18 Pro GPU Hardware              │
    │    (Physical Device)                       │
    │                                            │
    │  ├─ Control Registers                      │
    │  ├─ Power Management Registers             │
    │  ├─ Memory Management Registers            │
    │  ├─ Display Control Registers              │
    │  ├─ Video Encoder Registers                │
    │  ├─ Shader Execution Registers             │
    │  ├─ Interrupt Registers                    │
    │  ├─ Thermal Sensors                        │
    │  └─ GPU Memory (Unified)                   │
    │                                            │
    └────────────────────────────────────────────┘
```

---

## 2. Device Extension Structure

The `APPLE_A18PRO_DEVICE_EXTENSION` is the core context structure:

```c
typedef struct _APPLE_A18PRO_DEVICE_EXTENSION {
    // Physical Memory Mapping
    PHYSICAL_ADDRESS PhysicalMemoryBase;
    ULONG PhysicalMemoryLength;
    PVOID VirtualMemoryBase;
    
    // Hardware Information
    GPU_DEVICE_INFO HwInfo;              // Device ID, revision, core count
    GPU_CAPABILITIES Capabilities;       // Feature flags and limits
    
    // Register Mappings (Virtual Addresses)
    PVOID ControlRegs;                   // GPU control registers
    PVOID PowerRegs;                     // Power management registers
    PVOID MemoryRegs;                    // Memory management registers
    PVOID DisplayRegs;                   // Display output registers
    PVOID ShaderRegs;                    // Shader execution registers
    
    // Current Display State
    ULONG CurrentWidth;                  // Current resolution width
    ULONG CurrentHeight;                 // Current resolution height
    ULONG CurrentBitsPerPixel;           // Current color depth (32)
    ULONG CurrentRefreshRate;            // Current refresh rate (Hz)
    
    // Framebuffer (Display Memory)
    PHYSICAL_ADDRESS FramebufferPhysical; // Physical address
    PVOID FramebufferVirtual;             // Virtual address
    ULONG FramebufferSize;                // Size in bytes
    
    // Power Management
    DEVICE_POWER_STATE PowerState;        // Current D-state (D0-D3)
    SYSTEM_POWER_STATE SystemPowerState;  // System power state
    
    // Interrupt Handling
    PKINTERRUPT InterruptObject;          // Interrupt object
    BOOLEAN InterruptConnected;           // Interrupt connected flag
    
    // Synchronization
    KSPIN_LOCK DeviceLock;                // Spinlock for register access
    KEVENT HardwareInitialized;           // Event for init completion
    
    // Thermal Management
    ULONG CurrentTemperature;             // Current GPU temperature (°C)
    ULONG TemperatureLimit;               // Thermal throttle threshold
} APPLE_A18PRO_DEVICE_EXTENSION;
```

---

## 3. Register Access Patterns

### 3.1 Safe Register Access

All register access is protected by spinlocks to ensure thread safety:

```c
// Template for safe register write
VOID SafeRegisterWrite(PAPPLE_A18PRO_DEVICE_EXTENSION DevExt,
                       PVOID RegisterAddr,
                       ULONG Value)
{
    KIRQL OldIrql;
    
    // Raise IRQL to DISPATCH_LEVEL
    KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
    
    // Write to register
    *(PULONG)RegisterAddr = Value;
    
    // Memory barrier ensures write completes
    KeMemoryBarrier();
    
    // Restore original IRQL
    KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
}

// Template for safe register read
ULONG SafeRegisterRead(PAPPLE_A18PRO_DEVICE_EXTENSION DevExt,
                       PVOID RegisterAddr)
{
    KIRQL OldIrql;
    ULONG Value;
    
    KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
    Value = *(PULONG)RegisterAddr;
    KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
    
    return Value;
}
```

### 3.2 Register Offset Usage

Registers are accessed by combining base addresses with offsets:

```c
// Example: Set GPU enable bit
PULONG ControlReg = (PULONG)((PCHAR)DevExt->ControlRegs + GPU_CONTROL_REG);
*ControlReg = GPU_CONTROL_ENABLE;
KeMemoryBarrier();

// Example: Read thermal status
PULONG ThermalReg = (PULONG)((PCHAR)DevExt->PowerRegs + GPU_THERMAL_STATUS_REG);
ULONG Temperature = *ThermalReg;

// Example: Check interrupt status
PULONG IntStatusReg = (PULONG)((PCHAR)DevExt->ControlRegs + GPU_INTERRUPT_STATUS_REG);
ULONG Status = *IntStatusReg;
if (Status & GPU_INT_VSYNC) {
    // Handle VSync interrupt
}
```

---

## 4. Initialization Sequence

The driver initialization follows this sequence:

```
1. DriverEntry()
   │
   └─→ Set up driver callbacks
       └─→ Register DriverUnload

2. Device Initialize (PnP Manager calls)
   │
   └─→ AppleA18ProInitialize()
       │
       ├─→ Initialize Synchronization Objects
       │   ├─ KeInitializeSpinLock()
       │   └─ KeInitializeEvent()
       │
       ├─→ Map Physical Memory
       │   └─ AppleA18ProMapMemory()
       │       └─ ExAllocatePoolWithTag()  (or MmMapIoSpace in real driver)
       │
       ├─→ Query Device Capabilities
       │   └─ AppleA18ProQueryCapabilities()
       │
       ├─→ Allocate Framebuffer
       │   └─ AppleA18ProAllocateFramebuffer()
       │       ├─ MmAllocateContiguousMemory()
       │       ├─ MmGetPhysicalAddress()
       │       └─ RtlZeroMemory()
       │
       ├─→ Enable GPU Hardware
       │   └─ AppleA18ProEnableDevice()
       │       ├─ Write GPU_CONTROL_ENABLE to control register
       │       ├─ Set Power State to D0
       │       └─ Enable Interrupts
       │
       └─→ Signal Initialization Complete
           └─ KeSetEvent(&HardwareInitialized)

3. Driver Ready for Use
   │
   └─→ Accept display mode changes, power state transitions, etc.
```

---

## 5. Display Mode Management

### 5.1 Mode Enumeration

The driver provides 10 supported display modes:

```c
SupportedModes[10] = {
    { 2408, 1506, 120, 32, 4 },  // Native @ 120Hz
    { 2408, 1506, 60,  32, 4 },  // Native @ 60Hz
    { 2048, 1280, 120, 32, 4 },  // Scaled @ 120Hz
    { 2048, 1280, 60,  32, 4 },  // Scaled @ 60Hz
    { 1920, 1200, 120, 32, 4 },  // Scaled @ 120Hz
    { 1920, 1200, 60,  32, 4 },  // Scaled @ 60Hz
    { 1680, 1050, 120, 32, 4 },  // Scaled @ 120Hz
    { 1680, 1050, 60,  32, 4 },  // Scaled @ 60Hz
    { 1600, 1024, 120, 32, 4 },  // Scaled @ 120Hz
    { 1600, 1024, 60,  32, 4 },  // Scaled @ 60Hz
};
```

### 5.2 Mode Change Flow

```
User requests mode change
    │
    └─→ AppleA18ProSetDisplayMode(NewMode)
        │
        ├─→ Acquire Spinlock
        │
        ├─→ Update DevExt Display Parameters
        │   ├─ CurrentWidth = NewMode.Width
        │   ├─ CurrentHeight = NewMode.Height
        │   └─ CurrentRefreshRate = NewMode.RefreshRate
        │
        ├─→ Write Display Mode Register
        │   └─ GPU_DISPLAY_MODE_REG = (Width << 16) | Height
        │
        ├─→ Write Timing Parameters
        │   ├─ H_SYNC, H_BLANK, H_BACK_PORCH, etc.
        │   └─ V_SYNC, V_BLANK, V_BACK_PORCH, etc.
        │
        └─→ Release Spinlock
```

---

## 6. Power State Management

### 6.1 Power State Transitions

```
Request Power State Transition
    │
    └─→ AppleA18ProSetPowerState(NewState)
        │
        ├─→ Acquire Spinlock
        │
        ├─→ Map Windows D-state to Hardware State
        │   ├─ D0 → GPU_POWER_STATE_ACTIVE
        │   ├─ D1/D2 → GPU_POWER_STATE_IDLE
        │   └─ D3 → GPU_POWER_STATE_OFF
        │
        ├─→ Write to Power Control Register
        │   └─ GPU_POWER_CTRL_REG = PowerCtrlValue
        │
        ├─→ Memory Barrier
        │
        ├─→ Update DevExt.PowerState
        │
        └─→ Release Spinlock
```

### 6.2 Device Power States

```
D0 - Fully Powered On
├─ GPU enabled for rendering
├─ Display active
├─ All clocks enabled
└─ Maximum power consumption

D1 - Low Power State
├─ Rendering possible with latency
├─ Display remains active
├─ Some clocks gated
└─ Reduced power

D2 - Deeper Low Power
├─ Minimal GPU operation
├─ Display on
└─ Most clocks disabled

D3 - Off
├─ GPU powered off
├─ Display powered off
└─ Minimal power consumption
```

---

## 7. Interrupt Handling

### 7.1 Interrupt Service Routine (ISR)

```c
BOOLEAN AppleA18ProInterruptHandler(PKINTERRUPT Interrupt,
                                     PVOID ServiceContext)
{
    DevExt = ServiceContext;
    
    // Read interrupt status
    Status = Read(GPU_INTERRUPT_STATUS_REG)
    
    if (Status == 0)
        return FALSE;  // Not our interrupt
    
    // Process VSync (120Hz @ display resolution)
    if (Status & GPU_INT_VSYNC)
        Handle_VSync_Interrupt();
    
    // Process Command Completion
    if (Status & GPU_INT_COMPLETION)
        KeSetEvent(&HardwareInitialized);
    
    // Process Errors
    if (Status & GPU_INT_ERROR) {
        ErrorStatus = Read(GPU_ERROR_STATUS_REG);
        DbgPrint("GPU Error: 0x%08X", ErrorStatus);
    }
    
    // Process Thermal Alert
    if (Status & GPU_INT_THERMAL)
        Handle_Thermal_Alert();
    
    // Clear interrupt status
    Write(GPU_INTERRUPT_CLEAR_REG, Status);
    
    return TRUE;  // Interrupt serviced
}
```

### 7.2 Interrupt Types

```
GPU_INT_VSYNC (0x01)
├─ Generated at start of vertical blanking interval
├─ Frequency: 120 Hz (for 2408×1506@120Hz mode)
├─ Used for display buffer synchronization
└─ Must clear to continue receiving VSync interrupts

GPU_INT_HSYNC (0x02)
├─ Generated at start of horizontal blanking interval
├─ Frequency: 2408×120 = 289,000 Hz
└─ Can be used for scanline-based operations

GPU_INT_COMPLETION (0x04)
├─ Signaled when command completes
├─ Used for synchronization
└─ Signals HardwareInitialized event

GPU_INT_ERROR (0x08)
├─ GPU encountered error
├─ Read GPU_ERROR_STATUS_REG for details
└─ May require device reset

GPU_INT_THERMAL (0x10)
├─ GPU exceeded thermal limit
├─ Triggers automatic throttling
└─ Firmware reduces clock speeds
```

---

## 8. Memory Management

### 8.1 Memory Regions

```
Physical Memory Layout:

SoC Unified Memory
├─ GPU Register Space (256 MB)
│  ├─ Control Registers
│  ├─ Power Registers
│  ├─ Memory Registers
│  ├─ Display Registers
│  └─ Shader Registers
│
├─ Framebuffer Memory (varies with resolution)
│  └─ 2408×1506×4 bytes = 14.6 MB per framebuffer
│
├─ Command Queue (64 KB)
│  └─ Commands submitted by driver
│
└─ GPU Memory Heap
   ├─ Texture storage
   ├─ Shader code
   └─ Other GPU resources
```

### 8.2 Memory Operations

```c
// 1. Map Hardware Memory
AppleA18ProMapMemory()
    │
    ├─→ ExAllocatePoolWithTag(256 MB)  // In this stub implementation
    │   // Real implementation would use:
    │   // MmMapIoSpace(PhysicalAddress, Length, MmCached)
    │
    └─→ Divide into register regions
        ├─ ControlRegs = Base + 0x00000000
        ├─ PowerRegs = Base + 0x00100000
        ├─ MemoryRegs = Base + 0x00200000
        ├─ DisplayRegs = Base + 0x00300000
        └─ ShaderRegs = Base + 0x00500000

// 2. Allocate Framebuffer
AppleA18ProAllocateFramebuffer(Width, Height, BitsPerPixel)
    │
    ├─→ Calculate Size = Width × Height × (BitsPerPixel/8)
    │   // Example: 2408 × 1506 × 4 = 14,607,744 bytes
    │
    ├─→ MmAllocateContiguousMemory(Size)
    │
    ├─→ MmGetPhysicalAddress(VirtualAddress)
    │
    └─→ RtlZeroMemory(FramebufferVirtual, Size)

// 3. Free Framebuffer
AppleA18ProFreeFramebuffer()
    └─→ MmFreeContiguousMemory(FramebufferVirtual)

// 4. Unmap Hardware Memory
AppleA18ProUnmapMemory()
    └─→ ExFreePoolWithTag(VirtualMemoryBase)
        // Real implementation would use:
        // MmUnmapIoSpace(VirtualAddress, Length)
```

---

## 9. Thermal Management

### 9.1 Temperature Monitoring

```c
AppleA18ProReadTemperature(DevExt, &Temperature)
    │
    ├─→ Acquire Spinlock
    │
    ├─→ Read GPU_THERMAL_STATUS_REG
    │   // Returns temperature in Celsius
    │   // Typical range: 30-100°C
    │
    └─→ Release Spinlock

AppleA18ProSetThermalLimit(DevExt, LimitTemp)
    │
    ├─→ Acquire Spinlock
    │
    ├─→ Write to GPU_THERMAL_LIMIT_REG
    │   // When exceeded, GPU throttles clock speeds
    │   // Typical limits: 85-100°C
    │
    └─→ Release Spinlock
```

### 9.2 Thermal Response

When GPU temperature exceeds thermal limit:

```
Temperature > Limit
    │
    └─→ GPU_INT_THERMAL interrupt generated
        │
        ├─→ ISR reads GPU_THERMAL_STATUS_REG
        │
        ├─→ GPU automatically throttles
        │  ├─ Reduces clock frequency
        │  ├─ Reduces voltage
        │  └─ Decreases power consumption
        │
        └─→ Performance reduced until cool
```

---

## 10. Synchronization & Thread Safety

### 10.1 Spinlock Usage

All register access is protected by spinlock:

```c
// Template for synchronized operation
NTSTATUS AppleA18ProSynchronizedOperation(DevExt, Params)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    
    // Acquire spinlock (raises IRQL to DISPATCH_LEVEL)
    KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
    
    // Perform register operations
    // Can safely access hardware state
    
    // Memory barrier for hardware consistency
    KeMemoryBarrier();
    
    // Release spinlock (restores IRQL)
    KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
    
    return Status;
}
```

### 10.2 Event Synchronization

Events are used for waiting on hardware completion:

```c
// Signal hardware complete
KeSetEvent(&DevExt->HardwareInitialized, 0, FALSE);

// Wait for hardware complete (with timeout)
Status = KeWaitForSingleObject(
    &DevExt->HardwareInitialized,
    Executive,
    KernelMode,
    FALSE,
    &Timeout  // -10000 * milliseconds
);
```

---

## 11. Error Handling

### 11.1 Device Error Recovery

```
GPU Error Detected
    │
    └─→ GPU_INT_ERROR interrupt
        │
        ├─→ Read GPU_ERROR_STATUS_REG
        │
        ├─→ Log error information
        │
        ├─→ Attempt Recovery
        │  ├─ If recoverable:
        │  │  └─ AppleA18ProResetDevice()
        │  │
        │  └─ If not recoverable:
        │     └─ Disable device and notify system
        │
        └─→ Resume or shutdown
```

### 11.2 Reset Procedure

```c
AppleA18ProResetDevice(DevExt)
    │
    ├─→ Acquire Spinlock
    │
    ├─→ Write GPU_CONTROL_RESET to Control Register
    │   └─ Sets reset bit
    │
    ├─→ KeStallExecutionProcessor(10)
    │   └─ Wait 10 microseconds for reset to take effect
    │
    ├─→ Write GPU_CONTROL_ENABLE to Control Register
    │   └─ Re-enable GPU after reset
    │
    ├─→ KeMemoryBarrier()
    │
    └─→ Release Spinlock
```

---

## 12. Performance Considerations

### 12.1 Register Access Optimization

```c
// Inefficient: Multiple spinlock acquisitions
for (i = 0; i < 10; i++) {
    KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
    WriteRegister(Address[i], Value[i]);
    KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
}

// Efficient: Single spinlock for batch writes
KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
for (i = 0; i < 10; i++) {
    WriteRegister(Address[i], Value[i]);
}
KeMemoryBarrier();
KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
```

### 12.2 Memory Efficiency

```c
// Framebuffer allocation is contiguous for:
// - DMA operations
// - Direct GPU access
// - No paging delays
// - Predictable access patterns
```

---

## 13. Future Extension Points

The architecture supports adding:

```c
// User-mode communication
IoControl(DeviceHandle, IOCTL_*, InputBuffer, OutputBuffer);

// Command submission
Submit_GPU_Commands(DevExt, CommandBuffer, CommandCount);

// Memory allocation
Allocate_GPU_Memory(DevExt, Size, &GPUAddress);

// Shader compilation
Compile_Metal_Shader(ShaderSource, &BinaryCode);

// Performance monitoring
Read_GPU_Counters(DevExt, &PerformanceData);
```

---

**Document Version:** 1.0  
**Date:** June 21, 2026  
**Driver Version:** 1.0.0
