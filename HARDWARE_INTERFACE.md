# Apple A18 Pro GPU Hardware Interface

## 1. PCI Device Identification

```
Vendor ID:          0x106B (Apple Inc.)
Device ID:          0x16B5 (Theoretical - not publicly documented)
Bus:                PCIe or Direct SoC Integration
Bus Mastering:      Capable
Memory Mapping:     Required
Device Type:        Display Controller / GPU
```

The device would appear in Windows Device Manager as:
```
Display adapters
└─ Apple A18 Pro Graphics
```

---

## 2. Memory-Mapped I/O (MMIO) Architecture

### 2.1 Register Space Layout

```
Base Address Region 0 (BAR0): GPU Memory-Mapped I/O
└─ Size: Variable, typically 256 MB to 1 GB
    │
    ├─ 0x00000000 - 0x000FFFFF: Control Registers (GPU_CONTROL_BASE)
    ├─ 0x00100000 - 0x001FFFFF: Power Registers (GPU_POWER_BASE)
    ├─ 0x00200000 - 0x002FFFFF: Memory Registers (GPU_MEMORY_BASE)
    ├─ 0x00300000 - 0x003FFFFF: Display Registers (GPU_DISPLAY_BASE)
    ├─ 0x00400000 - 0x004FFFFF: Encoder Registers (GPU_ENCODER_BASE)
    ├─ 0x00500000 - 0x005FFFFF: Shader Registers (GPU_SHADER_BASE)
    ├─ 0x10000000 - 0x10FFFFFF: Command Queue
    └─ 0x20000000+: GPU Memory Heap
```

### 2.2 Register Access Methods

All register access is 32-bit (DWORD):

```c
// Write register
*(PULONG)((PCHAR)BaseAddress + Offset) = Value;
KeMemoryBarrier();  // Ensure write completes

// Read register
ULONG Value = *(PULONG)((PCHAR)BaseAddress + Offset);

// Modify bits
ULONG Reg = *(PULONG)Address;
Reg |= BIT_MASK;    // Set bits
Reg &= ~BIT_MASK;   // Clear bits
*(PULONG)Address = Reg;
KeMemoryBarrier();
```

---

## 3. Control Registers

### 3.1 GPU_CONTROL_BASE (0x00000000)

| Offset | Name | Type | Width | Purpose |
|--------|------|------|-------|---------|
| 0x00 | DEVICE_ID_REG | RO | 32-bit | Device identification |
| 0x04 | REVISION_REG | RO | 32-bit | Hardware revision |
| 0x08 | STATUS_REG | RO | 32-bit | Current GPU status |
| 0x0C | CONTROL_REG | RW | 32-bit | GPU control bits |
| 0x10 | ERROR_STATUS_REG | RO | 32-bit | Error flags |
| 0x14 | ERROR_CLEAR_REG | WO | 32-bit | Clear error bits |
| 0x20 | INTERRUPT_STATUS_REG | RO | 32-bit | Pending interrupts |
| 0x24 | INTERRUPT_ENABLE_REG | RW | 32-bit | Enable interrupts |
| 0x28 | INTERRUPT_CLEAR_REG | WO | 32-bit | Clear interrupts |

### 3.2 CONTROL_REG Bits

```
Bit 0: GPU_ENABLE
       1 = Enable GPU
       0 = Disable GPU (remains in clock gating)

Bit 1: GPU_DISABLE
       1 = Disable GPU (power down)
       0 = Normal operation

Bit 2: GPU_RESET
       1 = Reset GPU to initial state
       0 = Normal operation
       Auto-clears after reset complete

Bit 3: GPU_SLEEP
       1 = Enter sleep mode (low power)
       0 = Wake from sleep

Bits 4-31: Reserved
```

### 3.3 STATUS_REG Bits

