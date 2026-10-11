#ifndef ARP_H
#define ARP_H
#include "ethernet.h"
#include "list.h"
#include "netdev.h"
#include "skbuff.h"
#include "syshead.h"
#include "utils.h"

#define ARP_ETHERNET 0x0001
#define ARP_IPV4 0x0800
#define ARP_REQUEST 0x0001
#define ARP_REPLY 0x0002

#define ARP_HDR_LEN sizeof(struct arp_hdr)
#define ARP_DATA_LEN sizeof(struct arp_ipv4)

/**
 * @brief ARP缓存的状态
 */
#define ARP_CACHE_LEN 32
#define ARP_FREE 0
#define ARP_WAITING 1
#define ARP_RESOLVED 2

#ifdef DEBUG_ARP
#define arp_dbg(str, hdr)                                                      \
        do {                                                                   \
                print_debug("arp " str " (hwtype: %hu, protype: %.4hx, "       \
                            "hwsize: %d, prosize: %d, opcode: %.4hx)",         \
                            hdr->hwtype, hdr->protype, hdr->hwsize,            \
                            hdr->prosize, hdr->opcode);                        \
        } while (0)

#define arpdata_dbg(str, data)                                                 \
        do {                                                                   \
                print_debug(                                                   \
                    "arp data " str " (smac: %.2hhx:%.2hhx:%.2hhx:%.2hhx"      \
                    ":%.2hhx:%.2hhx, sip: %hhu.%hhu.%hhu.%hhu, dmac: "         \
                    "%.2hhx:%.2hhx"                                            \
                    ":%.2hhx:%.2hhx:%.2hhx:%.2hhx, dip: %hhu.%hhu.%hhu.%hhu)", \
                    data->smac[0], data->smac[1], data->smac[2],               \
                    data->smac[3], data->smac[4], data->smac[5],               \
                    data->sip >> 24, data->sip >> 16, data->sip >> 8,          \
                    data->sip >> 0, data->dmac[0], data->dmac[1],              \
                    data->dmac[2], data->dmac[3], data->dmac[4],               \
                    data->dmac[5], data->dip >> 24, data->dip >> 16,           \
                    data->dip >> 8, data->dip >> 0);                           \
        } while (0)

#define arpcache_dbg(str, entry)                                               \
        do {                                                                   \
                print_debug(                                                   \
                    "arp cache " str                                           \
                    " (hwtype: %hu, sip: %hhu.%hhu.%hhu.%hhu, "                \
                    "smac: %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx, state: " \
                    "%d)",                                                     \
                    entry->hwtype, entry->sip >> 24, entry->sip >> 16,         \
                    entry->sip >> 8, entry->sip >> 0, entry->smac[0],          \
                    entry->smac[1], entry->smac[2], entry->smac[3],            \
                    entry->smac[4], entry->smac[5], entry->state);             \
        } while (0)
#else
#define arp_dbg(str, hdr)
#define arpdata_dbg(str, data)
#define arpcache_dbg(str, entry)
#endif

/**
 * @brief ARP头
 *
 * ARP（地址解析协议）用于在局域网中根据 IPv4 地址查询 MAC 地址。
 * 常见的以太网 IPv4 ARP 报文头为 28 字节，不包含外层以太网帧头。
 *
 * hwtype  2字节  以太网为0x0001
 * protype 2字节  IPv4为0x0800
 * hwsize  1字节  硬件地址长度 以太网为6
 * prosize 1字节  协议地址长度 IPv4为4
 * opcode  2字节  ARP请求为0x0001 ARP应答为0x0002
 * data    20字节 ARP数据部分 包含
 *                发送方MAC 6字节
 *                发送方IP 4字节
 *                目标MAC 6字节，请求时填充为0
 *                目标IP 4字节
 */
struct arp_hdr {
        uint16_t hwtype;
        uint16_t protype;
        uint8_t hwsize;
        uint8_t prosize;
        uint16_t opcode;
        unsigned char data[];
} __attribute__((packed));

/**
 * @brief ARP IPv4
 */
struct arp_ipv4 {
        unsigned char smac[6];
        uint32_t sip;
        unsigned char dmac[6];
        uint32_t dip;
} __attribute__((packed));

/**
 * @brief ARP 缓存条目结构体，包含硬件类型、IP 地址、MAC 地址及状态。
 */
struct arp_cache_entry {
        struct list_head list;
        uint16_t hwtype;
        uint32_t sip;
        unsigned char smac[6];
        unsigned int state;
};

void arp_init(void);
void free_arp();
int arp_rcv(struct sk_buff *skb);
void arp_reply(struct sk_buff *skb, struct netdev *netdev);
int arp_request(uint32_t sip, uint32_t dip, struct netdev *netdev);
unsigned char *arp_get_hwaddr(uint32_t sip);

static inline struct arp_hdr *arp_hdr(struct sk_buff *skb) {
        return (struct arp_hdr *)(skb->head + ETH_HDR_LEN);
}

#endif
