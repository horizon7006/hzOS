#ifndef UDP_H
#define UDP_H

#include "net.h"

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

void udp_receive(ipv4_header_t* ip, uint8_t* data, uint32_t len);
void udp_send_packet(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port, uint8_t* data, uint32_t len);

#endif
