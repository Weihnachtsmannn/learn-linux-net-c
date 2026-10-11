#ifndef NETDEV_H
#define NETDEV_H

#include "ethernet.h"
#include "list.h"
#include "syshead.h"
#include "utils.h"

#define BUFLEN 1600
#define MAX_ADDR_LEN 32

#define netdev_dbg(fmt, args...)                                               \
        do {                                                                   \
                print_debug("NETDEV: " fmt, ##args);                           \
        } while (0)

struct sk_buff; // #include "skbuff.h"
struct netdev_protocol {
        uint16_t ethertype; /* 主机字节序 */
        int (*on_rcv)(struct sk_buff *skb);

        /* 这里用链表保存 EtherType 与处理函数的对应关系，
           避免为 16 位 EtherType分配一个 65536 项数组。 */
        struct list_head list;
};

int netdev_register_protocol(struct netdev_protocol *ops);

struct netdev {
        uint32_t addr;
        uint8_t addr_len;
        uint8_t hwaddr[6];
        uint32_t mtu;
};

void netdev_init();
void free_netdev();

struct netdev *netdev_get(uint32_t sip);
int netdev_transmit(struct sk_buff *skb, uint8_t *dst, uint16_t ethertype);
void *netdev_rx_loop(void *);

#endif
