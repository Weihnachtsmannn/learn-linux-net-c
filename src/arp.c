#include "arp.h"
#include "list.h"
#include "netdev.h"
#include "skbuff.h"

/**
 * @brief broadcast_hw 广播mac地址
 * @brief arp_cache arp缓存
 * @brief lock 互斥锁，用于保护ARP缓存的并发访问
 */
static uint8_t broadcast_hw[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static LIST_HEAD(arp_cache);
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static struct sk_buff *arp_alloc_skb() {
        struct sk_buff *skb =
            alloc_skb(ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);
        skb_reserve(skb, ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);
        skb->protocol = htons(ETH_P_ARP);

        return skb;
}

static struct arp_cache_entry *arp_entry_alloc(struct arp_hdr *hdr,
                                               struct arp_ipv4 *data) {
        struct arp_cache_entry *entry = malloc(sizeof(struct arp_cache_entry));
        list_init(&entry->list);

        entry->state = ARP_RESOLVED; /* 初始化为已解析状态 */
        entry->hwtype = hdr->hwtype;
        entry->sip = data->sip;
        memcpy(entry->smac, data->smac, sizeof(entry->smac));

        return entry;
}

static int insert_arp_translation_table(struct arp_hdr *hdr,
                                        struct arp_ipv4 *data) {
        struct arp_cache_entry *entry = arp_entry_alloc(hdr, data);

        pthread_mutex_lock(&lock);
        list_add_tail(&entry->list, &arp_cache);
        pthread_mutex_unlock(&lock);

        return 0;
}

static int update_arp_translation_table(struct arp_hdr *hdr,
                                        struct arp_ipv4 *data) {
        struct list_head *item;
        struct arp_cache_entry *entry;

        pthread_mutex_lock(&lock);
        list_for_each(item, &arp_cache) {
                entry = list_entry(item, struct arp_cache_entry, list);

                if (entry->hwtype == hdr->hwtype && entry->sip == data->sip) {
                        memcpy(entry->smac, data->smac, 6);
                        pthread_mutex_unlock(&lock);

                        return 1;
                }
        }

        pthread_mutex_unlock(&lock);

        return 0;
}

static struct netdev_protocol netdev_arp_proto = {
    .ethertype = ETH_P_ARP,
    .on_rcv = arp_rcv,
};

void arp_init(void) { netdev_register_protocol(&netdev_arp_proto); }

/**
 * @brief 处理接收到的ARP请求报文，更新ARP缓存并根据请求类型生成响应。
 */
int arp_rcv(struct sk_buff *skb) {
        struct arp_hdr *arphdr;   /* arp头 */
        struct arp_ipv4 *arpdata; /* ipv4 数据部分 */
        struct netdev *netdev;    /* 网络设备接口 */
        int merge = 0;

        /* 解析ARP头 */
        arphdr = arp_hdr(skb);
        arphdr->hwtype = ntohs(arphdr->hwtype);
        arphdr->protype = ntohs(arphdr->protype);
        arphdr->opcode = ntohs(arphdr->opcode);
        arp_dbg("in", arphdr);
        if (arphdr->hwtype != ARP_ETHERNET) {
                printf("ARP: Unsupported HW type\n");
                goto drop_pkt;
        }
        if (arphdr->protype != ARP_IPV4) {
                printf("ARP: Unsupported protocol\n");
                goto drop_pkt;
        }

        /* 解析ARP数据部分 */
        arpdata = (struct arp_ipv4 *)arphdr->data;
        arpdata->sip = ntohl(arpdata->sip);
        arpdata->dip = ntohl(arpdata->dip);
        arpdata_dbg("receive", arpdata);

        /* 收到arp请求广播就更新一次ARP缓存 */
        merge = update_arp_translation_table(arphdr, arpdata);

        /* arp广播的请求目的ip不是本机，丢弃 */
        if (!(netdev = netdev_get(arpdata->dip))) {
                printf("ARP was not for us\n");
                goto drop_pkt;
        }

        /* 没有arp缓存项，插入新的ARP缓存项 */
        if (!merge && insert_arp_translation_table(arphdr, arpdata) != 0) {
                print_err("ERR: No free space in ARP translation table\n");
                goto drop_pkt;
        }

        switch (arphdr->opcode) {
        case ARP_REQUEST:
                arp_reply(skb, netdev);
                return 0;
        default:
                printf("ARP: Opcode not supported\n");
                goto drop_pkt;
        }

drop_pkt:
        free_skb(skb);
        return -1;
}

int arp_request(uint32_t sip, uint32_t dip, struct netdev *netdev) {
        struct sk_buff *skb;
        struct arp_hdr *arp;
        struct arp_ipv4 *payload;
        int rc = 0;

        /* 分配ARP请求报文的skb，包括以太网帧头、ARP头和ARP数据部分 */
        skb = arp_alloc_skb();
        if (!skb)
                return -1;

        skb->dev = netdev;

        /* 填充ARP数据部分 */
        payload = (struct arp_ipv4 *)skb_push(skb, ARP_DATA_LEN);
        memcpy(payload->smac, netdev->hwaddr, netdev->addr_len);
        payload->sip = sip;
        memcpy(payload->dmac, broadcast_hw, netdev->addr_len);
        payload->dip = dip;

        /* 填充ARP头部 */
        arp = (struct arp_hdr *)skb_push(skb, ARP_HDR_LEN);
        arp_dbg("req", arp);
        arp->opcode = htons(ARP_REQUEST);
        arp->hwtype = htons(ARP_ETHERNET);
        arp->protype = htons(ETH_P_IP);
        arp->hwsize = netdev->addr_len;
        arp->prosize = 4;

        /* 将ARP数据部分的IP地址转换为网络字节序 */
        arpdata_dbg("req", payload);
        payload->sip = htonl(payload->sip);
        payload->dip = htonl(payload->dip);

        /* 对arp报文封装以太网帧，通过广播的方式从TAP虚拟网卡发出 */
        rc = netdev_transmit(skb, broadcast_hw, ETH_P_ARP);
        free_skb(skb);
        return rc;
}

/**
 * @brief 生成ARP应答报文并发送给请求方
 * @attention 这个项目中arp请求和arp响应复用了同一个skb实例
 *            但是linux内核会为ARP请求和ARP响应分配不同的skb实例
 */
void arp_reply(struct sk_buff *skb, struct netdev *netdev) {
        struct arp_hdr *arphdr;
        struct arp_ipv4 *arpdata;

        arphdr = arp_hdr(skb);

        skb_reserve(skb, ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);
        skb_push(skb, ARP_HDR_LEN + ARP_DATA_LEN);

        /* 提取ARP数据部分 */
        arpdata = (struct arp_ipv4 *)arphdr->data;
        memcpy(arpdata->dmac, arpdata->smac, 6);
        arpdata->dip = arpdata->sip;
        memcpy(arpdata->smac, netdev->hwaddr, 6);
        arpdata->sip = netdev->addr;

        /* 设置ARP报文的操作码为ARP应答 */
        arphdr->opcode = ARP_REPLY;

        arp_dbg("reply", arphdr);
        /* 将ARP报文头和数据部分的字段转换为网络字节序 */
        arphdr->opcode = htons(arphdr->opcode);
        arphdr->hwtype = htons(arphdr->hwtype);
        arphdr->protype = htons(arphdr->protype);

        arpdata_dbg("reply", arpdata);
        /* 将ARP数据部分的IP地址转换为网络字节序 */
        arpdata->sip = htonl(arpdata->sip);
        arpdata->dip = htonl(arpdata->dip);

        skb->dev = netdev;

        /* 通过网络设备发送ARP应答报文 */
        netdev_transmit(skb, arpdata->dmac, ETH_P_ARP);
        free_skb(skb);
}

/**
 * @brief 获取指定IP地址对应的硬件地址（MAC地址）
 */
unsigned char *arp_get_hwaddr(uint32_t sip) {
        struct list_head *item;
        struct arp_cache_entry *entry;

        pthread_mutex_lock(&lock);
        list_for_each(item, &arp_cache) {
                entry = list_entry(item, struct arp_cache_entry, list);

                if (entry->state == ARP_RESOLVED && entry->sip == sip) {
                        arpcache_dbg("entry", entry);

                        uint8_t *copy = entry->smac;
                        pthread_mutex_unlock(&lock);

                        return copy;
                }
        }

        pthread_mutex_unlock(&lock);

        return NULL;
}

void free_arp() {
        struct list_head *item, *tmp;
        struct arp_cache_entry *entry;

        list_for_each_safe(item, tmp, &arp_cache) {
                entry = list_entry(item, struct arp_cache_entry, list);
                list_del(item);

                free(entry);
        }
}
