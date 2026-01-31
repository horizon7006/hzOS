#include "net.h"
#include "udp.h"
#include "tcp.h"
#include "../drivers/rtl8139.h"
#include "../lib/memory.h"
#include "../lib/printf.h"

static uint8_t  local_mac[6];
static uint32_t local_ip;
static uint8_t  broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void net_init(uint8_t mac[6], uint32_t ip) {
    memcpy(local_mac, mac, 6);
    local_ip = ip;
    kprintf("net: initialized with IP %d.%d.%d.%d\n", 
            (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

void net_get_mac(uint8_t mac[6]) {
    memcpy(mac, local_mac, 6);
}

uint32_t net_get_ip(void) {
    return local_ip;
}

uint16_t ip_checksum(void* vdata, size_t length) {
    uint8_t* data = (uint8_t*)vdata;
    uint32_t acc = 0xffff;
    for (size_t i = 0; i + 1 < length; i += 2) {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
        if (acc > 0xffff) acc -= 0xffff;
    }
    if (length & 1) {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += ntohs(word);
        if (acc > 0xffff) acc -= 0xffff;
    }
    return htons(~acc);
}

void net_send_packet(uint8_t* dest_mac, uint16_t ethertype, uint8_t* data, uint32_t len) {
    uint8_t* buffer = (uint8_t*)kmalloc(sizeof(ethernet_frame_t) + len);
    ethernet_frame_t* frame = (ethernet_frame_t*)buffer;
    
    memcpy(frame->dest_mac, dest_mac, 6);
    memcpy(frame->src_mac, local_mac, 6);
    frame->type = htons(ethertype);
    
    memcpy(buffer + sizeof(ethernet_frame_t), data, len);
    
    rtl8139_send_packet(buffer, sizeof(ethernet_frame_t) + len);
    // Note: kmalloc is a bump allocator, we could ideally reuse a buffer here
}

static void handle_arp(arp_packet_t* arp, uint32_t len) {
    if (len < sizeof(arp_packet_t)) return;

    if (ntohs(arp->opcode) == ARP_OPCODE_REQUEST) {
        if (arp->dest_ip == htonl(local_ip)) {
            kprintf("net: responding to ARP request for our IP\n");
            
            arp_packet_t reply;
            reply.hardware_type = htons(1); // Ethernet
            reply.protocol_type = htons(ETHERTYPE_IPV4);
            reply.hardware_addr_len = 6;
            reply.protocol_addr_len = 4;
            reply.opcode = htons(ARP_OPCODE_REPLY);
            
            memcpy(reply.src_mac, local_mac, 6);
            reply.src_ip = htonl(local_ip);
            memcpy(reply.dest_mac, arp->src_mac, 6);
            reply.dest_ip = arp->src_ip;
            
            net_send_packet(arp->src_mac, ETHERTYPE_ARP, (uint8_t*)&reply, sizeof(arp_packet_t));
        }
    } else if (ntohs(arp->opcode) == ARP_OPCODE_REPLY) {
        kprintf("net: received ARP reply from %d.%d.%d.%d (%x:%x:%x:%x:%x:%x)\n",
                (arp->src_ip >> 24) & 0xFF, (arp->src_ip >> 16) & 0xFF,
                (arp->src_ip >> 8) & 0xFF, arp->src_ip & 0xFF,
                arp->src_mac[0], arp->src_mac[1], arp->src_mac[2],
                arp->src_mac[3], arp->src_mac[4], arp->src_mac[5]);
    }
}

void arp_send_request(uint32_t target_ip) {
    arp_packet_t req;
    req.hardware_type = htons(1);
    req.protocol_type = htons(ETHERTYPE_IPV4);
    req.hardware_addr_len = 6;
    req.protocol_addr_len = 4;
    req.opcode = htons(ARP_OPCODE_REQUEST);
    
    memcpy(req.src_mac, local_mac, 6);
    req.src_ip = htonl(local_ip);
    memset(req.dest_mac, 0, 6);
    req.dest_ip = target_ip;
    
    net_send_packet(broadcast_mac, ETHERTYPE_ARP, (uint8_t*)&req, sizeof(arp_packet_t));
}

#include "icmp.h"

void ip_send_packet(uint8_t* dest_mac, uint32_t dest_ip, uint8_t protocol, uint8_t* data, uint32_t len) {
    uint8_t* buffer = (uint8_t*)kmalloc(sizeof(ipv4_header_t) + len);
    ipv4_header_t* ip = (ipv4_header_t*)buffer;

    ip->version_ihl = 0x45; // Version 4, Header Length 5 (20 bytes)
    ip->tos = 0;
    ip->len = htons(sizeof(ipv4_header_t) + len);
    ip->id = htons(1);
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src_ip = htonl(local_ip);
    ip->dest_ip = dest_ip;
    ip->checksum = ip_checksum(ip, sizeof(ipv4_header_t));

    memcpy(buffer + sizeof(ipv4_header_t), data, len);

    net_send_packet(dest_mac, ETHERTYPE_IPV4, buffer, sizeof(ipv4_header_t) + len);
}

void net_receive(uint8_t* data, uint32_t len) {
    if (len < sizeof(ethernet_frame_t)) return;
    
    ethernet_frame_t* frame = (ethernet_frame_t*)data;
    uint16_t type = ntohs(frame->type);
    uint8_t* payload = data + sizeof(ethernet_frame_t);
    uint32_t payload_len = len - sizeof(ethernet_frame_t);

    if (type == ETHERTYPE_ARP) {
        handle_arp((arp_packet_t*)payload, payload_len);
    } else if (type == ETHERTYPE_IPV4) {
        if (payload_len < sizeof(ipv4_header_t)) return;
        
        ipv4_header_t* ip = (ipv4_header_t*)payload;
        uint32_t ip_hdr_len = (ip->version_ihl & 0x0F) * 4;
        uint8_t* ip_payload = payload + ip_hdr_len;
        uint32_t ip_payload_len = ntohs(ip->len) - ip_hdr_len;

        if (ip->protocol == IP_PROTOCOL_ICMP) {
            ethernet_frame_t* eth = (ethernet_frame_t*)data;
            icmp_receive(eth->src_mac, ip, (icmp_packet_t*)ip_payload, ip_payload_len);
        } else if (ip->protocol == IP_PROTOCOL_UDP) {
            udp_receive(ip, ip_payload, ip_payload_len);
        } else if (ip->protocol == 6) { // TCP Protocol ID
            tcp_receive(ip, ip_payload, ip_payload_len);
        }
    }
}
