/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 * Copyright (c) 2013-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef QAPI_TYPES_NET_H
#define QAPI_TYPES_NET_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-common.h"

typedef struct q_obj_set_link_arg q_obj_set_link_arg;

typedef struct q_obj_netdev_add_arg q_obj_netdev_add_arg;

typedef struct q_obj_netdev_del_arg q_obj_netdev_del_arg;

typedef struct NetdevNoneOptions NetdevNoneOptions;

typedef struct NetLegacyNicOptions NetLegacyNicOptions;

typedef struct StringList StringList;

typedef struct NetdevUserOptions NetdevUserOptions;

typedef struct NetdevTapOptions NetdevTapOptions;

typedef struct NetdevSocketOptions NetdevSocketOptions;

typedef struct NetdevL2TPv3Options NetdevL2TPv3Options;

typedef struct NetdevVdeOptions NetdevVdeOptions;

typedef struct NetdevBridgeOptions NetdevBridgeOptions;

typedef struct NetdevHubPortOptions NetdevHubPortOptions;

typedef struct NetdevNetmapOptions NetdevNetmapOptions;

typedef struct NetdevVhostUserOptions NetdevVhostUserOptions;

typedef enum NetClientDriver {
    NET_CLIENT_DRIVER_NONE = 0,
    NET_CLIENT_DRIVER_NIC = 1,
    NET_CLIENT_DRIVER_USER = 2,
    NET_CLIENT_DRIVER_TAP = 3,
    NET_CLIENT_DRIVER_L2TPV3 = 4,
    NET_CLIENT_DRIVER_SOCKET = 5,
    NET_CLIENT_DRIVER_VDE = 6,
    NET_CLIENT_DRIVER_BRIDGE = 7,
    NET_CLIENT_DRIVER_HUBPORT = 8,
    NET_CLIENT_DRIVER_NETMAP = 9,
    NET_CLIENT_DRIVER_VHOST_USER = 10,
    NET_CLIENT_DRIVER__MAX = 11,
} NetClientDriver;

#define NetClientDriver_str(val) \
    qapi_enum_lookup(&NetClientDriver_lookup, (val))

extern const QEnumLookup NetClientDriver_lookup;

typedef struct q_obj_Netdev_base q_obj_Netdev_base;

typedef struct Netdev Netdev;

typedef struct NetLegacy NetLegacy;

typedef enum NetLegacyOptionsType {
    NET_LEGACY_OPTIONS_TYPE_NONE = 0,
    NET_LEGACY_OPTIONS_TYPE_NIC = 1,
    NET_LEGACY_OPTIONS_TYPE_USER = 2,
    NET_LEGACY_OPTIONS_TYPE_TAP = 3,
    NET_LEGACY_OPTIONS_TYPE_L2TPV3 = 4,
    NET_LEGACY_OPTIONS_TYPE_SOCKET = 5,
    NET_LEGACY_OPTIONS_TYPE_VDE = 6,
    NET_LEGACY_OPTIONS_TYPE_BRIDGE = 7,
    NET_LEGACY_OPTIONS_TYPE_NETMAP = 8,
    NET_LEGACY_OPTIONS_TYPE_VHOST_USER = 9,
    NET_LEGACY_OPTIONS_TYPE__MAX = 10,
} NetLegacyOptionsType;

#define NetLegacyOptionsType_str(val) \
    qapi_enum_lookup(&NetLegacyOptionsType_lookup, (val))

extern const QEnumLookup NetLegacyOptionsType_lookup;

typedef struct q_obj_NetLegacyOptions_base q_obj_NetLegacyOptions_base;

typedef struct NetLegacyOptions NetLegacyOptions;

typedef enum NetFilterDirection {
    NET_FILTER_DIRECTION_ALL = 0,
    NET_FILTER_DIRECTION_RX = 1,
    NET_FILTER_DIRECTION_TX = 2,
    NET_FILTER_DIRECTION__MAX = 3,
} NetFilterDirection;

#define NetFilterDirection_str(val) \
    qapi_enum_lookup(&NetFilterDirection_lookup, (val))

extern const QEnumLookup NetFilterDirection_lookup;

