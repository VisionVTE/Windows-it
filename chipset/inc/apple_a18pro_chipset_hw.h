/*
 * Apple A18 Pro Chipset / System Controller Driver
 * 
 * Hardware Register Definitions for PMGR, AIC, and GPIO
 */

#ifndef _APPLE_A18PRO_CHIPSET_HW_H_
#define _APPLE_A18PRO_CHIPSET_HW_H_

// 1. PCI Hardware Identification
#define APPLE_VENDOR_ID                0x106B
#define A18PRO_CHIPSET_DEVICE_ID       0x16B0
#define A18PRO_CHIPSET_REVISION        0x01

// 2. Memory-Mapped I/O Base Offsets (GPU-Relative or System-relative virtual offsets)
#define PMGR_BASE_OFFSET               0x00000000  // Power Management (64KB)
#define AIC_BASE_OFFSET                0x00010000  // Interrupt Controller (64KB)
#define GPIO_BASE_OFFSET               0x00020000  // GPIO Controller (64KB)

#define CHIPSET_MMIO_SIZE              0x00030000  // 192KB total MMIO space

// 3. Power Manager (PMGR) Registers
#define PMGR_DEVICE_ID                 0x0000      // Chipset identification
#define PMGR_CONTROL                   0x0004      // Core PMGR configuration
#define PMGR_STATUS                    0x0008      // Core power statuses
#define PMGR_GPU_PWR_CTRL              0x0010      // Power gating GPU domain
#define PMGR_CPU_PWR_STAT              0x0014      // CPU power domains status
#define PMGR_CLOCK_GATE_EN             0x0020      // Clock gating enables
#define PMGR_VOLTAGE_SCALE             0x0030      // Core voltage rails scaling
#define PMGR_SLEEP_CONTROL             0x0040      // SoC deep sleep configuration

// PMGR Bits
#define PMGR_CTRL_ENABLE               0x00000001  // Enable power manager
#define PMGR_STATUS_STABLE             0x00000001  // Power domains stable
#define PMGR_GPU_DOMAIN_ON             0x00000001  // Power up GPU core
#define PMGR_SLEEP_ENTER               0x00000001  // Initiate low power state

// 4. Apple Interrupt Controller (AIC) Registers
#define AIC_CONTROL                    0x0000      // Global interrupt configuration
#define AIC_IRQ_STATUS                 0x0004      // Pending IRQ status (Read-Only)
#define AIC_FIQ_STATUS                 0x0008      // Pending FIQ status (Read-Only)
#define AIC_MASK_SET                   0x0010      // Mask interrupts (Write 1 to mask)
#define AIC_MASK_CLEAR                 0x0014      // Unmask interrupts (Write 1 to clear)
#define AIC_CLEAR_PENDING              0x0020      // Clear pending interrupts
#define AIC_AFFINITY_ROUTING           0x0030      // Route interrupts to CPU cores

// AIC Bits
#define AIC_CTRL_ENABLE                0x00000001  // Enable interrupt routing
#define AIC_IRQ_MAX                    128         // Up to 128 interrupt sources

// 5. GPIO Registers
#define GPIO_PIN_CONFIG(pin)           (0x0000 + ((pin) * 4)) // Configuration per pin
#define GPIO_PIN_VALUE(pin)            (0x0100 + ((pin) * 4)) // Pin values
#define GPIO_INT_ENABLE(pin)           (0x0200 + ((pin) * 4)) // GPIO Interrupt Enable
#define GPIO_INT_STATUS(pin)           (0x0300 + ((pin) * 4)) // GPIO Interrupt Status

// GPIO Pin Directions
#define GPIO_DIR_INPUT                 0x00000000  // Configure pin as input
#define GPIO_DIR_OUTPUT                0x00000001  // Configure pin as output
#define GPIO_PULL_NONE                 0x00000000  // No pullup/pulldown
#define GPIO_PULL_UP                   0x00000002  // Enable pullup
#define GPIO_PULL_DOWN                 0x00000004  // Enable pulldown
#define GPIO_INT_TRIGGER_LEVEL         0x00000000  // Level trigger interrupt
#define GPIO_INT_TRIGGER_EDGE_RISE     0x00000010  // Edge trigger rising
#define GPIO_INT_TRIGGER_EDGE_FALL     0x00000020  // Edge trigger falling

#define GPIO_MAX_PINS                  32          // 32 General Purpose Pins supported

#endif // _APPLE_A18PRO_CHIPSET_HW_H_