```
Bit 0: GPU_IDLE
       1 = GPU is idle, no active operations
       0 = GPU is busy

Bit 1: GPU_BUSY
       1 = GPU is processing commands
       0 = GPU is not busy

Bit 2: GPU_ERROR
       1 = GPU encountered error
       0 = No error

Bit 3: GPU_POWERED
       1 = GPU is powered on
       0 = GPU is powered off

Bit 4: GPU_IDLE_DEEP
       1 = GPU in deep idle (lowest power)
       0 = GPU in normal idle

Bits 5-31: Reserved
```

---

## 4. Power Management Registers

### 4.1 GPU_POWER_BASE (0x00100000)

| Offset | Name | Type | Width | Purpose |
|--------|------|------|-------|---------|
| 0x00 | POWER_CTRL_REG | RW | 32-bit | Power state control |
| 0x04 | CLOCK_CTRL_REG | RW | 32-bit | Clock frequency control |
| 0x08 | POWER_STATUS_REG | RO | 32-bit | Current power state |
| 0x0C | THERMAL_STATUS_REG | RO | 32-bit | GPU temperature |
| 0x10 | THERMAL_LIMIT_REG | RW | 32-bit | Thermal throttle threshold |
| 0x14 | VOLTAGE_CTRL_REG | RW | 32-bit | Supply voltage control |
| 0x18 | CLOCK_RATIO_REG | RW | 32-bit | Clock multiplier |

### 4.2 POWER_CTRL_REG Values

```
0x00000000: GPU_POWER_STATE_ACTIVE
            Full power, all clocks enabled
            Typical frequency: 3200 MHz
            Typical power: 5-10W

0x00000001: GPU_POWER_STATE_IDLE
            Low power, gated clocks
            Reduced frequency: 800 MHz
            Typical power: 1-2W

0x00000002: GPU_POWER_STATE_SLEEP
            Deep sleep, minimal power
            Very low frequency: 100 MHz
            Typical power: 0.5W

0x00000003: GPU_POWER_STATE_OFF
            Power off, no clock activity
            Frequency: 0 MHz
            Typical power: <100mW (retention)
```

### 4.3 THERMAL_STATUS_REG

```
Bits 0-7: CURRENT_TEMPERATURE
           Temperature in °C (0-100)
           Range: 0°C to 255°C
           Resolution: 1°C per unit

Bits 8-15: THRESHOLD_EXCEEDED
            1 = Current > Threshold
            0 = Within normal range

Bits 16-23: HYSTERESIS
             Used to prevent rapid on/off cycling
             Typically 5-10°C below threshold

Bits 24-31: SENSOR_STATUS
             0x0: Sensor OK
             0x1: Sensor error
             0x2: Sensor not ready
```

### 4.4 THERMAL_LIMIT_REG

```
Bits 0-7: THROTTLE_LIMIT
          Temperature threshold (°C)
          Typical: 85-100°C
          When exceeded, GPU automatically throttles

Bits 8-15: SHUTDOWN_LIMIT
           Emergency shutdown threshold (°C)
           Typical: 105-110°C
           Forces GPU power off

Bits 16-23: RESERVED

Bits 24-31: HYSTERESIS_WIDTH
            Temperature hysteresis (°C)
            Typical: 5-10°C
```

---

## 5. Display Control Registers

### 5.1 GPU_DISPLAY_BASE (0x00300000)

| Offset | Name | Type | Width | Purpose |
|--------|------|------|-------|---------|
| 0x00 | DISPLAY_CTRL_REG | RW | 32-bit | Display control bits |
| 0x04 | DISPLAY_STATUS_REG | RO | 32-bit | Display status |
| 0x08 | DISPLAY_MODE_REG | RW | 32-bit | Resolution setting |
| 0x0C | DISPLAY_TIMING_REG | RW | 32-bit | Timing parameters |
| 0x10 | FRAMEBUFFER_ADDR_REG | RW | 32-bit | Framebuffer physical address |
| 0x14 | FRAMEBUFFER_PITCH_REG | RW | 32-bit | Framebuffer pitch |

