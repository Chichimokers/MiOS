#ifndef MIOS_NET_H
#define MIOS_NET_H

#include "lib/types.h"

/*
 * MiOS - modulo de red.
 *
 * Sondea las interfaces (ethernet, wifi, loopback), sus direcciones y las
 * rutas usando netlink. Tambien consulta /sys para conocer el estado.
 */

#define NET_MAX_IFACES 16
#define NET_MAX_ADDRS   16
#define NET_MAX_ROUTES  32
#define NET_NAME_LEN    16
#define NET_ADDR_LEN    46
#define NET_HWADDR_LEN  8

enum net_link_type {
    NET_LINK_UNKNOWN = 0,
    NET_LINK_ETHERNET,
    NET_LINK_WIFI,
    NET_LINK_LOOPBACK
};

struct net_addr {
    int  family;
    char text[NET_ADDR_LEN];
    int  prefixlen;
};

struct net_iface {
    int      index;
    char     name[NET_NAME_LEN];
    int      type;
    unsigned flags;
    unsigned mtu;
    u8       hwaddr[NET_HWADDR_LEN];
    int      hwaddr_len;
    int      addr_count;
    struct net_addr addrs[NET_MAX_ADDRS];
};

struct net_route {
    int  family;
    char dst[NET_ADDR_LEN];
    char gateway[NET_ADDR_LEN];
    int  ifindex;
    int  metric;
};

struct net_info {
    int iface_count;
    struct net_iface ifaces[NET_MAX_IFACES];
    int route_count;
    struct net_route routes[NET_MAX_ROUTES];
};

/* Sondeo completo (src/net/net.c) */
int  net_probe(struct net_info *info);

struct net_iface *net_iface_by_index(struct net_info *info, int index);
struct net_iface *net_iface_by_name(struct net_info *info, const char *name);

const char *net_type_name(int type);
const char *net_state_name(const struct net_iface *iface);

void net_format_addr(int family, const u8 *addr, char *out, mios_size_t size);
void net_format_hwaddr(const u8 *hw, int len, char *out, mios_size_t size);

/* Comandos (src/net/cmd_net.c) */
void cmd_net_register(void);

#endif /* MIOS_NET_H */
