#include "icmpv4.h"
#include "ethernet.h"
#include "ip.h"
#include "utils.h"
#include "sock.h"

static const struct ip_ops icmpv4_ip_ops = {
    .handler = &icmpv4_incoming,
};

void icmpv4_init() { ip_register_handler(ICMPV4, &icmpv4_ip_ops); }

/**
 * @brief 接收并处理icmpv4报文
 *
 */
void icmpv4_incoming(struct sk_buff *skb) {
        struct iphdr *iphdr = ip_hdr(skb);
        struct icmp_v4 *icmp = (struct icmp_v4 *)iphdr->data;

        // TODO: Check csum

        switch (icmp->type) {
        case ICMP_V4_ECHO:
                icmpv4_reply(skb);
                return;
        case ICMP_V4_DST_UNREACHABLE:
                print_err("ICMPv4 received 'dst unreachable' code %d, "
                          "check your routes and firewall rules\n",
                          icmp->code);
                goto drop_pkt;
        default:
                print_err("ICMPv4 did not match supported types\n");
                goto drop_pkt;
        }

drop_pkt:
        free_skb(skb);
        return;
}

/**
 * @brief icmpv4回显请求的应答处理
 */
void icmpv4_reply(struct sk_buff *skb) {
        struct iphdr *iphdr = ip_hdr(skb);
        struct icmp_v4 *icmp;
        struct sock sk;
        memset(&sk, 0, sizeof(struct sock));

        uint16_t icmp_len = iphdr->len - (iphdr->ihl * 4);

        skb_reserve(skb, ETH_HDR_LEN + IP_HDR_LEN + icmp_len);
        skb_push(skb, icmp_len);

        icmp = (struct icmp_v4 *)skb->data;

        icmp->type = ICMP_V4_REPLY;
        icmp->csum = 0;
        icmp->csum = checksum(icmp, icmp_len, 0);

        skb->protocol = ICMPV4;
        sk.daddr = iphdr->saddr;

        ip_output(&sk, skb);
        free_skb(skb);
}
