#ifndef ICMPV4_H
#define ICMPV4_H

#include "skbuff.h"
#include "syshead.h"

/**
 * @brief icmpv4报文类型定义
 * 0 回显应答
 * 3 目的不可达
 * 5 重定向
 * 8 回显请求
 * 11 超时
 */
#define ICMP_V4_REPLY 0x00
#define ICMP_V4_DST_UNREACHABLE 0x03
#define ICMP_V4_SRC_QUENCH 0x04
#define ICMP_V4_REDIRECT 0x05
#define ICMP_V4_ECHO 0x08
#define ICMP_V4_ROUTER_ADV 0x09
#define ICMP_V4_ROUTER_SOL 0x0a
#define ICMP_V4_TIMEOUT 0x0b
#define ICMP_V4_MALFORMED 0x0c

/**
 * @brief icmpv4头部
 * @param type ICMPv4报文类型
 * @param code ICMPv4报文代码，对type进一步分类
 * @param csum 校验和，校验整个icmp报文，包含首部和数据
 * @param data 可变长度数据
 */
struct icmp_v4 {
        uint8_t type;
        uint8_t code;
        uint16_t csum;
        uint8_t data[];
} __attribute__((packed));

struct icmp_v4_echo {
        uint16_t id;
        uint16_t seq;
        uint8_t data[];
} __attribute__((packed));

struct icmp_v4_dst_unreachable {
        uint8_t unused;
        uint8_t len;
        uint16_t var;
        uint8_t data[];
} __attribute__((packed));

void icmpv4_init();
void icmpv4_incoming(struct sk_buff *skb);
void icmpv4_reply(struct sk_buff *skb);

#endif
