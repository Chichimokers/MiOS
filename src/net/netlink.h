#ifndef MIOS_NET_NETLINK_H
#define MIOS_NET_NETLINK_H

#include "lib/types.h"

/*
 * MiOS - ABI de netlink (NETLINK_ROUTE) y utilidades de mensajes.
 *
 * Solo se usan syscalls: socket/bind/sendto/recvfrom. No hay libc ni
 * librerias externas de red.
 */

/* Familias de direcciones y protocolos */
#define MIOS_AF_UNSPEC   0
#define MIOS_AF_INET     2
#define MIOS_AF_INET6    10
#define MIOS_AF_NETLINK  16

#define MIOS_SOCK_RAW    3

#define MIOS_NETLINK_ROUTE 0

/* Flags de mensaje netlink */
#define NLM_F_REQUEST 0x01
#define NLM_F_MULTI   0x02
#define NLM_F_ACK     0x04
#define NLM_F_ROOT    0x100
#define NLM_F_MATCH   0x200
#define NLM_F_DUMP    (NLM_F_ROOT | NLM_F_MATCH)

/* Tipos de mensaje netlink */
#define NLMSG_NOOP    1
#define NLMSG_ERROR   2
#define NLMSG_DONE    3

/* Tipos de mensaje de ruta */
#define RTM_NEWLINK   16
#define RTM_DELLINK   17
#define RTM_GETLINK   18
#define RTM_NEWADDR   20
#define RTM_DELADDR   21
#define RTM_GETADDR   22
#define RTM_NEWROUTE  24
#define RTM_DELROUTE  25
#define RTM_GETROUTE  26

/* Atributos de interfaz (IFLA_*) */
#define IFLA_ADDRESS  1
#define IFLA_IFNAME   3
#define IFLA_MTU      4

/* Atributos de direccion (IFA_*) */
#define IFA_ADDRESS   1
#define IFA_LOCAL     2

/* Atributos de ruta (RTA_*) */
#define RTA_DST       1
#define RTA_OIF       4
#define RTA_GATEWAY   5
#define RTA_PRIORITY  6

/* Tablas de enrutado */
#define RT_TABLE_MAIN 254

/* Flags de interfaz (IFF_*) */
#define IFF_UP        0x1
#define IFF_BROADCAST 0x2
#define IFF_LOOPBACK  0x8
#define IFF_RUNNING   0x40

/* Alineacion de mensajes */
#define NLMSG_ALIGNTO 4
#define NLMSG_ALIGN(len) (((len) + NLMSG_ALIGNTO - 1) & ~(NLMSG_ALIGNTO - 1))
#define NLMSG_HDRLEN ((int)NLMSG_ALIGN(sizeof(struct nlmsghdr)))
#define NLMSG_LENGTH(len) ((len) + NLMSG_HDRLEN)
#define NLMSG_DATA(nlh) ((void *)(((char *)(nlh)) + NLMSG_HDRLEN))
#define NLMSG_OK(nlh, len) ((len) >= (int)sizeof(struct nlmsghdr) && \
                            (nlh)->nlmsg_len >= sizeof(struct nlmsghdr) && \
                            (nlh)->nlmsg_len <= (u32)(len))
#define NLMSG_NEXT(nlh, len) ((len) -= (int)NLMSG_ALIGN((nlh)->nlmsg_len), \
                              (struct nlmsghdr *)(((char *)(nlh)) + \
                              NLMSG_ALIGN((nlh)->nlmsg_len)))

#define RTA_ALIGNTO 4
#define RTA_ALIGN(len) (((len) + RTA_ALIGNTO - 1) & ~(RTA_ALIGNTO - 1))
#define RTA_LENGTH(len) (RTA_ALIGN(sizeof(struct rtattr)) + (len))
#define RTA_DATA(rta) ((void *)(((char *)(rta)) + RTA_ALIGN(sizeof(struct rtattr))))
#define RTA_OK(rta, len) ((len) >= (int)sizeof(struct rtattr) && \
                          (rta)->rta_len >= sizeof(struct rtattr) && \
                          (rta)->rta_len <= (u32)(len))
#define RTA_NEXT(rta, len) ((len) -= (int)RTA_ALIGN((rta)->rta_len), \
                            (struct rtattr *)(((char *)(rta)) + \
                            RTA_ALIGN((rta)->rta_len)))

struct sockaddr_nl {
    u16 nl_family;
    u16 nl_pad;
    u32 nl_pid;
    u32 nl_groups;
};

struct nlmsghdr {
    u32 nlmsg_len;
    u16 nlmsg_type;
    u16 nlmsg_flags;
    u32 nlmsg_seq;
    u32 nlmsg_pid;
};

struct nlmsgerr {
    int error;
    struct nlmsghdr msg;
};

struct rtattr {
    u16 rta_len;
    u16 rta_type;
};

struct ifinfomsg {
    u8  ifi_family;
    u8  __ifi_pad;
    u16 ifi_type;
    int ifi_index;
    u32 ifi_flags;
    u32 ifi_change;
};

struct ifaddrmsg {
    u8  ifa_family;
    u8  ifa_prefixlen;
    u8  ifa_flags;
    u8  ifa_scope;
    u32 ifa_index;
};

struct rtmsg {
    u8  rtm_family;
    u8  rtm_dst_len;
    u8  rtm_src_len;
    u8  rtm_tos;
    u8  rtm_table;
    u8  rtm_protocol;
    u8  rtm_scope;
    u8  rtm_type;
    u32 rtm_flags;
};

struct net_info;

/* Volcados de netlink (src/net/netlink.c) */
int netlink_dump_links(struct net_info *info);
int netlink_dump_addrs(struct net_info *info);
int netlink_dump_routes(struct net_info *info);
int netlink_set_link_up(int ifindex, int up);

#endif /* MIOS_NET_NETLINK_H */
