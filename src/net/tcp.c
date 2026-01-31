#include "tcp.h"
#include "../lib/memory.h"
#include "../lib/printf.h"

// Pseudo Header for Checksum Calculation
typedef struct {
    uint32_t src_ip;
    uint32_t dest_ip;
    uint8_t  zeros;
    uint8_t  protocol;
    uint16_t tcp_len;
} __attribute__((packed)) tcp_pseudo_header_t;

void tcp_receive(ipv4_header_t* ip, uint8_t* data, uint32_t len) {
    if (len < sizeof(tcp_header_t)) return;

    tcp_header_t* tcp = (tcp_header_t*)data;
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    
    // Calculate Data Offset (Header Length)
    uint32_t header_len = ((tcp->data_offset >> 4) * 4);
    if (header_len < sizeof(tcp_header_t) || header_len > len) return;
    
    uint8_t* payload = data + header_len;
    uint32_t payload_len = len - header_len;

    kprintf("TCP: Recv from %d.%d.%d.%d:%d to %d Flags=%x (len=%d)\n",
            (ip->src_ip >> 24) & 0xFF, (ip->src_ip >> 16) & 0xFF,
            (ip->src_ip >> 8) & 0xFF, ip->src_ip & 0xFF,
            src_port, dest_port, tcp->flags, payload_len);

    // Basic Echo Server Logic on Port 80
    if (dest_port == 80) {
        if (tcp->flags & TCP_FLAG_SYN) {
            kprintf("TCP: SYN received. Sending SYN-ACK...\n");
            tcp_send_packet(ip->src_ip, src_port, dest_port, 
                            0, // My Seq (Should be random)
                            ntohl(tcp->seq) + 1, // Ack = Their Seq + 1
                            TCP_FLAG_SYN | TCP_FLAG_ACK, 
                            NULL, 0);
        } else if (tcp->flags & TCP_FLAG_ACK) {
             if (payload_len > 0) {
                 // Echo data back
                 kprintf("TCP: Data received: %d bytes. Echoing...\n", payload_len);
                 tcp_send_packet(ip->src_ip, src_port, dest_port,
                                 ntohl(tcp->ack), // My Seq = Their Ack
                                 ntohl(tcp->seq) + payload_len, // Ack = Their Seq + len
                                 TCP_FLAG_ACK | TCP_FLAG_PSH,
                                 payload, payload_len);
             }
        }
    }
}

void tcp_send_packet(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port, uint32_t seq, uint32_t ack, uint8_t flags, uint8_t* data, uint32_t len) {
    uint32_t packet_len = sizeof(tcp_header_t) + len;
    uint8_t* buffer = (uint8_t*)kmalloc(packet_len);
    tcp_header_t* tcp = (tcp_header_t*)buffer;
    
    memset(buffer, 0, packet_len);

    tcp->src_port = htons(src_port);
    tcp->dest_port = htons(dest_port);
    tcp->seq = htonl(seq);
    tcp->ack = htonl(ack);
    tcp->data_offset = (sizeof(tcp_header_t) / 4) << 4;
    tcp->flags = flags;
    tcp->window_size = htons(8192); // 8KB Window
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;

    if (len > 0) {
        memcpy(buffer + sizeof(tcp_header_t), data, len);
    }

    // Calculate Checksum with Pseudo Header
    // Need buffer large enough for Pseudo Header + TCP Segment
    uint32_t pseudo_len = sizeof(tcp_pseudo_header_t) + packet_len;
    uint8_t* checksum_buffer = (uint8_t*)kmalloc(pseudo_len);
    
    tcp_pseudo_header_t* pseudo = (tcp_pseudo_header_t*)checksum_buffer;
    pseudo->src_ip = htonl(net_get_ip());
    pseudo->dest_ip = htonl(dest_ip);
    pseudo->zeros = 0;
    pseudo->protocol = 6; // TCP Protocol ID
    pseudo->tcp_len = htons(packet_len);
    
    memcpy(checksum_buffer + sizeof(tcp_pseudo_header_t), buffer, packet_len);
    
    // Fix: ip_checksum assumes word alignment padding if odd length
    tcp->checksum = ip_checksum(checksum_buffer, pseudo_len);
    
    kfree(checksum_buffer);

    // Send via IP
    // TODO: ARP Lookup needed here too. Using Broadcast for now as hack.
    uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
    ip_send_packet(dest_mac, dest_ip, 6 /* TCP */, buffer, packet_len);
    
    kfree(buffer);
}
