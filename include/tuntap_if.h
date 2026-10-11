#ifndef TUNTAP_IF_H
#define TUNTAP_IF_H

void tun_init();                   /* 申请一个TAP虚拟网卡 */
int tun_read(char *buf, int len);  /* 从TAP读取以太网帧 */
int tun_write(char *buf, int len); /* 向TAP写入以太网帧 */
void free_tun();                   /* 释放TAP虚拟网卡 */

#endif
