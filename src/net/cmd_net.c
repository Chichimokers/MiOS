#include "mios.h"
#include "net/netlink.h"

/*
 * MiOS - comandos de red: net, routes, wifi, netup, netdown.
 */

static struct net_info net_data;

static int net_refresh(void) {
    int r = net_probe(&net_data);
    if (r < 0) {
        cons_printf("net: %s\n", mios_strerror(mios_error(r)));
    }
    return r;
}

static void build_stat_path(char *out, mios_size_t size,
                            const char *name, const char *leaf) {
    const char *prefix = "/sys/class/net/";
    const char *mid = "/statistics/";
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
    for (p = mid; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    for (p = leaf; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    out[i] = '\0';
}

static unsigned long read_stat(const char *ifname, const char *leaf) {
    char path[192];
    char buf[64];
    long n;

    if (ifname[0] == '\0') {
        return 0;
    }
    build_stat_path(path, sizeof(path), ifname, leaf);
    n = file_read(path, buf, sizeof(buf));
    if (n <= 0) {
        return 0;
    }
    return mios_strtoul(buf, 10);
}

static void print_iface_summary(const struct net_iface *iface) {
    char hw[NET_HWADDR_LEN * 3 + 1];
    int a;

    cons_printf("%-12s %-10s %-10s mtu %u\n",
                iface->name, net_type_name(iface->type),
                net_state_name(iface), iface->mtu);

    if (iface->hwaddr_len > 0) {
        net_format_hwaddr(iface->hwaddr, iface->hwaddr_len, hw, sizeof(hw));
        cons_printf("             MAC %s\n", hw);
    }
    if (iface->addr_count == 0) {
        cons_puts("             (sin direcciones)\n");
    } else {
        for (a = 0; a < iface->addr_count; a++) {
            cons_printf("             %s/%d\n",
                        iface->addrs[a].text, iface->addrs[a].prefixlen);
        }
    }
}

static void print_iface_detail(const struct net_iface *iface) {
    char hw[NET_HWADDR_LEN * 3 + 1];
    int a;

    cons_printf("\n===== %s =====\n", iface->name);
    cons_printf("Tipo:      %s\n", net_type_name(iface->type));
    cons_printf("Estado:    %s\n", net_state_name(iface));
    cons_printf("Indice:    %d\n", iface->index);
    cons_printf("MTU:       %u\n", iface->mtu);

    if (iface->hwaddr_len > 0) {
        net_format_hwaddr(iface->hwaddr, iface->hwaddr_len, hw, sizeof(hw));
        cons_printf("MAC:       %s\n", hw);
    }
    for (a = 0; a < iface->addr_count; a++) {
        cons_printf("Direccion: %s/%d\n",
                    iface->addrs[a].text, iface->addrs[a].prefixlen);
    }

    cons_printf("RX bytes:  %lu\n", read_stat(iface->name, "rx_bytes"));
    cons_printf("TX bytes:  %lu\n", read_stat(iface->name, "tx_bytes"));
    cons_puts("===================\n\n");
}

static void cmd_net(char *args) {
    char *tokens[1];
    int i;

    if (net_refresh() < 0) {
        return;
    }

    if (mios_split_words(args, tokens, 1) == 1) {
        struct net_iface *iface = net_iface_by_name(&net_data, tokens[0]);
        if (iface == MIOS_NULL) {
            cons_printf("net: interfaz '%s' no encontrada\n", tokens[0]);
            return;
        }
        print_iface_detail(iface);
        return;
    }

    cons_puts("\n===== RED =====\n");
    if (net_data.iface_count == 0) {
        cons_puts("No se detectaron interfaces.\n\n");
        return;
    }
    for (i = 0; i < net_data.iface_count; i++) {
        print_iface_summary(&net_data.ifaces[i]);
    }
    cons_puts("\n");
}

static void cmd_routes(char *args) {
    int i;
    (void)args;

    if (net_refresh() < 0) {
        return;
    }

    cons_puts("\n===== RUTAS =====\n");
    if (net_data.route_count == 0) {
        cons_puts("(sin rutas)\n\n");
        return;
    }
    for (i = 0; i < net_data.route_count; i++) {
        const struct net_route *r = &net_data.routes[i];
        struct net_iface *iface = net_iface_by_index(&net_data, r->ifindex);
        const char *dev = (iface != MIOS_NULL) ? iface->name : "-";

        cons_printf("%-22s %-22s dev %s\n",
                    (r->dst[0] != '\0') ? r->dst : "default",
                    (r->gateway[0] != '\0') ? r->gateway : "-",
                    dev);
    }
    cons_puts("\n");
}

static void cmd_wifi(char *args) {
    int i;
    int found = 0;
    (void)args;

    if (net_refresh() < 0) {
        return;
    }

    cons_puts("\n===== WIFI =====\n");
    for (i = 0; i < net_data.iface_count; i++) {
        const struct net_iface *iface = &net_data.ifaces[i];
        if (iface->type != NET_LINK_WIFI) {
            continue;
        }
        found = 1;
        print_iface_summary(iface);
    }
    if (!found) {
        cons_puts("No se detectaron interfaces wifi.\n");
        cons_puts("(usa 'net' para ver todas las interfaces)\n");
    }
    cons_puts("\n");
}

static void set_link(char *args, int up) {
    char *tokens[1];
    struct net_iface *iface;
    int r;

    if (mios_split_words(args, tokens, 1) != 1) {
        cons_printf("Uso: %s <interfaz>\n", up ? "netup" : "netdown");
        return;
    }

    if (net_refresh() < 0) {
        return;
    }

    iface = net_iface_by_name(&net_data, tokens[0]);
    if (iface == MIOS_NULL) {
        cons_printf("net: interfaz '%s' no encontrada\n", tokens[0]);
        return;
    }

    r = netlink_set_link_up(iface->index, up);
    if (r < 0) {
        cons_printf("%s: %s: %s\n", up ? "netup" : "netdown",
                    iface->name, mios_strerror(mios_error(r)));
        return;
    }

    cons_printf("%s: %s %s\n", up ? "netup" : "netdown",
                iface->name, up ? "activada" : "apagada");
}

static void cmd_netup(char *args) {
    set_link(args, 1);
}

static void cmd_netdown(char *args) {
    set_link(args, 0);
}

static void build_sys_net_path(char *out, mios_size_t size,
                               const char *name, const char *leaf) {
    const char *prefix = "/sys/class/net/";
    const char *p;
    mios_size_t i = 0;

    for (p = prefix; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    for (p = name; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    for (p = leaf; *p != '\0' && i + 1 < size; p++) {
        out[i] = *p;
        i++;
    }
    out[i] = '\0';
}

static void read_trimmed(const char *ifname, const char *leaf,
                         char *out, mios_size_t size) {
    char path[192];
    long n;
    mios_size_t i;

    build_sys_net_path(path, sizeof(path), ifname, leaf);
    out[0] = '\0';
    n = file_read(path, out, size);
    if (n <= 0) {
        out[0] = '\0';
        return;
    }
    i = mios_strlen(out);
    while (i > 0 && (out[i - 1] == '\n' || out[i - 1] == '\r' ||
                     out[i - 1] == ' ' || out[i - 1] == '\t')) {
        i--;
    }
    out[i] = '\0';
}

static void cmd_dev(char *args) {
    int fd;
    char buf[4096];
    long nread;
    (void)args;

    cons_puts("\n===== /sys/class/net =====\n");
    fd = mios_open("/sys/class/net", MIOS_O_RDONLY, 0);
    if (fd < 0) {
        cons_printf("dev: /sys/class/net: %s\n",
                    mios_strerror(mios_error(fd)));
        return;
    }
    for (;;) {
        long pos = 0;
        nread = mios_getdents64(fd, buf, sizeof(buf));
        if (nread <= 0) {
            break;
        }
        while (pos < nread) {
            struct mios_dirent64 *d = (struct mios_dirent64 *)(buf + pos);
            if (d->d_name[0] != '.') {
                char mac[64];
                char mtu[16];
                char state[32];
                read_trimmed(d->d_name, "/address", mac, sizeof(mac));
                read_trimmed(d->d_name, "/mtu", mtu, sizeof(mtu));
                read_trimmed(d->d_name, "/operstate", state, sizeof(state));
                cons_printf("  %-12s mac=%-18s mtu=%-6s state=%s\n",
                            d->d_name,
                            mac[0] != '\0' ? mac : "-",
                            mtu[0] != '\0' ? mtu : "-",
                            state[0] != '\0' ? state : "-");
            }
            pos += d->d_reclen;
        }
    }
    mios_close(fd);
    cons_puts("==========================\n\n");
}

static const command_t net_commands[] = {
    { "net",     "Estado de las interfaces de red", cmd_net },
    { "routes",  "Tabla de rutas",                  cmd_routes },
    { "wifi",    "Interfaces wifi detectadas",      cmd_wifi },
    { "netup",   "Activa una interfaz: netup <if>", cmd_netup },
    { "netdown", "Apaga una interfaz: netdown <if>", cmd_netdown },
    { "dev",     "Interfaces en /sys/class/net",     cmd_dev },
};

void cmd_net_register(void) {
    shell_register(net_commands,
                   sizeof(net_commands) / sizeof(net_commands[0]));
}
