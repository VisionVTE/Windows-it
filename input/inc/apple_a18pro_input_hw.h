/*
 * Apple A18 Pro HID Input Driver (SPI HID)
 * 
 * Hardware Register Definitions for Apple Keyboard/Trackpad SPI Controller
 */

#ifndef _APPLE_A18PRO_INPUT_HW_H_
#define _APPLE_A18PRO_INPUT_HW_H_

#define APPLE_VENDOR_ID                0x106B
#define A18PRO_INPUT_DEVICE_ID         0x16B4

// Memory-Mapped Register Base Space
#define SPI_MMIO_SIZE                  0x00010000  // 64KB MMIO space

// Registers
#define SPI_REG_VERSION                0x0000      // Controller version
#define SPI_REG_CONTROL                0x0004      // Core control
#define SPI_REG_STATUS                 0x0008      // Status indicator
#define SPI_REG_INT_ENABLE             0x0010      // Interrupt enable register
#define SPI_REG_INT_STATUS             0x0014      // Interrupt status
#define SPI_REG_TX_DATA                0x0020      // Transmit FIFO write
#define SPI_REG_RX_DATA                0x0024      // Receive FIFO read
#define SPI_REG_FIFO_CONTROL           0x0030      // FIFO threshold settings
#define SPI_REG_FIFO_STATUS            0x0034      // Current FIFO levels
#define SPI_REG_CLOCK_DIVIDER          0x0040      // SPI clock scaler

// SPI Control Bits
#define SPI_CTRL_RESET                 0x00000001  // Reset SPI engine
#define SPI_CTRL_ENABLE                0x00000002  // Enable SPI interface
#define SPI_CTRL_MASTER_MODE           0x00000004  // 1 = Master, 0 = Slave
#define SPI_STATUS_TX_FULL             0x00000001  // Transmit FIFO is full
#define SPI_STATUS_RX_EMPTY            0x00000002  // Receive FIFO is empty

// Interrupts
#define SPI_INT_RX_THRESHOLD           0x00000001  // RX FIFO threshold reached
#define SPI_INT_TX_EMPTY               0x00000002  // TX FIFO is empty
#define SPI_INT_ERROR                  0x00000004  // Frame/overrun error

#endif // _APPLE_A18PRO_INPUT_HW_H_
