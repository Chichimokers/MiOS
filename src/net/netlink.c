#include "mios.h"
#include "net/netlink.h"

/*
 * MiOS - transporte netlink para el modulo de red.
 * Solo usa socket/bind/sendto/recvfrom.
 */

static int netlink_socket(void) {
    int fd;
    long br;
    struct sockaddr_nl local;

    fd = mios_socket(MIOS_AF_NETLINK, MIOS_SOCK_RAW, MIOS_NETLINK_ROUTE);
    if (fd < 0) {
        return -mios_error(fd);
    }

    mios_memset(&local, 0, sizeof(local));
    local.nl_family = MIOS_AF_NETLINK;

    br = mios_bind(fd, &local, sizeof(local));
    if (br < 0) {
        mios_close(fd);
        return -mios_error(br);
    }

    return fd;
}

static int netlink_request(int fd, int type, int flags,
                           const void *payload, int payload_len) {
    char msg[256];
    struct nlmsghdr *nlh = (struct nlmsghdr *)msg;
    struct sockaddr_nl dst;
    long r;

    if (NLMSG_LENGTH(payload_len) > (int)sizeof(msg)) {
        return -MIOS_EINVAL;
    }

    mios_memset(msg, 0, sizeof(msg));
    nlh->nlmsg_len = (u32)NLMSG_LENGTH(payload_len);
    nlh->nlmsg_type = (u16)type;
    nlh->nlmsg_flags = (u16)flags;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = 0;

    if (payload != MIOS_NULL && payload_len > 0) {
        mios_memcpy(NLMSG_DATA(nlh), payload, (mios_size_t)payload_len);
    }

    mios_memset(&dst, 0, sizeof(dst));
    dst.nl_family = MIOS_AF_NETLINK;

    r = mios_sendto(fd, msg, (mios_size_t)nlh->nlmsg_len, 0,
                    &dst, (unsigned int)sizeof(dst));
    if (r < 0) {
        return -mios_error(r);
    }
    return 0;
}

typedef void (*nl_handler_t)(struct nlmsghdr *nlh, struct net_info *info);

static int netlink_dump(int type, const void *payload, int payload_len,
                        nl_handler_t handler, struct net_info *info) {
    int fd;
    int err;
    char buf[8192];

    fd = netlink_socket();
    if (fd < 0) {
        return fd;
    }

    err = netlink_request(fd, type, NLM_F_REQUEST | NLM_F_DUMP,
                          payload, payload_len);
    if (err < 0) {
        mios_close(fd);
        return err;
    }

    for (;;) {
        long n = mios_recvfrom(fd, buf, sizeof(buf), 0, MIOS_NULL, MIOS_NULL);
        int rem;
        struct nlmsghdr *nlh;

        if (n < 0) {
            mios_close(fd);
            return -mios_error(n);
        }
        if (n == 0) {
            break;
        }

        rem = (int)n;
        for (nlh = (struct nlmsghdr *)buf; NLMSG_OK(nlh, rem);
             nlh = NLMSG_NEXT(nlh, rem)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                mios_close(fd);
                return 0;
            }
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                struct nlmsgerr *e = (struct nlmsgerr *)NLMSG_DATA(nlh);
                err = (e->error != 0) ? e->error : -MIOS_EIO;
                mios_close(fd);
                return err;
            }
            handler(nlh, info);
        }
    }

    mios_close(fd);
    return 0;
}

