/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI/QMP events
 *
 * Copyright (c) 2014 Wenchao Xia
 * Copyright (c) 2015-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#ifndef QAPI_EVENTS_NET_H
#define QAPI_EVENTS_NET_H

#include "qapi-events-common.h"
#include "qapi/util.h"
#include "qapi-types-net.h"


void qapi_event_send_nic_rx_filter_changed(bool has_name, const char *name, const char *path, Error **errp);

#endif /* QAPI_EVENTS_NET_H */