typedef enum RxState {
    RX_STATE_NORMAL = 0,
    RX_STATE_NONE = 1,
    RX_STATE_ALL = 2,
    RX_STATE__MAX = 3,
} RxState;

#define RxState_str(val) \
    qapi_enum_lookup(&RxState_lookup, (val))

extern const QEnumLookup RxState_lookup;

typedef struct RxFilterInfo RxFilterInfo;

typedef struct q_obj_query_rx_filter_arg q_obj_query_rx_filter_arg;

typedef struct RxFilterInfoList RxFilterInfoList;

typedef struct q_obj_NIC_RX_FILTER_CHANGED_arg q_obj_NIC_RX_FILTER_CHANGED_arg;

struct q_obj_set_link_arg {
    char *name;
    bool up;
};

struct q_obj_netdev_add_arg {
    char *type;
    char *id;
};

struct q_obj_netdev_del_arg {
    char *id;
};

struct NetdevNoneOptions {
    char qapi_dummy_for_empty_struct;
};

void qapi_free_NetdevNoneOptions(NetdevNoneOptions *obj);

struct NetLegacyNicOptions {
    bool has_netdev;
    char *netdev;
    bool has_macaddr;
    char *macaddr;
    bool has_model;
    char *model;
    bool has_addr;
    char *addr;
    bool has_vectors;
    uint32_t vectors;
};

void qapi_free_NetLegacyNicOptions(NetLegacyNicOptions *obj);

struct StringList {
    StringList *next;
    String *value;
};

void qapi_free_StringList(StringList *obj);

struct NetdevUserOptions {
    bool has_hostname;
    char *hostname;
    bool has_q_restrict;
    bool q_restrict;
    bool has_ipv4;
    bool ipv4;
    bool has_ipv6;
    bool ipv6;
    bool has_ip;
    char *ip;
    bool has_net;
    char *net;
    bool has_host;
    char *host;
    bool has_tftp;
    char *tftp;
    bool has_bootfile;
    char *bootfile;
    bool has_dhcpstart;
    char *dhcpstart;
    bool has_dns;
    char *dns;
    bool has_dnssearch;
    StringList *dnssearch;
    bool has_ipv6_prefix;
    char *ipv6_prefix;
    bool has_ipv6_prefixlen;
    int64_t ipv6_prefixlen;
    bool has_ipv6_host;
    char *ipv6_host;
    bool has_ipv6_dns;
    char *ipv6_dns;
    bool has_smb;
    char *smb;
    bool has_smbserver;
    char *smbserver;
    bool has_hostfwd;
    StringList *hostfwd;
    bool has_guestfwd;
    StringList *guestfwd;
};

void qapi_free_NetdevUserOptions(NetdevUserOptions *obj);

struct NetdevTapOptions {
    bool has_ifname;
    char *ifname;
    bool has_fd;
    char *fd;
    bool has_fds;
    char *fds;
    bool has_script;
    char *script;
    bool has_downscript;
    char *downscript;
    bool has_br;
    char *br;
    bool has_helper;
    char *helper;
    bool has_sndbuf;
    uint64_t sndbuf;
    bool has_vnet_hdr;
    bool vnet_hdr;
    bool has_vhost;
    bool vhost;
    bool has_vhostfd;
    char *vhostfd;
    bool has_vhostfds;
    char *vhostfds;
    bool has_vhostforce;
    bool vhostforce;
    bool has_queues;
    uint32_t queues;
    bool has_poll_us;
    uint32_t poll_us;
};

void qapi_free_NetdevTapOptions(NetdevTapOptions *obj);

struct NetdevSocketOptions {
    bool has_fd;
    char *fd;
    bool has_listen;
    char *listen;
    bool has_connect;
    char *connect;
    bool has_mcast;
    char *mcast;
    bool has_localaddr;
    char *localaddr;
    bool has_udp;
    char *udp;
};

void qapi_free_NetdevSocketOptions(NetdevSocketOptions *obj);

