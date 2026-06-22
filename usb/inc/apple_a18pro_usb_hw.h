/*
 * Apple A18 Pro USB Driver (DWC3)
 * 
 * Hardware Register Definitions for Apple USB (DWC3) Controller
 */

#ifndef _APPLE_A18PRO_USB_HW_H_
#define _APPLE_A18PRO_USB_HW_H_

#define APPLE_VENDOR_ID                0x106B
#define A18PRO_USB_DEVICE_ID           0x16B3

// Memory-Mapped Register Base Space
#define USB_MMIO_SIZE                  0x00010000  // 64KB MMIO space

// Registers
#define USB_REG_VERSION                0x0000      // Controller version
#define USB_REG_CONTROL                0x0004      // Core control
#define USB_REG_STATUS                 0x0008      // Status indicator
#define USB_REG_PHY_CONTROL            0x0010      // USB PHY status/control
#define USB_REG_PHY_STATUS             0x0014      // USB PHY operational state
#define USB_REG_POWER_STATE            0x0020      // Host power domain control

// Synopsys DWC3 Core Base (usually offset by 0xC000 in MMIO)
#define DWC3_CORE_OFFSET               0xC000
#define DWC3_REG_GCTL                  (DWC3_CORE_OFFSET + 0x0110) // Global Control
#define DWC3_REG_GUSB2PHYCFG(n)        (DWC3_CORE_OFFSET + 0x0200 + ((n) * 4))
#define DWC3_REG_GUSB3PIPECTL(n)       (DWC3_CORE_OFFSET + 0x02C0 + ((n) * 4))

// xHCI Operational Registers (offset by 0x0100 in MMIO)
#define XHCI_OPERATIONAL_OFFSET        0x0100
#define XHCI_REG_USBCMD                (XHCI_OPERATIONAL_OFFSET + 0x0000) // USB Command
#define XHCI_REG_USBSTS                (XHCI_OPERATIONAL_OFFSET + 0x0004) // USB Status
#define XHCI_REG_PAGESIZE              (XHCI_OPERATIONAL_OFFSET + 0x0008) // Page Size
#define XHCI_REG_DNCTRL                (XHCI_OPERATIONAL_OFFSET + 0x0014) // Device Notification Control

// USB Control Bits
#define USB_CTRL_RESET                 0x00000001  // Reset controller
#define USB_CTRL_ENABLE                0x00000002  // Enable controller
#define USB_PHY_RESET                  0x00000001  // Reset USB physical layer
#define USB_PHY_POWER_ON               0x00000002  // Enable PHY power

#define XHCI_CMD_RUN                   0x00000001  // Run/Stop xHCI
#define XHCI_CMD_RESET                 0x00000002  // Reset xHCI
#define XHCI_STS_HALT                  0x00000001  // Host controller Halted

#endif // _APPLE_A18PRO_USB_HW_H_
