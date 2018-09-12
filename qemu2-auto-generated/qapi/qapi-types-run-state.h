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

#ifndef QAPI_TYPES_RUN_STATE_H
#define QAPI_TYPES_RUN_STATE_H

#include "qapi/qapi-builtin-types.h"

typedef enum RunState {
    RUN_STATE_DEBUG = 0,
    RUN_STATE_INMIGRATE = 1,
    RUN_STATE_INTERNAL_ERROR = 2,
    RUN_STATE_IO_ERROR = 3,
    RUN_STATE_PAUSED = 4,
    RUN_STATE_POSTMIGRATE = 5,
    RUN_STATE_PRELAUNCH = 6,
    RUN_STATE_FINISH_MIGRATE = 7,
    RUN_STATE_RESTORE_VM = 8,
    RUN_STATE_RUNNING = 9,
    RUN_STATE_SAVE_VM = 10,
    RUN_STATE_SHUTDOWN = 11,
    RUN_STATE_SUSPENDED = 12,
    RUN_STATE_WATCHDOG = 13,
    RUN_STATE_GUEST_PANICKED = 14,
    RUN_STATE_COLO = 15,
    RUN_STATE__MAX = 16,
} RunState;

#define RunState_str(val) \
    qapi_enum_lookup(&RunState_lookup, (val))

extern const QEnumLookup RunState_lookup;

typedef struct StatusInfo StatusInfo;

typedef struct q_obj_SHUTDOWN_arg q_obj_SHUTDOWN_arg;

typedef struct q_obj_RESET_arg q_obj_RESET_arg;

typedef struct q_obj_WATCHDOG_arg q_obj_WATCHDOG_arg;

typedef enum WatchdogAction {
    WATCHDOG_ACTION_RESET = 0,
    WATCHDOG_ACTION_SHUTDOWN = 1,
    WATCHDOG_ACTION_POWEROFF = 2,
    WATCHDOG_ACTION_PAUSE = 3,
    WATCHDOG_ACTION_DEBUG = 4,
    WATCHDOG_ACTION_NONE = 5,
    WATCHDOG_ACTION_INJECT_NMI = 6,
    WATCHDOG_ACTION__MAX = 7,
} WatchdogAction;

#define WatchdogAction_str(val) \
    qapi_enum_lookup(&WatchdogAction_lookup, (val))

extern const QEnumLookup WatchdogAction_lookup;

typedef struct q_obj_watchdog_set_action_arg q_obj_watchdog_set_action_arg;

typedef struct q_obj_GUEST_PANICKED_arg q_obj_GUEST_PANICKED_arg;

typedef enum GuestPanicAction {
    GUEST_PANIC_ACTION_PAUSE = 0,
    GUEST_PANIC_ACTION_POWEROFF = 1,
    GUEST_PANIC_ACTION__MAX = 2,
} GuestPanicAction;

#define GuestPanicAction_str(val) \
    qapi_enum_lookup(&GuestPanicAction_lookup, (val))

extern const QEnumLookup GuestPanicAction_lookup;

typedef enum GuestPanicInformationType {
    GUEST_PANIC_INFORMATION_TYPE_HYPER_V = 0,
    GUEST_PANIC_INFORMATION_TYPE_S390 = 1,
    GUEST_PANIC_INFORMATION_TYPE__MAX = 2,
} GuestPanicInformationType;

#define GuestPanicInformationType_str(val) \
    qapi_enum_lookup(&GuestPanicInformationType_lookup, (val))

extern const QEnumLookup GuestPanicInformationType_lookup;

typedef struct q_obj_GuestPanicInformation_base q_obj_GuestPanicInformation_base;

typedef struct GuestPanicInformation GuestPanicInformation;

typedef struct GuestPanicInformationHyperV GuestPanicInformationHyperV;

typedef enum S390CrashReason {
    S390_CRASH_REASON_UNKNOWN = 0,
    S390_CRASH_REASON_DISABLED_WAIT = 1,
    S390_CRASH_REASON_EXTINT_LOOP = 2,
    S390_CRASH_REASON_PGMINT_LOOP = 3,
    S390_CRASH_REASON_OPINT_LOOP = 4,
    S390_CRASH_REASON__MAX = 5,
} S390CrashReason;

#define S390CrashReason_str(val) \
    qapi_enum_lookup(&S390CrashReason_lookup, (val))

extern const QEnumLookup S390CrashReason_lookup;

typedef struct GuestPanicInformationS390 GuestPanicInformationS390;

struct StatusInfo {
    bool running;
    bool singlestep;
    RunState status;
};

void qapi_free_StatusInfo(StatusInfo *obj);

struct q_obj_SHUTDOWN_arg {
    bool guest;
};

struct q_obj_RESET_arg {
    bool guest;
};

struct q_obj_WATCHDOG_arg {
    WatchdogAction action;
};

struct q_obj_watchdog_set_action_arg {
    WatchdogAction action;
};

struct q_obj_GUEST_PANICKED_arg {
    GuestPanicAction action;
    bool has_info;
    GuestPanicInformation *info;
};

struct q_obj_GuestPanicInformation_base {
    GuestPanicInformationType type;
};

struct GuestPanicInformationHyperV {
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
};

struct GuestPanicInformationS390 {
    uint32_t core;
    uint64_t psw_mask;
    uint64_t psw_addr;
    S390CrashReason reason;
};

struct GuestPanicInformation {
    GuestPanicInformationType type;
    union { /* union tag is @type */
        GuestPanicInformationHyperV hyper_v;
        GuestPanicInformationS390 s390;
    } u;
};

void qapi_free_GuestPanicInformation(GuestPanicInformation *obj);

void qapi_free_GuestPanicInformationHyperV(GuestPanicInformationHyperV *obj);

void qapi_free_GuestPanicInformationS390(GuestPanicInformationS390 *obj);

#endif /* QAPI_TYPES_RUN_STATE_H */
