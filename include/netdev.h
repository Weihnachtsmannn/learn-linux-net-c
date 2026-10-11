#ifndef NETDEV_H
#define NETDEV_H
#include <stdint.h>
#include "ethernet.h"
// #include "skbuff.h"
#include "syshead.h"
#include "utils.h"

#define BUFLEN 1600
#define MAX_ADDR_LEN 32

#define netdev_dbg(fmt, args...)                                               \
        do {                                                                   \
                print_debug("NETDEV: " fmt, ##args);                           \
        } while (0)

struct eth_hdr;
struct sk_buff;
struct netdev_ops {
        uint16_t ethertype;
        int (*handler)(struct sk_buff *skb);

        /* 这里用链表保存 EtherType 与处理函数的对应关系，
           避免为 16 位 EtherType分配一个 65536 项数组。 */
        struct netdev_ops *next;
};

int netdev_register(struct netdev_ops *ops);

struct netdev {
        uint32_t addr;
        uint8_t addr_len;
        uint8_t hwaddr[6];
        uint32_t mtu;
        struct netdev_ops *ops;
};

void netdev_init();
void free_netdev();

struct netdev *netdev_get(uint32_t sip);
int netdev_transmit(struct sk_buff *skb, uint8_t *dst, uint16_t ethertype);
void *netdev_rx_loop(void *);

#endif
