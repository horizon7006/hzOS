#ifndef TCP_H
#define TCP_H

#include "net.h"

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  data_offset; // Reserved + Data Offset
    uint8_t  flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

void tcp_receive(ipv4_header_t* ip, uint8_t* data, uint32_t len);
void tcp_send_packet(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port, uint32_t seq, uint32_t ack, uint8_t flags, uint8_t* data, uint32_t len);

#endif
