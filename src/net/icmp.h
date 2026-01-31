#ifndef ICMP_H
#define ICMP_H

#include "../core/common.h"
#include "net.h"

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
    uint8_t  data[];
} __attribute__((packed)) icmp_packet_t;

#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

void icmp_receive(uint8_t* src_mac, ipv4_header_t* ip, icmp_packet_t* icmp, uint32_t len);

#endif
