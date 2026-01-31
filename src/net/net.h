#ifndef NET_H
#define NET_H

#include "../core/common.h"

/* Ethernet II Frame Header */
typedef struct {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t type;
} __attribute__((packed)) ethernet_frame_t;

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_ARP  0x0806

/* ARP Packet */
typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t  hardware_addr_len;
    uint8_t  protocol_addr_len;
    uint16_t opcode;
    uint8_t  src_mac[6];
    uint32_t src_ip;
    uint8_t  dest_mac[6];
    uint32_t dest_ip;
} __attribute__((packed)) arp_packet_t;

#define ARP_OPCODE_REQUEST 1
#define ARP_OPCODE_REPLY   2

/* IPv4 Header */
typedef struct {
    uint8_t  version_ihl;
    uint8_t  tos;
    uint16_t len;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ipv4_header_t;

#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_UDP  17

/* Core Functions */
void net_init(uint8_t mac[6], uint32_t ip);
void net_receive(uint8_t* data, uint32_t len);
void net_send_packet(uint8_t* dest_mac, uint16_t ethertype, uint8_t* data, uint32_t len);
void ip_send_packet(uint8_t* dest_mac, uint32_t dest_ip, uint8_t protocol, uint8_t* data, uint32_t len);
uint16_t ip_checksum(void* vdata, size_t length);

void net_get_mac(uint8_t mac[6]);
uint32_t net_get_ip(void);
void arp_send_request(uint32_t target_ip);

/* Helper: Swap 16/32 bit networking byte order (big-endian) */
static inline uint16_t ntohs(uint16_t n) {
    return ((n & 0xFF) << 8) | ((n & 0xFF00) >> 8);
}

static inline uint16_t htons(uint16_t n) {
    return ntohs(n);
}

static inline uint32_t ntohl(uint32_t n) {
    return ((n & 0xFF) << 24) | ((n & 0xFF00) << 8) | ((n & 0xFF0000) >> 8) | ((n & 0xFF000000) >> 24);
}

static inline uint32_t htonl(uint32_t n) {
    return ntohl(n);
}

#endif
