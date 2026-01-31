#ifndef RTL8139_H
#define RTL8139_H

#include "../core/common.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

/* Registers */
#define RTL_REG_MAC0     0x00
#define RTL_REG_MAC4     0x04
#define RTL_REG_RBSTART  0x30
#define RTL_REG_CMD      0x37
#define RTL_REG_CAPR     0x38
#define RTL_REG_IMR      0x3C
#define RTL_REG_ISR      0x3E
#define RTL_REG_TCR      0x40
#define RTL_REG_RCR      0x44
#define RTL_REG_MPC      0x4C
#define RTL_REG_CONFIG1  0x52

/* Command Register Bits */
#define RTL_CMD_RESET    0x10
#define RTL_CMD_RE       0x08
#define RTL_CMD_TE       0x04
#define RTL_CMD_BUFE     0x01

/* Interrupt Status/Mask Register Bits */
#define RTL_INT_ROK      0x01
#define RTL_INT_TER      0x02
#define RTL_INT_RER      0x04
#define RTL_INT_TUV      0x08
#define RTL_INT_RXOVW    0x10
#define RTL_INT_PUN      0x20

/* Receive Configuration Register Bits */
#define RTL_RCR_AAP      0x01    /* Accept All Packets */
#define RTL_RCR_APM      0x02    /* Accept Physical Match */
#define RTL_RCR_AM       0x04    /* Accept Multicast */
#define RTL_RCR_AB       0x08    /* Accept Broadcast */
#define RTL_RCR_WRAP     0x80    /* Wrap around */

/* TX Status Registers */
#define RTL_REG_TSD0     0x10
#define RTL_REG_TSAD0    0x20

#define RX_BUF_SIZE      8192
#define RX_BUF_PAD       16
#define RX_BUF_TOTAL     (RX_BUF_SIZE + RX_BUF_PAD + 2048) // Extra padding for safety

void rtl8139_init(void);
void rtl8139_send_packet(void* data, uint32_t len);

#endif