struct NetdevL2TPv3Options {
    char *src;
    char *dst;
    bool has_srcport;
    char *srcport;
    bool has_dstport;
    char *dstport;
    bool has_ipv6;
    bool ipv6;
    bool has_udp;
    bool udp;
    bool has_cookie64;
    bool cookie64;
    bool has_counter;
    bool counter;
    bool has_pincounter;
    bool pincounter;
    bool has_txcookie;
    uint64_t txcookie;
    bool has_rxcookie;
    uint64_t rxcookie;
    uint32_t txsession;
    bool has_rxsession;
    uint32_t rxsession;
    bool has_offset;
    uint32_t offset;
};

void qapi_free_NetdevL2TPv3Options(NetdevL2TPv3Options *obj);

struct NetdevVdeOptions {
    bool has_sock;
    char *sock;
    bool has_port;
    uint16_t port;
    bool has_group;
    char *group;
    bool has_mode;
    uint16_t mode;
};

void qapi_free_NetdevVdeOptions(NetdevVdeOptions *obj);

struct NetdevBridgeOptions {
    bool has_br;
    char *br;
    bool has_helper;
    char *helper;
};

void qapi_free_NetdevBridgeOptions(NetdevBridgeOptions *obj);

struct NetdevHubPortOptions {
    int32_t hubid;
    bool has_netdev;
    char *netdev;
};

void qapi_free_NetdevHubPortOptions(NetdevHubPortOptions *obj);

struct NetdevNetmapOptions {
    char *ifname;
    bool has_devname;
    char *devname;
};

void qapi_free_NetdevNetmapOptions(NetdevNetmapOptions *obj);

struct NetdevVhostUserOptions {
    char *chardev;
    bool has_vhostforce;
    bool vhostforce;
    bool has_queues;
    int64_t queues;
};

void qapi_free_NetdevVhostUserOptions(NetdevVhostUserOptions *obj);

struct q_obj_Netdev_base {
    char *id;
    NetClientDriver type;
};

struct Netdev {
    char *id;
    NetClientDriver type;
    union { /* union tag is @type */
        NetdevNoneOptions none;
        NetLegacyNicOptions nic;
        NetdevUserOptions user;
        NetdevTapOptions tap;
        NetdevL2TPv3Options l2tpv3;
        NetdevSocketOptions socket;
        NetdevVdeOptions vde;
        NetdevBridgeOptions bridge;
        NetdevHubPortOptions hubport;
        NetdevNetmapOptions netmap;
        NetdevVhostUserOptions vhost_user;
    } u;
};

void qapi_free_Netdev(Netdev *obj);

struct NetLegacy {
    bool has_vlan;
    int32_t vlan;
    bool has_id;
    char *id;
    bool has_name;
    char *name;
    NetLegacyOptions *opts;
};

void qapi_free_NetLegacy(NetLegacy *obj);

struct q_obj_NetLegacyOptions_base {
    NetLegacyOptionsType type;
};

struct NetLegacyOptions {
    NetLegacyOptionsType type;
    union { /* union tag is @type */
        NetdevNoneOptions none;
        NetLegacyNicOptions nic;
        NetdevUserOptions user;
        NetdevTapOptions tap;
        NetdevL2TPv3Options l2tpv3;
        NetdevSocketOptions socket;
        NetdevVdeOptions vde;
        NetdevBridgeOptions bridge;
        NetdevNetmapOptions netmap;
        NetdevVhostUserOptions vhost_user;
    } u;
};

void qapi_free_NetLegacyOptions(NetLegacyOptions *obj);

struct RxFilterInfo {
    char *name;
    bool promiscuous;
    RxState multicast;
    RxState unicast;
    RxState vlan;
    bool broadcast_allowed;
    bool multicast_overflow;
    bool unicast_overflow;
    char *main_mac;
    intList *vlan_table;
    strList *unicast_table;
    strList *multicast_table;
};

void qapi_free_RxFilterInfo(RxFilterInfo *obj);

struct q_obj_query_rx_filter_arg {
    bool has_name;
    char *name;
};

struct RxFilterInfoList {
    RxFilterInfoList *next;
    RxFilterInfo *value;
};

void qapi_free_RxFilterInfoList(RxFilterInfoList *obj);

struct q_obj_NIC_RX_FILTER_CHANGED_arg {
    bool has_name;
    char *name;
    char *path;
};

#endif /* QAPI_TYPES_NET_H */
