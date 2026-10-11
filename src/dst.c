#include "dst.h"
#include "arp.h"
#include "ip.h"
#include "syshead.h"

int dst_neigh_output(struct sk_buff *skb) {
        struct iphdr *iphdr = ip_hdr(skb);
        struct netdev *netdev = skb->dev;
        struct rtentry *rt = skb->rt;
        uint32_t daddr = ntohl(iphdr->daddr);
        uint32_t saddr = ntohl(iphdr->saddr);

        uint8_t *dmac;

        if (rt->flags & RT_GATEWAY) {
                daddr = rt->gateway;
        }

        /* 获取目标MAC地址 */
        dmac = arp_get_hwaddr(daddr);

        if (dmac) {
                return netdev_transmit(skb, dmac, ETH_P_IP);
        } else {
                /* 没有获取到目标MAC地址，发送arp请求*/
                arp_request(saddr, daddr, netdev);

                /* Inform upper layer that traffic was not sent, retry later */
                return -1;
        }
}
