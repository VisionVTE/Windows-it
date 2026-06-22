/*
 * Apple A18 Pro GPU Windows Display Driver
 * MacBook Neo (Mac17,5) - WDDM Display Driver Model
 * 
 * Implementation: Core Driver Functions
 */

#include "apple_a18pro_driver.h"
#include <ntstrsafe.h>

/* ============================================================================
 * Driver Initialization and Cleanup
 * ============================================================================ */

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    /*
     * DriverEntry: Main driver entry point called by Windows during loading
     * 
     * Args:
     *   DriverObject: Pointer to the driver object
     *   RegistryPath: Registry path where driver parameters are stored
     * 
     * Returns: NTSTATUS indicating success or failure
     */
    
    NTSTATUS Status;
    
    DbgPrint("[Apple A18 Pro] Driver Entry - Version %d.%d.%d.%d\n",
             DRIVER_MAJOR_VERSION,
             DRIVER_MINOR_VERSION,
             DRIVER_BUILD_NUMBER,
             DRIVER_REVISION);
    
    DbgPrint("[Apple A18 Pro] Registry Path: %wZ\n", RegistryPath);
    
    /* Set up required driver callbacks */
    DriverObject->DriverUnload = DriverUnload;
    
    /* The actual device initialization will be handled by the miniport driver
     * This is the display driver portion of the WDDM model */
    
    DbgPrint("[Apple A18 Pro] Driver Entry Complete\n");
    
    return STATUS_SUCCESS;
}

VOID DriverUnload(
    PDRIVER_OBJECT DriverObject
)
{
    /*
     * DriverUnload: Called when the driver is being unloaded
     */
    
    DbgPrint("[Apple A18 Pro] Driver Unload\n");
    
    /* Cleanup any global resources */
    /* In a real implementation, this would clean up device objects,
     * deregister callbacks, etc. */
}

/* ============================================================================
 * Device Initialization
 * ============================================================================ */

NTSTATUS AppleA18ProInitialize(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProInitialize: Initialize the GPU device
     * 
     * This function:
     * - Maps hardware memory regions
     * - Initializes GPU registers
     * - Sets up power management
     * - Allocates framebuffer memory
     * 
     * Returns: NTSTATUS
     */
    
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG RegisterValue;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Initializing device\n");
    
    /* Initialize synchronization objects */
    KeInitializeSpinLock(&DeviceExtension->DeviceLock);
    KeInitializeEvent(&DeviceExtension->HardwareInitialized, NotificationEvent, FALSE);
    
    /* Map physical memory regions */
    Status = AppleA18ProMapMemory(DeviceExtension);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[Apple A18 Pro] Failed to map memory: 0x%08X\n", Status);
        return Status;
    }
    
    /* Read and validate device ID */
    if (DeviceExtension->ControlRegs) {
        RegisterValue = *(PULONG)DeviceExtension->ControlRegs;
        DbgPrint("[Apple A18 Pro] Device ID Register: 0x%08X\n", RegisterValue);
    }
    
    /* Set default display configuration */
    DeviceExtension->CurrentWidth = DISPLAY_WIDTH;
    DeviceExtension->CurrentHeight = DISPLAY_HEIGHT;
    DeviceExtension->CurrentBitsPerPixel = 32;
    DeviceExtension->CurrentRefreshRate = DISPLAY_FREQUENCY_HZ;
    
    /* Query and store device capabilities */
    Status = AppleA18ProQueryCapabilities(DeviceExtension, &DeviceExtension->Capabilities);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[Apple A18 Pro] Failed to query capabilities: 0x%08X\n", Status);
        AppleA18ProUnmapMemory(DeviceExtension);
        return Status;
    }
    
    /* Allocate framebuffer */
    Status = AppleA18ProAllocateFramebuffer(
        DeviceExtension,
        DeviceExtension->CurrentWidth,
        DeviceExtension->CurrentHeight,
        DeviceExtension->CurrentBitsPerPixel
    );
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[Apple A18 Pro] Failed to allocate framebuffer: 0x%08X\n", Status);
        AppleA18ProUnmapMemory(DeviceExtension);
        return Status;
    }
    
    /* Enable the device */
    Status = AppleA18ProEnableDevice(DeviceExtension);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[Apple A18 Pro] Failed to enable device: 0x%08X\n", Status);
        AppleA18ProFreeFramebuffer(DeviceExtension);
        AppleA18ProUnmapMemory(DeviceExtension);
        return Status;
    }
    
    KeSetEvent(&DeviceExtension->HardwareInitialized, 0, FALSE);
    
    DbgPrint("[Apple A18 Pro] Device initialization complete\n");
    
    return Status;
}

