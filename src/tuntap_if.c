#include "basic.h"
#include "syshead.h"
#include "utils.h"

static int tun_fd; /* 通过tun_fd读写虚拟网卡 */
static char *dev;  /* 虚拟网卡的名称 */

char *tapaddr = "10.0.0.5";
char *taproute = "10.0.0.0/24";

/**
 * ip route add dev tap0 10.0.0.0/24
 * 发往10.0.0.0/24的流量通过tap0接口出去
 */
static int set_if_route(char *dev, char *cidr) {
        // return run_cmd("ip route add dev %s %s", dev, cidr);
        return run_cmd("ip route replace %s dev %s", cidr, dev);
}

/**
 * ip address add dev tap0 local 10.0.0.5
 * 给内核tap0这个虚拟网卡添加ip地址
 */
static int set_if_address(char *dev, char *cidr) {
        return run_cmd("ip address add dev %s local %s", dev, cidr);
}

/**
 * ip link set dev tap0 up
 * 将 tap0 接口打开，允许它参与网络通信。
 */
static int set_if_up(char *dev) {
        return run_cmd("ip link set dev %s up", dev);
}

/*
 * Taken from Kernel Documentation/networking/tuntap.txt
 */
static int tun_alloc(char *dev) {
        struct ifreq ifr;
        int fd, err;

        if ((fd = open("/dev/net/tap", O_RDWR)) < 0) {
                perror("Cannot open TUN/TAP dev\n"
                       "Make sure one exists with "
                       "'$ mknod /dev/net/tap c 10 200'");
                exit(1);
        }

        CLEAR(ifr);

        /**
         * Flags: IFF_TUN   - TUN device (no Ethernet headers)
         *        IFF_TAP   - TAP device
         *
         *        IFF_NO_PI - Do not provide packet information
         *
         * @brief TUN 和 TAP 是 Linux
         *        提供的两种虚拟网络接口，让用户态程序能够直接收发网络数据
         *        普通网卡通过硬件收发数据，TUN/TAP 则与程序交换数据
         *        物理网卡：Linux 内核 ↔ 网卡硬件 ↔ 网线
         *        TUN/TAP：Linux 内核 ↔ 虚拟网卡 ↔ 用户态程序
         * @note 它们的主要区别
         *       TUN: 三层数据（IP包）
         *       TAP: 二层数据（以太网帧）
         *       这里选择TAP是因为要处理以太网帧和ARP，
         *       需要包含MAC地址的二层数据
         */
        ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
        if (*dev) {
                strncpy(ifr.ifr_name, dev, IFNAMSIZ);
        }

        /**
         * @brief ioctl 让linux内核准备一张TAP虚拟网卡，
         *        并把打开的fd关联到这张网卡上，供程序收发数据
         * @param fd 打开的 TUN/TAP 驱动文件描述符
         * @param TUNSETIFF 指定执行创建或关联 TUN/TAP 接口的操作
         * @param ifr 指向请求结构体的指针
         * @note 这里ifr.ifr_name由内核生成，并设置给dev
         */
        if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
                perror("ERR: Could not ioctl tun");
                close(fd);
                return err;
        }

        strcpy(dev, ifr.ifr_name);
        return fd;
}

/**
 * @attention TAP保留报文边界，在缓冲区足够大的情况下，
 *            一次成功的read()读取一个以太网帧，不会
 *            像TCP字节流那样把几个帧拼在一起。
 */
int tun_read(char *buf, int len) { return read(tun_fd, buf, len); }

int tun_write(char *buf, int len) { return write(tun_fd, buf, len); }

/**
 * @brief 创建一个带有以太网帧的虚拟TAP网卡
 * @param dev      虚拟TAP网卡的名称
 * @param taproute 虚拟TAP网卡的路由信息
 * @param tapaddr  虚拟TAP网卡的IP地址信息
 *
 * 查看linux网卡命令 ip link show 或 ifconfig
 * 查看路由信息      ip route show
 *
 * 对于这个程序可以使用以下命令
 * ip -d link show tap0
 * ip address show dev tap0
 * ip route show dev tap0
 * ip route get 10.0.0.4
 */
void tun_init() {
        dev = calloc(10, 1);     /* 向linux内核申请的虚拟网卡的名称 */
        tun_fd = tun_alloc(dev); /* 读写虚拟网卡的文件描述符 */

        /* 打开虚拟网卡 */
        if (set_if_up(dev) != 0) {
                print_err("ERROR when setting up if\n");
        }

        /* 设置默认路由 */
        if (set_if_route(dev, taproute) != 0) {
                print_err("ERROR when setting route for if\n");
        }

        /* 设置虚拟网卡的IP地址 */
        if (set_if_address(dev, tapaddr) != 0) {
                print_err("ERROR when setting addr for if\n");
        }
}

void free_tun() { free(dev); }
