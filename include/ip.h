#ifndef IPV4_H
#define IPV4_H

#include "ethernet.h"
#include "skbuff.h"
// #include "sock.h" 2.
#include "syshead.h"

#define IPV4 0x04
#define IP_TCP 0x06
#define ICMPV4 0x01

#define IP_HDR_LEN sizeof(struct iphdr)
#define ip_len(ip) (ip->len - (ip->ihl * 4))

#ifdef DEBUG_IP
#define ip_dbg(msg, hdr)                                                       \
        do {                                                                   \
                print_debug(                                                   \
                    "ip " msg " (ihl: %hhu version: %hhu tos: %hhu "           \
                    "len %hu id: %hu frag_offset: %hu ttl: %hhu "              \
                    "proto: %hhu csum: %hx "                                   \
                    "saddr: %hhu.%hhu.%hhu.%hhu daddr: %hhu.%hhu.%hhu.%hhu)",  \
                    hdr->ihl, hdr->version, hdr->tos, hdr->len, hdr->id,       \
                    hdr->frag_offset, hdr->ttl, hdr->proto, hdr->csum,         \
                    hdr->saddr >> 24, hdr->saddr >> 16, hdr->saddr >> 8,       \
                    hdr->saddr >> 0, hdr->daddr >> 24, hdr->daddr >> 16,       \
                    hdr->daddr >> 8, hdr->daddr >> 0);                         \
        } while (0)
#else
#define ip_dbg(msg, hdr)
#endif

// 1.
struct sk_buff;
struct sock;
struct ip_ops {
        void (*handler)(struct sk_buff *skb);
};

int ip_register_handler(unsigned int proto, const struct ip_ops *ops);

/**
 * @brief IPv4头部结构体定义
 *
 *IPv4 Packet Format (each row = 32 bits / 4 bytes)
 *
 *  0       4       8               16    19          24            31
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|  IHL  |   DSCP        |        Total Length           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         Identification        |Flags|     Fragment Offset     |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  Time to Live |    Protocol   |         Header Checksum       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                        Source Address                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      Destination Address                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Options and Padding                        |
 *  |                         (optional)                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                             Data                              |
 *  |                              ...                              |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @param ihl 首部长度，单位为4字节，最小值为5，表示20字节的固定首部长度
 * @param version 当前报文使用哪个IP版本，4:IPv4 6:IPv6
 * @param tos 区分服务，用来获得更好的服务，只有在使用区分服务时才起作用
 * @param len 数据报的总长度，包括首部和数据之和
 *            首部包括固定长度的20字节和可选的options字段。
 * @param id ipv4在存储器中维持一个计数器，每生产一个数据报，计数器+1，
 *           并设置该字段。
 * @param frag_offset 3bit 标志，只有2bit有意义。
 *                    最低位为1，表示后面还有分片，0表示最后一个数据
 *                    中间一位为1，表示不能分片，为0才允许分片。
 *                    最高位保留，必须为0。
 *                    13bit 片偏移
 *                    指出较长的分组在分片后，该片在原分组中的相对位置，
 *                    以8字节为单位。
 * @param ttl 生存时间，表示数据报在网络中的生命，功能是跳数限制
 * @param proto ipv4数据部分的上层是什么协议 1:ICMP 6:TCP 17:UDP
 * @param csum 报文头部校验和
 *             数据报每经过一个设备，设备都要重新计算一下首部检验和，
 *             若首部未发生变化，则此结果必为0，于是就保留这个数据报。
 *             这个字段只检验数据报的首部，但不包括数据部分。
 *             首部 = 20字节固定长度 + 可变长度的options字段
 * @param saddr 报文发送方的ip地址
 * @param daddr 报文接收方的ip地址
 */
struct iphdr {
        uint8_t ihl : 4; /* TODO: Support Big Endian hosts */
        uint8_t version : 4;
        uint8_t tos;
        uint16_t len;
        uint16_t id;
        uint16_t frag_offset;
        uint8_t ttl;
        uint8_t proto;
        uint16_t csum;
        uint32_t saddr;
        uint32_t daddr;
        uint8_t data[];
} __attribute__((packed));

static inline struct iphdr *ip_hdr(const struct sk_buff *skb) {
        return (struct iphdr *)(skb->head + ETH_HDR_LEN);
}

int ip_init(void);
int ip_rcv(struct sk_buff *skb);
int ip_output(struct sock *sk, struct sk_buff *skb);

#endif
