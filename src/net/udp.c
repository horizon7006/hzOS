#include "udp.h"
#include "../lib/memory.h"
#include "../lib/printf.h"

void udp_receive(ipv4_header_t* ip, uint8_t* data, uint32_t len) {
    if (len < sizeof(udp_header_t)) return;

    udp_header_t* udp = (udp_header_t*)data;
    uint16_t src_port = ntohs(udp->src_port);
    uint16_t dest_port = ntohs(udp->dest_port);
    uint16_t length = ntohs(udp->length);

    // Verify length
    if (length < sizeof(udp_header_t) || length > len) return;

    // uint8_t* payload = data + sizeof(udp_header_t); // Unused for now
    uint32_t payload_len = length - sizeof(udp_header_t);

    kprintf("UDP: Recv from %d.%d.%d.%d:%d to %d (len=%d)\n",
            (ip->src_ip >> 24) & 0xFF, (ip->src_ip >> 16) & 0xFF,
            (ip->src_ip >> 8) & 0xFF, ip->src_ip & 0xFF,
            src_port, dest_port, payload_len);

    // Handle specific ports here (e.g. DHCP, DNS)
}

void udp_send_packet(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port, uint8_t* data, uint32_t len) {
    uint32_t packet_len = sizeof(udp_header_t) + len;
    uint8_t* buffer = (uint8_t*)kmalloc(packet_len);
    udp_header_t* udp = (udp_header_t*)buffer;

    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(packet_len);
    udp->checksum = 0; // Optional in IPv4

    memcpy(buffer + sizeof(udp_header_t), data, len);

    // Find MAC for dest_ip (for now, simplistic or broadcast)
    // Real implementation needs ARP cache lookup. 
    // For now we assume typical behavior or need an interface that resolves MAC.
    // ip_send_packet takes dest_mac.
    
    // TEMPORARY: If we don't have ARP cache, we can't easily find MAC.
    // We will broadcast if local subnet, or send to Gateway MAC.
    // Since we don't have Gateway/Netmask config yet, let's just use Broadcast 
    // OR rely on a future ARP lookup helper.
    
    // For this step, I'll modify ip_send_packet to handle ARP resolution? 
    // No, ip_send_packet expects MAC.
    
    // Hack: Send to Broadcast for testing, unless we saw the IP recently.
    // But `net.c` has `handle_arp` but no storage.
    
    uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
    
    ip_send_packet(dest_mac, dest_ip, IP_PROTOCOL_UDP, buffer, packet_len);
    
    kfree(buffer);
}