static void parse_link(struct nlmsghdr *nlh, struct net_info *info) {
    struct ifinfomsg *ifi;
    struct rtattr *rta;
    struct net_iface *iface;
    int alen;

    if (info->iface_count >= NET_MAX_IFACES) {
        return;
    }

    ifi = (struct ifinfomsg *)NLMSG_DATA(nlh);
    iface = &info->ifaces[info->iface_count];
    mios_memset(iface, 0, sizeof(*iface));
    iface->index = ifi->ifi_index;
    iface->flags = ifi->ifi_flags;

    alen = (int)nlh->nlmsg_len - NLMSG_LENGTH((int)sizeof(struct ifinfomsg));
    rta = (struct rtattr *)((char *)ifi + NLMSG_ALIGN(sizeof(struct ifinfomsg)));

    while (RTA_OK(rta, alen)) {
        int n = (int)rta->rta_len - (int)RTA_ALIGN(sizeof(struct rtattr));

        if (rta->rta_type == IFLA_IFNAME) {
            mios_strncpy(iface->name, (char *)RTA_DATA(rta), NET_NAME_LEN - 1);
            iface->name[NET_NAME_LEN - 1] = '\0';
        } else if (rta->rta_type == IFLA_MTU) {
            if (n >= (int)sizeof(u32)) {
                mios_memcpy(&iface->mtu, RTA_DATA(rta), sizeof(u32));
            }
        } else if (rta->rta_type == IFLA_ADDRESS) {
            if (n > NET_HWADDR_LEN) {
                n = NET_HWADDR_LEN;
            }
            mios_memcpy(iface->hwaddr, RTA_DATA(rta), (mios_size_t)n);
            iface->hwaddr_len = n;
        }
        rta = RTA_NEXT(rta, alen);
    }

    info->iface_count++;
}

static void parse_addr(struct nlmsghdr *nlh, struct net_info *info) {
    struct ifaddrmsg *ifa;
    struct rtattr *rta;
    struct net_iface *iface;
    int alen;
    int have = 0;
    u8 addr[16];

    ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);
    iface = net_iface_by_index(info, (int)ifa->ifa_index);
    if (iface == MIOS_NULL || iface->addr_count >= NET_MAX_ADDRS) {
        return;
    }

    mios_memset(addr, 0, sizeof(addr));
    alen = (int)nlh->nlmsg_len - NLMSG_LENGTH((int)sizeof(struct ifaddrmsg));
    rta = (struct rtattr *)((char *)ifa + NLMSG_ALIGN(sizeof(struct ifaddrmsg)));

    while (RTA_OK(rta, alen)) {
        if (rta->rta_type == IFA_LOCAL || rta->rta_type == IFA_ADDRESS) {
            int n = (int)rta->rta_len - (int)RTA_ALIGN(sizeof(struct rtattr));
            if (n > (int)sizeof(addr)) {
                n = (int)sizeof(addr);
            }
            mios_memset(addr, 0, sizeof(addr));
            mios_memcpy(addr, RTA_DATA(rta), (mios_size_t)n);
            have = 1;
            if (rta->rta_type == IFA_LOCAL) {
                break;
            }
        }
        rta = RTA_NEXT(rta, alen);
    }

    if (have) {
        struct net_addr *na = &iface->addrs[iface->addr_count];
        na->family = ifa->ifa_family;
        na->prefixlen = ifa->ifa_prefixlen;
        net_format_addr(ifa->ifa_family, addr, na->text, sizeof(na->text));
        iface->addr_count++;
    }
}

static void parse_route(struct nlmsghdr *nlh, struct net_info *info) {
    struct rtmsg *rtm;
    struct rtattr *rta;
    struct net_route *route;
    int alen;
    int have_dst = 0;
    int have_gw = 0;
    int oif = 0;
    int metric = 0;
    u8 dst[16];
    u8 gw[16];

    if (info->route_count >= NET_MAX_ROUTES) {
        return;
    }

    rtm = (struct rtmsg *)NLMSG_DATA(nlh);
    if (rtm->rtm_table != RT_TABLE_MAIN) {
        return;
    }
    mios_memset(dst, 0, sizeof(dst));
    mios_memset(gw, 0, sizeof(gw));

    alen = (int)nlh->nlmsg_len - NLMSG_LENGTH((int)sizeof(struct rtmsg));
    rta = (struct rtattr *)((char *)rtm + NLMSG_ALIGN(sizeof(struct rtmsg)));

    while (RTA_OK(rta, alen)) {
        int n = (int)rta->rta_len - (int)RTA_ALIGN(sizeof(struct rtattr));

        if (rta->rta_type == RTA_DST) {
            if (n > (int)sizeof(dst)) {
                n = (int)sizeof(dst);
            }
            mios_memcpy(dst, RTA_DATA(rta), (mios_size_t)n);
            have_dst = 1;
        } else if (rta->rta_type == RTA_GATEWAY) {
            if (n > (int)sizeof(gw)) {
                n = (int)sizeof(gw);
            }
            mios_memcpy(gw, RTA_DATA(rta), (mios_size_t)n);
            have_gw = 1;
        } else if (rta->rta_type == RTA_OIF) {
            if (n >= (int)sizeof(int)) {
                mios_memcpy(&oif, RTA_DATA(rta), sizeof(int));
            }
        } else if (rta->rta_type == RTA_PRIORITY) {
            if (n >= (int)sizeof(int)) {
                mios_memcpy(&metric, RTA_DATA(rta), sizeof(int));
            }
        }
        rta = RTA_NEXT(rta, alen);
    }

    route = &info->routes[info->route_count];
    route->family = rtm->rtm_family;
    route->ifindex = oif;
    route->metric = metric;

    if (rtm->rtm_dst_len == 0) {
        mios_strcpy(route->dst, "default");
    } else if (have_dst) {
        net_format_addr(rtm->rtm_family, dst, route->dst, sizeof(route->dst));
    } else {
        route->dst[0] = '\0';
    }

    if (have_gw) {
        net_format_addr(rtm->rtm_family, gw, route->gateway,
                        sizeof(route->gateway));
    } else {
        route->gateway[0] = '\0';
    }

    info->route_count++;
}

