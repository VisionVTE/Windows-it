/*
 * Apple A18 Pro Storage Driver (ANS)
 * 
 * Hardware Register Definitions for Apple NVM Storage (ANS) Controller
 */

#ifndef _APPLE_A18PRO_STORAGE_HW_H_
#define _APPLE_A18PRO_STORAGE_HW_H_

#define APPLE_VENDOR_ID                0x106B
#define A18PRO_STORAGE_DEVICE_ID       0x16B1

// Memory-Mapped Register Base Space
#define ANS_MMIO_SIZE                  0x00010000  // 64KB MMIO space

// Registers
#define ANS_REG_VERSION                0x0000      // Controller version
#define ANS_REG_CONTROL                0x0004      // Core control
#define ANS_REG_STATUS                 0x0008      // Status indicator
#define ANS_REG_INT_ENABLE             0x0010      // Interrupt enable register
#define ANS_REG_INT_STATUS             0x0014      // Interrupt status
#define ANS_REG_CMD_SUBMIT_ADDR_L      0x0020      // Admin Command Queue Submit Low
#define ANS_REG_CMD_SUBMIT_ADDR_H      0x0024      // Admin Command Queue Submit High
#define ANS_REG_CMD_TAIL_DOORBELL      0x0028      // Submission doorbell
#define ANS_REG_COMP_HEAD_DOORBELL     0x002C      // Completion doorbell
#define ANS_REG_DMA_PHYS_ADDR_L        0x0040      // Data DMA Address Low
#define ANS_REG_DMA_PHYS_ADDR_H        0x0044      // Data DMA Address High
#define ANS_REG_DMA_LENGTH             0x0048      // Data DMA transfer length
#define ANS_REG_DMA_CONTROL            0x004C      // DMA trigger control

// ANS Control Bits
#define ANS_CTRL_RESET                 0x00000001  // Reset controller
#define ANS_CTRL_ENABLE                0x00000002  // Enable controller
#define ANS_STATUS_READY               0x00000001  // Controller ready for commands
#define ANS_DMA_START                  0x00000001  // Trigger DMA read/write
#define ANS_DMA_DIR_WRITE              0x00000002  // 1 = Write to flash, 0 = Read from flash

// Interrupts
#define ANS_INT_CMD_COMPLETE           0x00000001  // Command complete IRQ
#define ANS_INT_DMA_COMPLETE           0x00000002  // DMA transfer complete IRQ
#define ANS_INT_ERROR                  0x00000004  // Device error IRQ

#endif // _APPLE_A18PRO_STORAGE_HW_H_
