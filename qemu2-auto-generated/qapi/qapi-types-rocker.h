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

#ifndef QAPI_TYPES_ROCKER_H
#define QAPI_TYPES_ROCKER_H

#include "qapi/qapi-builtin-types.h"

typedef struct RockerSwitch RockerSwitch;

typedef struct q_obj_query_rocker_arg q_obj_query_rocker_arg;

typedef enum RockerPortDuplex {
    ROCKER_PORT_DUPLEX_HALF = 0,
    ROCKER_PORT_DUPLEX_FULL = 1,
    ROCKER_PORT_DUPLEX__MAX = 2,
} RockerPortDuplex;

#define RockerPortDuplex_str(val) \
    qapi_enum_lookup(&RockerPortDuplex_lookup, (val))

extern const QEnumLookup RockerPortDuplex_lookup;

typedef enum RockerPortAutoneg {
    ROCKER_PORT_AUTONEG_OFF = 0,
    ROCKER_PORT_AUTONEG_ON = 1,
    ROCKER_PORT_AUTONEG__MAX = 2,
} RockerPortAutoneg;

#define RockerPortAutoneg_str(val) \
    qapi_enum_lookup(&RockerPortAutoneg_lookup, (val))

extern const QEnumLookup RockerPortAutoneg_lookup;

typedef struct RockerPort RockerPort;

typedef struct q_obj_query_rocker_ports_arg q_obj_query_rocker_ports_arg;

typedef struct RockerPortList RockerPortList;

typedef struct RockerOfDpaFlowKey RockerOfDpaFlowKey;

typedef struct RockerOfDpaFlowMask RockerOfDpaFlowMask;

typedef struct RockerOfDpaFlowAction RockerOfDpaFlowAction;

typedef struct RockerOfDpaFlow RockerOfDpaFlow;

typedef struct q_obj_query_rocker_of_dpa_flows_arg q_obj_query_rocker_of_dpa_flows_arg;

typedef struct RockerOfDpaFlowList RockerOfDpaFlowList;

typedef struct RockerOfDpaGroup RockerOfDpaGroup;

typedef struct q_obj_query_rocker_of_dpa_groups_arg q_obj_query_rocker_of_dpa_groups_arg;

typedef struct RockerOfDpaGroupList RockerOfDpaGroupList;

struct RockerSwitch {
    char *name;
    uint64_t id;
    uint32_t ports;
};

void qapi_free_RockerSwitch(RockerSwitch *obj);

struct q_obj_query_rocker_arg {
    char *name;
};

struct RockerPort {
    char *name;
    bool enabled;
    bool link_up;
    uint32_t speed;
    RockerPortDuplex duplex;
    RockerPortAutoneg autoneg;
};

void qapi_free_RockerPort(RockerPort *obj);

struct q_obj_query_rocker_ports_arg {
    char *name;
};

struct RockerPortList {
    RockerPortList *next;
    RockerPort *value;
};

void qapi_free_RockerPortList(RockerPortList *obj);

struct RockerOfDpaFlowKey {
    uint32_t priority;
    uint32_t tbl_id;
    bool has_in_pport;
    uint32_t in_pport;
    bool has_tunnel_id;
    uint32_t tunnel_id;
    bool has_vlan_id;
    uint16_t vlan_id;
    bool has_eth_type;
    uint16_t eth_type;
    bool has_eth_src;
    char *eth_src;
    bool has_eth_dst;
    char *eth_dst;
    bool has_ip_proto;
    uint8_t ip_proto;
    bool has_ip_tos;
    uint8_t ip_tos;
    bool has_ip_dst;
    char *ip_dst;
};

void qapi_free_RockerOfDpaFlowKey(RockerOfDpaFlowKey *obj);

struct RockerOfDpaFlowMask {
    bool has_in_pport;
    uint32_t in_pport;
    bool has_tunnel_id;
    uint32_t tunnel_id;
    bool has_vlan_id;
    uint16_t vlan_id;
    bool has_eth_src;
    char *eth_src;
    bool has_eth_dst;
    char *eth_dst;
    bool has_ip_proto;
    uint8_t ip_proto;
    bool has_ip_tos;
    uint8_t ip_tos;
};

void qapi_free_RockerOfDpaFlowMask(RockerOfDpaFlowMask *obj);

struct RockerOfDpaFlowAction {
    bool has_goto_tbl;
    uint32_t goto_tbl;
    bool has_group_id;
    uint32_t group_id;
    bool has_tunnel_lport;
    uint32_t tunnel_lport;
    bool has_vlan_id;
    uint16_t vlan_id;
    bool has_new_vlan_id;
    uint16_t new_vlan_id;
    bool has_out_pport;
    uint32_t out_pport;
};

void qapi_free_RockerOfDpaFlowAction(RockerOfDpaFlowAction *obj);

struct RockerOfDpaFlow {
    uint64_t cookie;
    uint64_t hits;
    RockerOfDpaFlowKey *key;
    RockerOfDpaFlowMask *mask;
    RockerOfDpaFlowAction *action;
};

void qapi_free_RockerOfDpaFlow(RockerOfDpaFlow *obj);

struct q_obj_query_rocker_of_dpa_flows_arg {
    char *name;
    bool has_tbl_id;
    uint32_t tbl_id;
};

struct RockerOfDpaFlowList {
    RockerOfDpaFlowList *next;
    RockerOfDpaFlow *value;
};

void qapi_free_RockerOfDpaFlowList(RockerOfDpaFlowList *obj);

struct RockerOfDpaGroup {
    uint32_t id;
    uint8_t type;
    bool has_vlan_id;
    uint16_t vlan_id;
    bool has_pport;
    uint32_t pport;
    bool has_index;
    uint32_t index;
    bool has_out_pport;
    uint32_t out_pport;
    bool has_group_id;
    uint32_t group_id;
    bool has_set_vlan_id;
    uint16_t set_vlan_id;
    bool has_pop_vlan;
    uint8_t pop_vlan;
    bool has_group_ids;
    uint32List *group_ids;
    bool has_set_eth_src;
    char *set_eth_src;
    bool has_set_eth_dst;
    char *set_eth_dst;
    bool has_ttl_check;
    uint8_t ttl_check;
};

void qapi_free_RockerOfDpaGroup(RockerOfDpaGroup *obj);

struct q_obj_query_rocker_of_dpa_groups_arg {
    char *name;
    bool has_type;
    uint8_t type;
};

struct RockerOfDpaGroupList {
    RockerOfDpaGroupList *next;
    RockerOfDpaGroup *value;
};

void qapi_free_RockerOfDpaGroupList(RockerOfDpaGroupList *obj);

#endif /* QAPI_TYPES_ROCKER_H */
