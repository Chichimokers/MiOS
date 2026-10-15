#include "mios.h"
#include "net/netlink.h"

/*
 * MiOS - modulo de red: sondeo de interfaces, direcciones y rutas.
 */

static void build_sys_path(char *out, mios_size_t size,
                           const char *name, const char *leaf) {
    const char *prefix = "/sys/class/net/";
    mios_size_t i = 0;
    const char *p;

    for (p = prefix; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    for (p = name; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    if (leaf != MIOS_NULL) {
        for (p = leaf; *p != '\0' && i + 1 < size; p++) {
            out[i] = *p;
            i++;
        }
    }
    out[i] = '\0';
}

static void detect_type(struct net_iface *iface) {
    char path[160];
    int fd;

    if (iface->flags & IFF_LOOPBACK) {
        iface->type = NET_LINK_LOOPBACK;
        return;
    }

    if (iface->name[0] == '\0') {
        iface->type = NET_LINK_UNKNOWN;
        return;
    }

    build_sys_path(path, sizeof(path), iface->name, "/wireless");
    fd = mios_open(path, MIOS_O_RDONLY, 0);
    if (fd >= 0) {
        mios_close(fd);
        iface->type = NET_LINK_WIFI;
    } else {
        iface->type = NET_LINK_ETHERNET;
    }
}

int net_probe(struct net_info *info) {
    int r;
    int i;

    mios_memset(info, 0, sizeof(*info));

    r = netlink_dump_links(info);
    if (r < 0) {
        return r;
    }

    for (i = 0; i < info->iface_count; i++) {
        detect_type(&info->ifaces[i]);
    }

    (void)netlink_dump_addrs(info);
    (void)netlink_dump_routes(info);
    return 0;
}

struct net_iface *net_iface_by_index(struct net_info *info, int index) {
    int i;
    for (i = 0; i < info->iface_count; i++) {
        if (info->ifaces[i].index == index) {
            return &info->ifaces[i];
        }
    }
    return MIOS_NULL;
}

struct net_iface *net_iface_by_name(struct net_info *info, const char *name) {
    int i;
    for (i = 0; i < info->iface_count; i++) {
        if (mios_strcmp(info->ifaces[i].name, name) == 0) {
            return &info->ifaces[i];
        }
    }
    return MIOS_NULL;
}

const char *net_type_name(int type) {
    switch (type) {
    case NET_LINK_ETHERNET:
        return "ethernet";
    case NET_LINK_WIFI:
        return "wifi";
    case NET_LINK_LOOPBACK:
        return "loopback";
    default:
        return "desconocido";
    }
}

const char *net_state_name(const struct net_iface *iface) {
    if (!(iface->flags & IFF_UP)) {
        return "apagada";
    }
    if (iface->flags & IFF_RUNNING) {
        return "activa";
    }
    return "sin enlace";
}

static mios_size_t append_uint(char *out, mios_size_t size, mios_size_t pos,
                               unsigned long v) {
    char tmp[24];
    int n = 0;

    if (v == 0) {
        tmp[n] = '0';
        n++;
    }
    while (v > 0) {
        tmp[n] = (char)('0' + (v % 10));
        v /= 10;
        n++;
    }
    while (n > 0 && pos + 1 < size) {
        n--;
        out[pos] = tmp[n];
        pos++;
    }
    return pos;
}

static mios_size_t append_hex16(char *out, mios_size_t size, mios_size_t pos,
                                unsigned v) {
    const char *hx = "0123456789abcdef";
    char tmp[4];
    int n = 0;

    if (v == 0) {
        tmp[n] = '0';
        n++;
    }
    while (v > 0) {
        tmp[n] = hx[v & 0xf];
        v >>= 4;
        n++;
    }
    while (n > 0 && pos + 1 < size) {
        n--;
        out[pos] = tmp[n];
        pos++;
    }
    return pos;
}

void net_format_addr(int family, const u8 *addr, char *out, mios_size_t size) {
    mios_size_t pos = 0;
    int i;

    if (size == 0) {
        return;
    }

    if (family == MIOS_AF_INET) {
        for (i = 0; i < 4; i++) {
            pos = append_uint(out, size, pos, addr[i]);
            if (i < 3 && pos + 1 < size) {
                out[pos] = '.';
                pos++;
            }
        }
    } else if (family == MIOS_AF_INET6) {
        for (i = 0; i < 8; i++) {
            unsigned v = ((unsigned)addr[i * 2] << 8) | addr[i * 2 + 1];
            pos = append_hex16(out, size, pos, v);
            if (i < 7 && pos + 1 < size) {
                out[pos] = ':';
                pos++;
            }
        }
    } else {
        if (pos + 1 < size) {
            out[pos] = '?';
            pos++;
        }
    }
    out[pos] = '\0';
}

void net_format_hwaddr(const u8 *hw, int len, char *out, mios_size_t size) {
    const char *hx = "0123456789abcdef";
    mios_size_t pos = 0;
    int i;

    if (size == 0) {
        return;
    }
    for (i = 0; i < len; i++) {
        if (i > 0 && pos + 1 < size) {
            out[pos] = ':';
            pos++;
        }
        if (pos + 2 < size) {
            out[pos] = hx[(hw[i] >> 4) & 0xf];
            pos++;
            out[pos] = hx[hw[i] & 0xf];
            pos++;
        }
    }
    out[pos] = '\0';
}