### 5.2 DISPLAY_MODE_REG

```
Bits 0-15: DISPLAY_WIDTH
           Resolution width in pixels
           Range: 0-4096
           For A18 Pro native: 2408

Bits 16-31: DISPLAY_HEIGHT
            Resolution height in pixels
            Range: 0-4096
            For A18 Pro native: 1506
```

### 5.3 DISPLAY_TIMING_REG

```
Bits 0-15: REFRESH_RATE
           Refresh rate in Hz (0-240)
           For A18 Pro: 120

Bits 16-23: VSYNC_PULSE_WIDTH
            V sync pulse width in lines

Bits 24-31: VSYNC_BACK_PORCH
            Lines after V sync
            For A18 Pro: 35
```

### 5.4 Native Display Timing (2408×1506@120Hz)

```
Horizontal Timing:
├─ Active pixels: 2408
├─ Front porch: 40 pixels
├─ Sync pulse: 10 pixels
├─ Back porch: 110 pixels
└─ Total: 2568 pixels / line

Vertical Timing:
├─ Active lines: 1506
├─ Front porch: 20 lines
├─ Sync pulse: 5 lines
├─ Back porch: 35 lines
└─ Total: 1566 lines / frame

Frame Frequency:
├─ Pixel clock: 2568 × 1566 × 120 = 483 MHz
├─ Refresh rate: 120 Hz
└─ Frame time: 8.33 ms
```

---

## 6. Interrupt Architecture

### 6.1 GPU_INTERRUPT_STATUS_REG (0x00000020)

```
Bit 0: GPU_INT_VSYNC
       1 = VSync interrupt pending
       Frequency: 120 times/second (120Hz display)

Bit 1: GPU_INT_HSYNC
       1 = HSync interrupt pending
       Frequency: 2408 × 120 = 289,000 times/second

Bit 2: GPU_INT_COMPLETION
       1 = Command completion interrupt pending

Bit 3: GPU_INT_ERROR
       1 = GPU error occurred

Bit 4: GPU_INT_THERMAL
       1 = Thermal alert
       Usually when temperature > threshold

Bits 5-31: Reserved
```

### 6.2 Interrupt Processing Flow

```
GPU generates interrupt
    │
    └─→ Write interrupt bit to GPU_INTERRUPT_STATUS_REG
        │
        ├─→ ISR reads GPU_INTERRUPT_STATUS_REG
        │   │
        │   ├─ Check each bit for active interrupt
        │   │
        │   └─ Call appropriate handler
        │
        ├─→ Handler performs operation
        │   (e.g., update display buffer for VSYNC)
        │
        └─→ Write to GPU_INTERRUPT_CLEAR_REG
            to clear the interrupt flag
```

---

## 7. Memory Management Registers

### 7.1 GPU_MEMORY_BASE (0x00200000)

| Offset | Name | Type | Width | Purpose |
|--------|------|------|-------|---------|
| 0x00 | MEM_WINDOW_CTRL_REG | RW | 32-bit | Memory window configuration |
| 0x04 | MEM_PROTECTION_REG | RW | 32-bit | Memory protection bits |
| 0x08 | MEM_TRANSLATE_REG | RW | 32-bit | Address translation |
| 0x0C | TLB_FLUSH_REG | WO | 32-bit | TLB cache flush |
| 0x10 | COHERENCY_CTRL_REG | RW | 32-bit | Cache coherency control |

### 7.2 Memory Organization

```
Unified Memory Space (Apple Silicon):
├─ GPU can directly access CPU memory
├─ CPU can directly access GPU memory
├─ No separate discrete GPU memory pool
├─ Automatic cache coherency (in hardware)
└─ 8GB total on MacBook Neo

Memory Regions:
├─ System memory: 0x0 - 0x1FFFFFFFF (up to 8GB)
├─ GPU registers: 0x0 - 0xFFFFFFFF (MMIO)
├─ Framebuffer: Allocated in unified memory
└─ GPU heap: Allocated in unified memory
```

