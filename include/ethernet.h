#ifndef ETHERNET_H_
#define ETHERNET_H_

#include "netdev.h"
#include "skbuff.h"
#include "syshead.h"
#include "utils.h"
#include <linux/if_ether.h>

#define ETH_HDR_LEN sizeof(struct eth_hdr)

#ifdef DEBUG_ETH
#define eth_dbg(msg, hdr)                                                      \
        do {                                                                   \
                print_debug(                                                   \
                    "eth " msg " ("                                            \
                    "dmac: %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx, "        \
                    "smac: %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx, "        \
                    "ethertype: %.4hx)",                                       \
                    hdr->dmac[0], hdr->dmac[1], hdr->dmac[2], hdr->dmac[3],    \
                    hdr->dmac[4], hdr->dmac[5], hdr->smac[0], hdr->smac[1],    \
                    hdr->smac[2], hdr->smac[3], hdr->smac[4], hdr->smac[5],    \
                    hdr->ethertype);                                           \
        } while (0)
#else
#define eth_dbg(msg, hdr)
#endif

struct sk_buff;
struct netdev;

uint8_t *skb_head(struct sk_buff *skb);

/**
 * @brief Ethernet header structure
 * ┌──────────┬──────────┬──────────┬──────────────────┬─────────┐
 * │ 目标 MAC │ 源 MAC   │ 类型     │ 数据 + 必要填充   │ FCS     │
 * │ 6 字节   │ 6 字节   │ 2 字节   │ 46～1500 字节     │ 4 字节  │
 * └──────────┴──────────┴──────────┴──────────────────┴─────────┘
 *
 * 2字节类型：(小端)
 *      ETH_P_ARP  0x0806
 *      ETH_P_IP   0x0800
 *      ETH_P_IPV6 0x86DD
 *
 * __attribute__((packed))
 * 是 GCC 的扩展，作用是：让结构体成员紧密排列，尽量消除对齐产生的填充字节
 */
struct eth_hdr {
        uint8_t dmac[6];
        uint8_t smac[6];
        uint16_t ethertype;
        uint8_t payload[];
} __attribute__((packed));

/**
 * @brief 从skb中获取以太网头结构体
 */
static inline struct eth_hdr *eth_hdr(struct sk_buff *skb) {
        struct eth_hdr *hdr = (struct eth_hdr *)skb_head(skb);

        /* 网络大端字节序转为主机小端字节序 */
        hdr->ethertype = ntohs(hdr->ethertype);

        return hdr;
}

#endif
