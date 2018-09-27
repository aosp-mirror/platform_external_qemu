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

#ifndef QAPI_EVENTS_BLOCK_H
#define QAPI_EVENTS_BLOCK_H

#include "qapi-events-block-core.h"
#include "qapi/util.h"
#include "qapi-types-block.h"


void qapi_event_send_device_tray_moved(const char *device, const char *id, bool tray_open, Error **errp);

void qapi_event_send_quorum_failure(const char *reference, int64_t sector_num, int64_t sectors_count, Error **errp);

void qapi_event_send_quorum_report_bad(QuorumOpType type, bool has_error, const char *error, const char *node_name, int64_t sector_num, int64_t sectors_count, Error **errp);

#endif /* QAPI_EVENTS_BLOCK_H */
