#include "ip.h"
#include "netdev.h"
#include "skbuff.h"
#include "syshead.h"
#include "utils.h"

#define IP_PROTOCOL_COUNT 256
static const struct ip_ops *ip_operations[IP_PROTOCOL_COUNT];

static int ip_dispatch(struct sk_buff *skb, unsigned int proto) {
        const struct ip_ops *ops;
        if (proto >= IP_PROTOCOL_COUNT)
                return -1;

        ops = ip_operations[proto];
        if (ops && ops->handler) {
                ops->handler(skb);
                return 0;
        } else {
                return -1;
        }
}

int ip_register_handler(unsigned int proto, const struct ip_ops *ops) {
        if (proto >= IP_PROTOCOL_COUNT || !ops || !ops->handler)
                return -1;

        /* Check if a handler is already registered for this protocol */
        if (ip_operations[proto])
                return -1;

        ip_operations[proto] = ops;
        return 0;
}

static void ip_init_pkt(struct iphdr *ih) {
        ih->saddr = ntohl(ih->saddr);
        ih->daddr = ntohl(ih->daddr);
        ih->len = ntohs(ih->len);
        ih->id = ntohs(ih->id);
}

int ip_rcv(struct sk_buff *skb) {
        struct iphdr *ih = ip_hdr(skb);
        uint16_t csum = -1;

        /* 检查是否为ipv4报文 */
        if (ih->version != IPV4) {
                print_err("Datagram version was not IPv4\n");
                goto drop_pkt;
        }

        /* 检查首部长度：IHL以4字节为单位，最小为5，即20字节 */
        if (ih->ihl < 5) {
                print_err("IPv4 header length must be at least 5\n");
                goto drop_pkt;
        }

        /* time to live */
        if (ih->ttl == 0) {
                // TODO: Send ICMP error
                print_err("Time to live of datagram reached 0\n");
                goto drop_pkt;
        }

        /* 校验IPv4首部，包含首部选项；结果为0表示校验通过 */
        csum = checksum(ih, ih->ihl * 4, 0);
        if (csum != 0) {
                // Invalid checksum, drop packet handling
                goto drop_pkt;
        }

        // TODO: Check fragmentation, possibly reassemble
        /* 检查分片，并在需要时重组 */

        /* 将总长度、标识、源IP和目的IP转换为主机字节序 */
        ip_init_pkt(ih);

        /* 打印ip首部信息 */
        ip_dbg("in", ih);

        /* 交给上层协议处理 5. */
        if (ip_dispatch(skb, ih->proto) < 0) {
                print_err("No handler for IP protocol %u\n", ih->proto);
                goto drop_pkt;
        }
        return 0;

        // switch (ih->proto) {
        // case ICMPV4:
        //         icmpv4_incoming(skb);
        //         return 0;
        // case IP_TCP:
        //         tcp_in(skb);
        //         return 0;
        // default:
        //         // print_err("Unknown IP header proto\n");
        //         goto drop_pkt;
        // }

drop_pkt:
        free_skb(skb);
        return 0;
}

static struct netdev_protocol netdev_ip_proto = {
    .ethertype = ETH_P_IP,
    .on_rcv = ip_rcv,
};

int ip_init(void) { return netdev_register_protocol(&netdev_ip_proto); }