#include "netdev.h"
#include "basic.h"
#include "ethernet.h"
#include "skbuff.h"
#include "syshead.h"
#include "tuntap_if.h"
#include "utils.h"

/* 不应依赖上层 */
// #include "arp.h"
// #include "ip.h"

static struct netdev_ops *netdev_list;
int netdev_register(struct netdev_ops *ops) {
        struct netdev_ops *item;
        if (!ops || !ops->handler)
                return -1;

        /* 已经注册过该ethertype */
        for (item = netdev_list; item; item = item->next) {
                if (item->ethertype == ops->ethertype) {
                        return -1;
                }
        }

        ops->next = netdev_list;
        netdev_list = ops;
        return 0;
}

struct netdev *loop;   /* 本地回环接口 */
struct netdev *netdev; /* 对外通信接口 */
extern int running;

static struct netdev *netdev_alloc(char *addr, char *hwaddr, uint32_t mtu) {
        struct netdev *dev = malloc(sizeof(struct netdev));

        dev->addr = ip_parse(addr);

        sscanf(hwaddr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dev->hwaddr[0],
               &dev->hwaddr[1], &dev->hwaddr[2], &dev->hwaddr[3],
               &dev->hwaddr[4], &dev->hwaddr[5]);

        dev->addr_len = 6;
        dev->mtu = mtu;

        return dev;
}

// void netdev_init(char *addr, char *hwaddr) {
void netdev_init() {
        loop = netdev_alloc("127.0.0.1", "00:00:00:00:00:00", 1500);
        netdev = netdev_alloc("10.0.0.4", "00:0c:29:6d:50:25", 1500);
}

/**
 * @brief 封装并发送以太网帧到TAP虚拟网卡
 *
 * @param skb       要发送的以太网帧缓冲区
 * @param dst_hw    目标硬件地址
 * @param ethertype 以太网类型，ip或arp
 * @return int 发送结果，0表示成功，非0表示失败
 */
int netdev_transmit(struct sk_buff *skb, uint8_t *dst_hw, uint16_t ethertype) {
        struct netdev *dev;
        struct eth_hdr *hdr;
        int ret = 0;

        dev = skb->dev;

        /* 构建以太网帧头，封装以太网帧 */
        skb_push(skb, ETH_HDR_LEN);
        hdr = (struct eth_hdr *)skb->data;
        memcpy(hdr->dmac, dst_hw, dev->addr_len);
        memcpy(hdr->smac, dev->hwaddr, dev->addr_len);
        hdr->ethertype = htons(ethertype);
        eth_dbg("out", hdr);

        /* 发送以太网帧到TAP虚拟网卡 */
        ret = tun_write((char *)skb->data, skb->len);

        return ret;
}

static void netdev_receive(struct sk_buff *skb) {
        struct eth_hdr *hdr = eth_hdr(skb);
        struct netdev_ops *item;
        eth_dbg("in", hdr);

        // switch (hdr->ethertype) {
        // case ETH_P_ARP:
        //         arp_rcv(skb);
        //         break;
        // case ETH_P_IP:
        //         ip_rcv(skb);
        //         break;
        // case ETH_P_IPV6:
        // default:
        //         // printf("Unsupported ethertype %x\n", hdr->ethertype);
        //         free_skb(skb);
        //         break;
        // }

        for (item = netdev_list; item; item = item->next) {
                if (item->ethertype == hdr->ethertype) {
                        item->handler(skb);
                        return;
                }
        }

        free_skb(skb);
}

/**
 * @brief TAP虚拟网卡处理线程阻塞接收以太网帧
 */
void *netdev_rx_loop(void *unused) {
        (void)unused;

        while (running) {
                /* 预分配用来填充以太网帧的缓冲区 */
                struct sk_buff *skb = alloc_skb(BUFLEN);

                /* 阻塞等待TAP网卡数据，如果帧大于缓冲区超出的部分会被丢弃，
                 * 下次读取的是下一帧，这里值判断失败的情况，
                 * 1600字节的缓冲区大小足够了 */
                if (tun_read((char *)skb->data, BUFLEN) < 0) {
                        perror("ERR: Read from tun_fd");
                        free_skb(skb);
                        return NULL;
                }

                /* 分发arp ipv4 */
                netdev_receive(skb);
        }

        return NULL;
}

struct netdev *netdev_get(uint32_t sip) {
        if (netdev->addr == sip) {
                return netdev;
        } else {
                return NULL;
        }
}

void free_netdev() {
        free(loop);
        free(netdev);
}
