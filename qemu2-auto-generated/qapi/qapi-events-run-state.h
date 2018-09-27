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

#ifndef QAPI_EVENTS_RUN_STATE_H
#define QAPI_EVENTS_RUN_STATE_H

#include "qapi/util.h"
#include "qapi-types-run-state.h"


void qapi_event_send_shutdown(bool guest, Error **errp);

void qapi_event_send_powerdown(Error **errp);

void qapi_event_send_reset(bool guest, Error **errp);

void qapi_event_send_stop(Error **errp);

void qapi_event_send_resume(Error **errp);

void qapi_event_send_suspend(Error **errp);

void qapi_event_send_suspend_disk(Error **errp);

void qapi_event_send_wakeup(Error **errp);

void qapi_event_send_watchdog(WatchdogAction action, Error **errp);

void qapi_event_send_guest_panicked(GuestPanicAction action, bool has_info, GuestPanicInformation *info, Error **errp);

#endif /* QAPI_EVENTS_RUN_STATE_H */