NTSTATUS AppleA18ProQueryCapabilities(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PGPU_CAPABILITIES Capabilities
)
{
    /*
     * AppleA18ProQueryCapabilities: Query hardware capabilities
     * 
     * Returns the GPU's supported features and limits
     */
    
    if (!DeviceExtension || !Capabilities) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Querying capabilities\n");
    
    /* Set capability flags based on Apple A18 Pro specifications */
    Capabilities->SupportsHardwareAcceleration = TRUE;
    Capabilities->SupportsDoubleBuffering = TRUE;
    Capabilities->SupportsStereo = FALSE;
    Capabilities->SupportsVSync = TRUE;
    Capabilities->SupportsIntelligentBusStandby = TRUE;
    Capabilities->MaxDisplayModes = 32;
    Capabilities->MaxSurfaceCount = 8;
    
    return STATUS_SUCCESS;
}

/* ============================================================================
 * Hardware Control
 * ============================================================================ */

NTSTATUS AppleA18ProEnableDevice(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProEnableDevice: Enable GPU hardware
     * 
     * Powers on GPU and initializes core functionality
     */
    
    NTSTATUS Status;
    KIRQL OldIrql;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Enabling device\n");
    
    /* Acquire spinlock for thread-safe register access */
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->ControlRegs) {
        PULONG ControlReg = (PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_CONTROL_REG);
        *ControlReg = GPU_CONTROL_ENABLE;
        
        /* Memory barrier to ensure write completes */
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    /* Set initial power state */
    Status = AppleA18ProSetPowerState(DeviceExtension, PowerDeviceD0);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[Apple A18 Pro] Failed to set power state: 0x%08X\n", Status);
        return Status;
    }
    
    /* Enable interrupts */
    Status = AppleA18ProEnableInterrupts(DeviceExtension);
    if (!NT_SUCCESS(Status)) {
        DbgPrint("[Apple A18 Pro] Failed to enable interrupts: 0x%08X\n", Status);
        return Status;
    }
    
    DeviceExtension->PowerState = PowerDeviceD0;
    
    DbgPrint("[Apple A18 Pro] Device enabled successfully\n");
    
    return STATUS_SUCCESS;
}

NTSTATUS AppleA18ProDisableDevice(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProDisableDevice: Disable GPU hardware
     * 
     * Powers down GPU and halts all operations
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Disabling device\n");
    
    /* Disable interrupts first */
    AppleA18ProDisableInterrupts(DeviceExtension);
    
    /* Acquire spinlock */
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->ControlRegs) {
        PULONG ControlReg = (PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_CONTROL_REG);
        *ControlReg = GPU_CONTROL_DISABLE;
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    /* Set power state to off */
    AppleA18ProSetPowerState(DeviceExtension, PowerDeviceD3);
    
    DeviceExtension->PowerState = PowerDeviceD3;
    
    DbgPrint("[Apple A18 Pro] Device disabled\n");
    
    return STATUS_SUCCESS;
}

