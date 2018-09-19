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

#ifndef QAPI_EVENTS_MISC_H
#define QAPI_EVENTS_MISC_H

#include "qapi/util.h"
#include "qapi-types-misc.h"


void qapi_event_send_balloon_change(int64_t actual, Error **errp);

void qapi_event_send_device_deleted(bool has_device, const char *device, const char *path, Error **errp);

void qapi_event_send_dump_completed(DumpQueryResult *result, bool has_error, const char *error, Error **errp);

void qapi_event_send_mem_unplug_error(const char *device, const char *msg, Error **errp);

void qapi_event_send_acpi_device_ost(ACPIOSTInfo *info, Error **errp);

void qapi_event_send_rtc_change(int64_t offset, Error **errp);

void qapi_event_send_command_dropped(QObject *id, CommandDropReason reason, Error **errp);

typedef enum QAPIEvent {
    QAPI_EVENT_SHUTDOWN = 0,
    QAPI_EVENT_POWERDOWN = 1,
    QAPI_EVENT_RESET = 2,
    QAPI_EVENT_STOP = 3,
    QAPI_EVENT_RESUME = 4,
    QAPI_EVENT_SUSPEND = 5,
    QAPI_EVENT_SUSPEND_DISK = 6,
    QAPI_EVENT_WAKEUP = 7,
    QAPI_EVENT_WATCHDOG = 8,
    QAPI_EVENT_GUEST_PANICKED = 9,
    QAPI_EVENT_BLOCK_IMAGE_CORRUPTED = 10,
    QAPI_EVENT_BLOCK_IO_ERROR = 11,
    QAPI_EVENT_BLOCK_JOB_COMPLETED = 12,
    QAPI_EVENT_BLOCK_JOB_CANCELLED = 13,
    QAPI_EVENT_BLOCK_JOB_ERROR = 14,
    QAPI_EVENT_BLOCK_JOB_READY = 15,
    QAPI_EVENT_BLOCK_JOB_PENDING = 16,
    QAPI_EVENT_BLOCK_WRITE_THRESHOLD = 17,
    QAPI_EVENT_DEVICE_TRAY_MOVED = 18,
    QAPI_EVENT_QUORUM_FAILURE = 19,
    QAPI_EVENT_QUORUM_REPORT_BAD = 20,
    QAPI_EVENT_VSERPORT_CHANGE = 21,
    QAPI_EVENT_NIC_RX_FILTER_CHANGED = 22,
    QAPI_EVENT_SPICE_CONNECTED = 23,
    QAPI_EVENT_SPICE_INITIALIZED = 24,
    QAPI_EVENT_SPICE_DISCONNECTED = 25,
    QAPI_EVENT_SPICE_MIGRATE_COMPLETED = 26,
    QAPI_EVENT_VNC_CONNECTED = 27,
    QAPI_EVENT_VNC_INITIALIZED = 28,
    QAPI_EVENT_VNC_DISCONNECTED = 29,
    QAPI_EVENT_MIGRATION = 30,
    QAPI_EVENT_MIGRATION_PASS = 31,
    QAPI_EVENT_BALLOON_CHANGE = 32,
    QAPI_EVENT_DEVICE_DELETED = 33,
    QAPI_EVENT_DUMP_COMPLETED = 34,
    QAPI_EVENT_MEM_UNPLUG_ERROR = 35,
    QAPI_EVENT_ACPI_DEVICE_OST = 36,
    QAPI_EVENT_RTC_CHANGE = 37,
    QAPI_EVENT_COMMAND_DROPPED = 38,
    QAPI_EVENT__MAX = 39,
} QAPIEvent;

#define QAPIEvent_str(val) \
    qapi_enum_lookup(&QAPIEvent_lookup, (val))

extern const QEnumLookup QAPIEvent_lookup;

#endif /* QAPI_EVENTS_MISC_H */