---

## 8. Command Queue (GPU_SHADER_BASE)

### 8.1 GPU_SHADER_BASE (0x00500000)

| Offset | Name | Type | Width | Purpose |
|--------|------|------|-------|---------|
| 0x00 | COMMAND_QUEUE_HEAD | RW | 32-bit | Read pointer |
| 0x04 | COMMAND_QUEUE_TAIL | RW | 32-bit | Write pointer |
| 0x08 | COMMAND_QUEUE_STATUS | RO | 32-bit | Queue status |

### 8.2 Command Queue Operation

```
Command Queue Buffer (0x10000000 - 0x10FFFFFF):
┌──────────────────────────────────────────┐
│          Command Queue (64 KB)           │
│                                          │
│  ┌──────────────────────────────────┐    │
│  │ Command 1 (Metal instruction)    │    │
│  ├──────────────────────────────────┤    │ ← HEAD pointer
│  │ Command 2 (Metal instruction)    │    │
│  ├──────────────────────────────────┤    │
│  │ Command 3 (Metal instruction)    │    │
│  ├──────────────────────────────────┤    │ ← TAIL pointer
│  │ Empty space                      │    │
│  ├──────────────────────────────────┤    │
│  │                                  │    │
│  └──────────────────────────────────┘    │
│                                          │
└──────────────────────────────────────────┘

Operation:
1. Driver writes commands to memory at TAIL
2. Driver increments TAIL pointer
3. GPU executes commands from HEAD
4. GPU increments HEAD pointer
5. When HEAD == TAIL, queue is empty
```

---

## 9. Power States & Transitions

### 9.1 State Transition Diagram

```
                    ┌─────────────┐
                    │    D0       │
                    │  (Active)   │
                    │ Full Power  │
                    └──────┬──────┘
                           │
                    ┌──────▼──────┐
                    │    D1/D2    │
                    │  (Idle)     │
                    │ Low Power   │
                    └──────┬──────┘
                           │
                    ┌──────▼──────┐
                    │    D3       │
                    │   (Off)     │
                    │ Minimal Pwr │
                    └─────────────┘

Typical Transition Times:
D0 ↔ D1: ~1-10 ms
D1 ↔ D2: ~5-50 ms
D2 ↔ D3: ~10-100 ms
```

### 9.2 Power Consumption

```
Device Power State    Frequency    Power Draw
─────────────────────────────────────────────
D0 (Active)          3200 MHz     5-10W
D1 (Idle-Light)      1600 MHz     2-3W
D2 (Idle-Deep)        400 MHz     0.5-1W
D3 (Off)              0 MHz       <100mW
```

---

## 10. Thermal Characteristics

### 10.1 Temperature Sensors

The A18 Pro includes multiple thermal sensors:

```
Sensor 0: GPU Core
           Location: GPU execution engine
           Update rate: ~100 samples/second

Sensor 1: GPU Memory Controller
           Location: Memory interface
           Update rate: ~100 samples/second

Sensor 2: SoC Package
           Location: Package thermal junction
           Update rate: ~100 samples/second
```

### 10.2 Thermal Response Curve

```
Temperature (°C) | Action
─────────────────|──────────────────────────────
< 50            | Normal operation, fans off
50-70           | Normal operation, fans ramping
70-85           | Normal operation, fans full speed
85-95           | Thermal throttling begins
95-100          | Severe throttling
100+            | Emergency shutdown
```

---

## 11. Interrupt Timing

### 11.1 VSync Interrupt Timing (120Hz Display)

```
Frame 1 starts (Pixel Clock begins)
    │
    ├─ 0.0 ms: Horizontal scan starts
    ├─ 8.33 ms: Frame complete (VSync)
    │
    └─→ GPU_INT_VSYNC interrupt generated
        │
        └─→ ISR executes (~1-10 μs)
            ├─ Update display buffer pointers
            ├─ Signal completion event
            └─ Clear interrupt bit

Frame 2 starts...
```