NTSTATUS AppleA18ProResetDevice(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProResetDevice: Reset GPU to initial state
     * 
     * Used for error recovery and reinitialization
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Resetting device\n");
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->ControlRegs) {
        PULONG ControlReg = (PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_CONTROL_REG);
        *ControlReg = GPU_CONTROL_RESET;
        KeMemoryBarrier();
        
        /* Wait for reset to complete (typically a few microseconds) */
        KeStallExecutionProcessor(10);
        
        *ControlReg = GPU_CONTROL_ENABLE;
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    DbgPrint("[Apple A18 Pro] Device reset complete\n");
    
    return STATUS_SUCCESS;
}

/* ============================================================================
 * Display Mode Management
 * ============================================================================ */

NTSTATUS AppleA18ProSetDisplayMode(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PDISPLAY_MODE_INFO DisplayMode
)
{
    /*
     * AppleA18ProSetDisplayMode: Configure display output parameters
     * 
     * Sets resolution, refresh rate, and timing parameters
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension || !DisplayMode) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Setting display mode: %ux%u@%uHz, %u-bit\n",
             DisplayMode->Width,
             DisplayMode->Height,
             DisplayMode->RefreshRate,
             DisplayMode->BitsPerPixel);
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    /* Update device context */
    DeviceExtension->CurrentWidth = DisplayMode->Width;
    DeviceExtension->CurrentHeight = DisplayMode->Height;
    DeviceExtension->CurrentRefreshRate = DisplayMode->RefreshRate;
    DeviceExtension->CurrentBitsPerPixel = DisplayMode->BitsPerPixel;
    DisplayMode->BytesPerPixel = DisplayMode->BitsPerPixel / 8;
    
    /* Write timing parameters to hardware registers */
    if (DeviceExtension->DisplayRegs) {
        PULONG DisplayCtrlReg = (PULONG)((PCHAR)DeviceExtension->DisplayRegs + GPU_DISPLAY_CTRL_REG);
        PULONG DisplayModeReg = (PULONG)((PCHAR)DeviceExtension->DisplayRegs + GPU_DISPLAY_MODE_REG);
        
        /* Update display mode register with resolution and color depth */
        *DisplayModeReg = (DisplayMode->Width << 16) | DisplayMode->Height;
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    return STATUS_SUCCESS;
}

NTSTATUS AppleA18ProGetDisplayMode(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PDISPLAY_MODE_INFO DisplayMode
)
{
    /*
     * AppleA18ProGetDisplayMode: Query current display configuration
     */
    
    if (!DeviceExtension || !DisplayMode) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DisplayMode->Width = DeviceExtension->CurrentWidth;
    DisplayMode->Height = DeviceExtension->CurrentHeight;
    DisplayMode->RefreshRate = DeviceExtension->CurrentRefreshRate;
    DisplayMode->BitsPerPixel = DeviceExtension->CurrentBitsPerPixel;
    DisplayMode->BytesPerPixel = DeviceExtension->CurrentBitsPerPixel / 8;
    
    return STATUS_SUCCESS;
}

NTSTATUS AppleA18ProEnumerateDisplayModes(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG ModeNumber,
    PDISPLAY_MODE_INFO DisplayMode
)
{
    /*
     * AppleA18ProEnumerateDisplayModes: List supported display modes
     * 
     * A18 Pro's Retina display is 2408x1506 @ 120Hz
     * Supported modes include scaled resolutions
     */
    
    static DISPLAY_MODE_INFO SupportedModes[] = {
        /* Native mode */
        { 2408, 1506, 120, 32, 4, 0 },
        { 2408, 1506, 60,  32, 4, 0 },
        
        /* Common scaled resolutions */
        { 2048, 1280, 120, 32, 4, 0 },
        { 2048, 1280, 60,  32, 4, 0 },
        { 1920, 1200, 120, 32, 4, 0 },
        { 1920, 1200, 60,  32, 4, 0 },
        { 1680, 1050, 120, 32, 4, 0 },
        { 1680, 1050, 60,  32, 4, 0 },
        { 1600, 1024, 120, 32, 4, 0 },
        { 1600, 1024, 60,  32, 4, 0 },
    };
    
    #define NUM_SUPPORTED_MODES (sizeof(SupportedModes) / sizeof(DISPLAY_MODE_INFO))
    
    if (!DeviceExtension || !DisplayMode) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if (ModeNumber >= NUM_SUPPORTED_MODES) {
        return STATUS_NO_MORE_ENTRIES;
    }
    
    *DisplayMode = SupportedModes[ModeNumber];
    
    return STATUS_SUCCESS;
}

/* ============================================================================
 * Power Management
 * ============================================================================ */

NTSTATUS AppleA18ProSetPowerState(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    DEVICE_POWER_STATE PowerState
)
{
    /*
     * AppleA18ProSetPowerState: Manage device power states
     * 
     * D0: Fully powered on
     * D1-D2: Low-power states
     * D3: Off
     */
    
    KIRQL OldIrql;
    ULONG PowerCtrlValue = 0;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Setting power state: D%u\n", PowerState);
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    switch (PowerState) {
        case PowerDeviceD0:
            PowerCtrlValue = GPU_POWER_STATE_ACTIVE;
            break;
        case PowerDeviceD1:
        case PowerDeviceD2:
            PowerCtrlValue = GPU_POWER_STATE_IDLE;
            break;
        case PowerDeviceD3:
            PowerCtrlValue = GPU_POWER_STATE_OFF;
            break;
        default:
            KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
            return STATUS_INVALID_PARAMETER;
    }
    
    if (DeviceExtension->PowerRegs) {
        PULONG PowerStateReg = (PULONG)((PCHAR)DeviceExtension->PowerRegs + GPU_POWER_CTRL_REG);
        *PowerStateReg = PowerCtrlValue;
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    DeviceExtension->PowerState = PowerState;
    
    return STATUS_SUCCESS;
}

DEVICE_POWER_STATE AppleA18ProGetPowerState(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProGetPowerState: Query current power state
     */
    
    if (!DeviceExtension) {
        return PowerDeviceUnspecified;
    }
    
    return DeviceExtension->PowerState;
}

/* ============================================================================
 * Memory Management
 * ============================================================================ */

NTSTATUS AppleA18ProMapMemory(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProMapMemory: Map GPU MMIO memory regions to virtual address space
     */
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    DbgPrint("[Apple A18 Pro] Mapping memory regions\n");
    
    /* In a real implementation, this would:
     * 1. Get physical address from PCI configuration
     * 2. Map each register region using MmMapIoSpace
     * 3. Handle mapping failures
     * 
     * For this stub, we'll allocate non-paged pool as a placeholder
     */
    
    DeviceExtension->VirtualMemoryBase = ExAllocatePoolWithTag(
        NonPagedPool,
        0x10000000, /* 256MB placeholder */
        'APP1'
    );
    
    if (!DeviceExtension->VirtualMemoryBase) {
        DbgPrint("[Apple A18 Pro] Failed to allocate virtual memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    /* Divide into register regions */
    DeviceExtension->ControlRegs = (PVOID)((PCHAR)DeviceExtension->VirtualMemoryBase + GPU_CONTROL_BASE);
    DeviceExtension->PowerRegs = (PVOID)((PCHAR)DeviceExtension->VirtualMemoryBase + GPU_POWER_BASE);
    DeviceExtension->MemoryRegs = (PVOID)((PCHAR)DeviceExtension->VirtualMemoryBase + GPU_MEMORY_BASE);
    DeviceExtension->DisplayRegs = (PVOID)((PCHAR)DeviceExtension->VirtualMemoryBase + GPU_DISPLAY_BASE);
    DeviceExtension->ShaderRegs = (PVOID)((PCHAR)DeviceExtension->VirtualMemoryBase + GPU_SHADER_BASE);
    
    return STATUS_SUCCESS;
}

VOID AppleA18ProUnmapMemory(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProUnmapMemory: Unmap GPU memory regions
     */
    
    if (!DeviceExtension) {
        return;
    }
    
    DbgPrint("[Apple A18 Pro] Unmapping memory regions\n");
    
    if (DeviceExtension->VirtualMemoryBase) {
        ExFreePoolWithTag(DeviceExtension->VirtualMemoryBase, 'APP1');
        DeviceExtension->VirtualMemoryBase = NULL;
    }
}

NTSTATUS AppleA18ProAllocateFramebuffer(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG Width,
    ULONG Height,
    ULONG BitsPerPixel
)
{
    /*
     * AppleA18ProAllocateFramebuffer: Allocate GPU memory for framebuffer
     */
    
    ULONG BytesPerPixel;
    ULONG FramebufferSize;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    BytesPerPixel = BitsPerPixel / 8;
    FramebufferSize = Width * Height * BytesPerPixel;
    
    DbgPrint("[Apple A18 Pro] Allocating framebuffer: %u bytes (%ux%u@%u-bit)\n",
             FramebufferSize, Width, Height, BitsPerPixel);
    
    /* Allocate contiguous physical memory for framebuffer */
    DeviceExtension->FramebufferVirtual = MmAllocateContiguousMemory(
        FramebufferSize,
        RtlConvertUlongToLargeInteger(0xFFFFFFFF)
    );
    
    if (!DeviceExtension->FramebufferVirtual) {
        DbgPrint("[Apple A18 Pro] Failed to allocate framebuffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    DeviceExtension->FramebufferPhysical = MmGetPhysicalAddress(DeviceExtension->FramebufferVirtual);
    DeviceExtension->FramebufferSize = FramebufferSize;
    
    /* Zero out framebuffer memory */
    RtlZeroMemory(DeviceExtension->FramebufferVirtual, FramebufferSize);
    
    DbgPrint("[Apple A18 Pro] Framebuffer allocated at: %p (physical: %p)\n",
             DeviceExtension->FramebufferVirtual,
             DeviceExtension->FramebufferPhysical.LowPart);
    
    return STATUS_SUCCESS;
}

VOID AppleA18ProFreeFramebuffer(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProFreeFramebuffer: Free allocated framebuffer memory
     */
    
    if (!DeviceExtension || !DeviceExtension->FramebufferVirtual) {
        return;
    }
    
    DbgPrint("[Apple A18 Pro] Freeing framebuffer\n");
    
    MmFreeContiguousMemory(DeviceExtension->FramebufferVirtual);
    DeviceExtension->FramebufferVirtual = NULL;
    DeviceExtension->FramebufferSize = 0;
}

/* ============================================================================
 * Interrupt Handling
 * ============================================================================ */

BOOLEAN AppleA18ProInterruptHandler(
    PKINTERRUPT Interrupt,
    PVOID ServiceContext
)
{
    /*
     * AppleA18ProInterruptHandler: GPU interrupt service routine
     * 
     * Handles VSync, completion, and error interrupts
     */
    
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension = (PAPPLE_A18PRO_DEVICE_EXTENSION)ServiceContext;
    ULONG InterruptStatus;
    BOOLEAN Serviced = FALSE;
    
    if (!DeviceExtension || !DeviceExtension->ControlRegs) {
        return FALSE;
    }
    
    /* Read interrupt status register */
    InterruptStatus = *(PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_INTERRUPT_STATUS_REG);
    
    if (InterruptStatus == 0) {
        return FALSE; /* Not our interrupt */
    }
    
    /* Process VSync interrupt */
    if (InterruptStatus & GPU_INT_VSYNC) {
        /* Handle vertical sync - update display buffer pointers, etc. */
        Serviced = TRUE;
    }
    
    /* Process completion interrupt */
    if (InterruptStatus & GPU_INT_COMPLETION) {
        /* Signal command completion event */
        KeSetEvent(&DeviceExtension->HardwareInitialized, 0, FALSE);
        Serviced = TRUE;
    }
    
    /* Process error interrupt */
    if (InterruptStatus & GPU_INT_ERROR) {
        ULONG ErrorStatus = *(PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_ERROR_STATUS_REG);
        DbgPrint("[Apple A18 Pro] GPU Error: 0x%08X\n", ErrorStatus);
        Serviced = TRUE;
    }
    
    /* Clear interrupt status */
    if (Serviced) {
        *(PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_INTERRUPT_CLEAR_REG) = InterruptStatus;
    }
    
    return Serviced;
}

NTSTATUS AppleA18ProEnableInterrupts(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProEnableInterrupts: Enable GPU interrupts
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->ControlRegs) {
        PULONG InterruptEnableReg = (PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_INTERRUPT_ENABLE_REG);
        *InterruptEnableReg = GPU_INT_VSYNC | GPU_INT_COMPLETION | GPU_INT_ERROR | GPU_INT_THERMAL;
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    return STATUS_SUCCESS;
}

NTSTATUS AppleA18ProDisableInterrupts(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProDisableInterrupts: Disable GPU interrupts
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->ControlRegs) {
        PULONG InterruptEnableReg = (PULONG)((PCHAR)DeviceExtension->ControlRegs + GPU_INTERRUPT_ENABLE_REG);
        *InterruptEnableReg = 0;
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    return STATUS_SUCCESS;
}

/* ============================================================================
 * Thermal Management
 * ============================================================================ */

NTSTATUS AppleA18ProReadTemperature(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    PULONG Temperature
)
{
    /*
     * AppleA18ProReadTemperature: Read current GPU temperature
     * 
     * Temperature is in Celsius
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension || !Temperature) {
        return STATUS_INVALID_PARAMETER;
    }
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->PowerRegs) {
        PULONG ThermalStatusReg = (PULONG)((PCHAR)DeviceExtension->PowerRegs + GPU_THERMAL_STATUS_REG);
        *Temperature = *ThermalStatusReg;
    } else {
        *Temperature = 0;
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    return STATUS_SUCCESS;
}

NTSTATUS AppleA18ProSetThermalLimit(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG TemperatureLimit
)
{
    /*
     * AppleA18ProSetThermalLimit: Set maximum GPU temperature threshold
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->PowerRegs) {
        PULONG ThermalLimitReg = (PULONG)((PCHAR)DeviceExtension->PowerRegs + GPU_THERMAL_LIMIT_REG);
        *ThermalLimitReg = TemperatureLimit;
        KeMemoryBarrier();
    }
    
    DeviceExtension->TemperatureLimit = TemperatureLimit;
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
    
    return STATUS_SUCCESS;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

NTSTATUS AppleA18ProWaitForHardware(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension,
    ULONG TimeoutMs
)
{
    /*
     * AppleA18ProWaitForHardware: Wait for GPU to complete operations
     * 
     * Uses kernel event signaling
     */
    
    LARGE_INTEGER Timeout;
    
    if (!DeviceExtension) {
        return STATUS_INVALID_PARAMETER;
    }
    
    Timeout.QuadPart = -10000LL * TimeoutMs; /* Convert to 100-nanosecond intervals */
    
    return KeWaitForSingleObject(
        &DeviceExtension->HardwareInitialized,
        Executive,
        KernelMode,
        FALSE,
        &Timeout
    );
}

VOID AppleA18ProFlushCache(
    PAPPLE_A18PRO_DEVICE_EXTENSION DeviceExtension
)
{
    /*
     * AppleA18ProFlushCache: Flush GPU cache to ensure data coherency
     */
    
    KIRQL OldIrql;
    
    if (!DeviceExtension) {
        return;
    }
    
    KeAcquireSpinLock(&DeviceExtension->DeviceLock, &OldIrql);
    
    if (DeviceExtension->MemoryRegs) {
        PULONG TlbFlushReg = (PULONG)((PCHAR)DeviceExtension->MemoryRegs + GPU_TLB_FLUSH_REG);
        *TlbFlushReg = 1;
        KeMemoryBarrier();
    }
    
    KeReleaseSpinLock(&DeviceExtension->DeviceLock, OldIrql);
}
