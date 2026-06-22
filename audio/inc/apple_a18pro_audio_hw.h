/*
 * Apple A18 Pro Audio Driver (MCA)
 * 
 * Hardware Register Definitions for Apple Multichannel Audio (MCA) Controller
 */

#ifndef _APPLE_A18PRO_AUDIO_HW_H_
#define _APPLE_A18PRO_AUDIO_HW_H_

#define APPLE_VENDOR_ID                0x106B
#define A18PRO_AUDIO_DEVICE_ID         0x16B2

// Memory-Mapped Register Base Space
#define MCA_MMIO_SIZE                  0x00010000  // 64KB MMIO space

// Registers
#define MCA_REG_VERSION                0x0000      // Controller version
#define MCA_REG_CONTROL                0x0004      // Core control
#define MCA_REG_STATUS                 0x0008      // Status indicator
#define MCA_REG_TX_CONTROL             0x0010      // Transmitter control
#define MCA_REG_RX_CONTROL             0x0014      // Receiver control
#define MCA_REG_TX_FIFO                0x0020      // Transmit FIFO write
#define MCA_REG_RX_FIFO                0x0024      // Receive FIFO read
#define MCA_REG_DMA_TX_ADDR_L          0x0030      // Audio TX DMA Buffer Low
#define MCA_REG_DMA_TX_ADDR_H          0x0034      // Audio TX DMA Buffer High
#define MCA_REG_DMA_TX_SIZE            0x0038      // TX buffer size
#define MCA_REG_DMA_TX_HEAD            0x003C      // TX current head pointer
#define MCA_REG_DAC_VOLUME_L           0x0050      // DAC Left channel volume
#define MCA_REG_DAC_VOLUME_R           0x0054      // DAC Right channel volume

// MCA Control Bits
#define MCA_CTRL_RESET                 0x00000001  // Reset controller
#define MCA_CTRL_ENABLE                0x00000002  // Enable controller
#define MCA_TX_ENABLE                  0x00000001  // Enable transmitter
#define MCA_RX_ENABLE                  0x00000001  // Enable receiver
#define MCA_DMA_TX_START               0x00000001  // Start TX DMA playback
#define MCA_DMA_TX_PAUSE               0x00000002  // Pause playback

#endif // _APPLE_A18PRO_AUDIO_HW_H_
