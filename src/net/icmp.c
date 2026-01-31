#include "icmp.h"
#include "../lib/memory.h"
#include "../lib/printf.h"

// Helper from net.c (might need to move to a header or make it shared)
uint16_t ip_checksum(void* data, size_t length);

void icmp_receive(uint8_t* src_mac, ipv4_header_t* ip, icmp_packet_t* icmp, uint32_t len) {
    if (len < sizeof(icmp_packet_t)) return;

    if (icmp->type == ICMP_TYPE_ECHO_REQUEST) {
        kprintf("icmp: received echo request from %d.%d.%d.%d\n",
                (ip->src_ip >> 0) & 0xFF, (ip->src_ip >> 8) & 0xFF,
                (ip->src_ip >> 16) & 0xFF, (ip->src_ip >> 24) & 0xFF);

        // Prepare Echo Reply
        uint8_t* buffer = (uint8_t*)kmalloc(len);
        icmp_packet_t* reply = (icmp_packet_t*)buffer;
        
        memcpy(reply, icmp, len);
        reply->type = ICMP_TYPE_ECHO_REPLY;
        reply->checksum = 0;
        reply->checksum = ip_checksum(reply, len);

        ip_send_packet(src_mac, ip->src_ip, IP_PROTOCOL_ICMP, (uint8_t*)reply, len);
    }
}