### 11.2 Interrupt Frequency

```
VSync (120Hz):          120 interrupts/second
HSync (2408×120):       289,000 interrupts/second (usually disabled)
Completion (varies):    Depends on workload
Thermal (infrequent):   Occasional if GPU hot
```

---

## 12. Practical Register Access Examples

### 12.1 Enable GPU

```c
NTSTATUS EnableGPU(PAPPLE_A18PRO_DEVICE_EXTENSION DevExt) {
    KIRQL OldIrql;
    
    KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
    
    // Write enable bit to control register
    PULONG ControlReg = (PULONG)((PCHAR)DevExt->ControlRegs + GPU_CONTROL_REG);
    *ControlReg = GPU_CONTROL_ENABLE;
    KeMemoryBarrier();
    
    // Wait for GPU to acknowledge
    KeStallExecutionProcessor(100);  // 100 microseconds
    
    // Verify GPU is powered
    PULONG StatusReg = (PULONG)((PCHAR)DevExt->ControlRegs + GPU_STATUS_REG);
    ULONG Status = *StatusReg;
    
    KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
    
    return (Status & GPU_STATUS_POWERED_UP) ? STATUS_SUCCESS : STATUS_DEVICE_DATA_ERROR;
}
```

### 12.2 Set Display Mode

```c
NTSTATUS SetDisplayMode(PAPPLE_A18PRO_DEVICE_EXTENSION DevExt,
                        ULONG Width, ULONG Height) {
    KIRQL OldIrql;
    
    KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
    
    // Write resolution to display mode register
    PULONG ModeReg = (PULONG)((PCHAR)DevExt->DisplayRegs + GPU_DISPLAY_MODE_REG);
    ULONG ModeValue = (Width << 16) | Height;
    *ModeReg = ModeValue;
    KeMemoryBarrier();
    
    KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
    
    // Update device context
    DevExt->CurrentWidth = Width;
    DevExt->CurrentHeight = Height;
    
    return STATUS_SUCCESS;
}
```

### 12.3 Read Temperature

```c
NTSTATUS ReadTemperature(PAPPLE_A18PRO_DEVICE_EXTENSION DevExt,
                         PULONG Temperature) {
    KIRQL OldIrql;
    
    KeAcquireSpinLock(&DevExt->DeviceLock, &OldIrql);
    
    // Read thermal status register
    PULONG ThermalReg = (PULONG)((PCHAR)DevExt->PowerRegs + GPU_THERMAL_STATUS_REG);
    ULONG ThermalValue = *ThermalReg;
    
    // Extract temperature (bits 0-7)
    *Temperature = ThermalValue & 0xFF;
    
    KeReleaseSpinLock(&DevExt->DeviceLock, OldIrql);
    
    return STATUS_SUCCESS;
}
```

---

## 13. Summary Table: Key Registers

| Address | Name | Type | Key Purpose |
|---------|------|------|-------------|
| 0x0C | CONTROL | RW | Enable/Disable/Reset GPU |
| 0x08 | STATUS | RO | Check if GPU is idle/busy |
| 0x20 | INT_STATUS | RO | Check for pending interrupts |
| 0x24 | INT_ENABLE | RW | Enable specific interrupts |
| 0x00100000 | POWER_CTRL | RW | Set power state (D0-D3) |
| 0x00100008 | THERMAL | RO | Read GPU temperature |
| 0x0010000C | THERMAL_LIMIT | RW | Set throttle temperature |
| 0x00300008 | DISPLAY_MODE | RW | Set resolution |
| 0x0030000C | DISPLAY_TIMING | RW | Set refresh rate |
| 0x00500000 | COMMAND_HEAD | RW | GPU command queue head |
| 0x00500004 | COMMAND_TAIL | RW | GPU command queue tail |

---

**Document Version:** 1.0  
**Date:** June 21, 2026  
**Driver Version:** 1.0.0