int netlink_dump_links(struct net_info *info) {
    struct ifinfomsg req;
    mios_memset(&req, 0, sizeof(req));
    req.ifi_family = MIOS_AF_UNSPEC;
    return netlink_dump(RTM_GETLINK, &req, (int)sizeof(req), parse_link, info);
}

int netlink_dump_addrs(struct net_info *info) {
    struct ifaddrmsg req;
    mios_memset(&req, 0, sizeof(req));
    req.ifa_family = MIOS_AF_UNSPEC;
    return netlink_dump(RTM_GETADDR, &req, (int)sizeof(req), parse_addr, info);
}

int netlink_dump_routes(struct net_info *info) {
    struct rtmsg req;
    mios_memset(&req, 0, sizeof(req));
    req.rtm_family = MIOS_AF_UNSPEC;
    return netlink_dump(RTM_GETROUTE, &req, (int)sizeof(req), parse_route, info);
}

int netlink_set_link_up(int ifindex, int up) {
    int fd;
    int err = 0;
    char msg[256];
    struct nlmsghdr *nlh = (struct nlmsghdr *)msg;
    struct ifinfomsg *ifi;
    struct sockaddr_nl dst;
    long n;

    fd = netlink_socket();
    if (fd < 0) {
        return fd;
    }

    mios_memset(msg, 0, sizeof(msg));
    nlh->nlmsg_len = (u32)NLMSG_LENGTH((int)sizeof(struct ifinfomsg));
    nlh->nlmsg_type = RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = 1;
    nlh->nlmsg_pid = 0;

    ifi = (struct ifinfomsg *)NLMSG_DATA(nlh);
    ifi->ifi_family = MIOS_AF_UNSPEC;
    ifi->ifi_index = ifindex;
    ifi->ifi_flags = up ? IFF_UP : 0;
    ifi->ifi_change = IFF_UP;

    mios_memset(&dst, 0, sizeof(dst));
    dst.nl_family = MIOS_AF_NETLINK;

    n = mios_sendto(fd, msg, nlh->nlmsg_len, 0, &dst,
                    (unsigned int)sizeof(dst));
    if (n < 0) {
        mios_close(fd);
        return -mios_error(n);
    }

    n = mios_recvfrom(fd, msg, sizeof(msg), 0, MIOS_NULL, MIOS_NULL);
    if (n < 0) {
        mios_close(fd);
        return -mios_error(n);
    }

    {
        int rem = (int)n;
        struct nlmsghdr *rh = (struct nlmsghdr *)msg;
        for (; NLMSG_OK(rh, rem); rh = NLMSG_NEXT(rh, rem)) {
            if (rh->nlmsg_type == NLMSG_ERROR) {
                struct nlmsgerr *e = (struct nlmsgerr *)NLMSG_DATA(rh);
                err = e->error;
                break;
            }
        }
    }

    mios_close(fd);
    return err;
}
