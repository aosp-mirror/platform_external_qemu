/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * QAPI/QMP schema introspection
 *
 * Copyright (C) 2015-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi-introspect.h"

const QLitObject qmp_schema_qlit = QLIT_QLIST(((QLitObject[]) {
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-status") },
        { "ret-type", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("2") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("SHUTDOWN") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("POWERDOWN") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("3") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("RESET") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("STOP") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("RESUME") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("SUSPEND") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("SUSPEND_DISK") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("WAKEUP") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("4") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("WATCHDOG") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("5") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("watchdog-set-action") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("6") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("GUEST_PANICKED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("7") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("x-block-latency-histogram-set") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-block") },
        { "ret-type", QLIT_QSTR("[8]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("9") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-blockstats") },
        { "ret-type", QLIT_QSTR("[10]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-block-jobs") },
        { "ret-type", QLIT_QSTR("[11]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("12") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block_passwd") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("13") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block_resize") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("14") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-snapshot-sync") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("15") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-snapshot") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("16") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("change-backing-file") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("17") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-commit") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("18") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("drive-backup") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("19") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-backup") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-named-block-nodes") },
        { "ret-type", QLIT_QSTR("[20]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("21") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("drive-mirror") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("22") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-dirty-bitmap-add") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("23") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-dirty-bitmap-remove") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("23") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-dirty-bitmap-clear") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("23") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("x-debug-block-dirty-bitmap-sha256") },
        { "ret-type", QLIT_QSTR("24") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("25") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-mirror") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("26") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block_set_io_throttle") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("27") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-stream") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("28") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-job-set-speed") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("29") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-job-cancel") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("30") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-job-pause") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("31") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-job-resume") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("32") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-job-complete") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("33") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-job-dismiss") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("34") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-job-finalize") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("35") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-add") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("36") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-del") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("37") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("x-blockdev-create") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("38") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-open-tray") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("39") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-close-tray") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("40") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-remove-medium") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("41") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-insert-medium") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("42") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-change-medium") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("43") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_IMAGE_CORRUPTED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("44") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_IO_ERROR") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("45") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_JOB_COMPLETED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("46") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_JOB_CANCELLED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("47") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_JOB_ERROR") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("48") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_JOB_READY") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("49") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_JOB_PENDING") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("50") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BLOCK_WRITE_THRESHOLD") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("51") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("block-set-write-threshold") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("52") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("x-blockdev-change") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("53") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("x-blockdev-set-iothread") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("54") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-snapshot-internal-sync") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("55") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("blockdev-snapshot-delete-internal-sync") },
        { "ret-type", QLIT_QSTR("56") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("57") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("eject") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("58") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("nbd-server-start") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("59") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("nbd-server-add") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("60") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("nbd-server-remove") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("nbd-server-stop") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("61") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("DEVICE_TRAY_MOVED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("62") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("QUORUM_FAILURE") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("63") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("QUORUM_REPORT_BAD") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-chardev") },
        { "ret-type", QLIT_QSTR("[64]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-chardev-backends") },
        { "ret-type", QLIT_QSTR("[65]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("66") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("ringbuf-write") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("67") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("ringbuf-read") },
        { "ret-type", QLIT_QSTR("str") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("68") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("chardev-add") },
        { "ret-type", QLIT_QSTR("69") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("70") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("chardev-change") },
        { "ret-type", QLIT_QSTR("69") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("71") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("chardev-remove") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("72") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("chardev-send-break") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("73") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("VSERPORT_CHANGE") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("74") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("set_link") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("75") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("netdev_add") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("76") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("netdev_del") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("77") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-rx-filter") },
        { "ret-type", QLIT_QSTR("[78]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("79") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("NIC_RX_FILTER_CHANGED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("80") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-rocker") },
        { "ret-type", QLIT_QSTR("81") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("82") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-rocker-ports") },
        { "ret-type", QLIT_QSTR("[83]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("84") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-rocker-of-dpa-flows") },
        { "ret-type", QLIT_QSTR("[85]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("86") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-rocker-of-dpa-groups") },
        { "ret-type", QLIT_QSTR("[87]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-tpm-models") },
        { "ret-type", QLIT_QSTR("[88]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-tpm-types") },
        { "ret-type", QLIT_QSTR("[89]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-tpm") },
        { "ret-type", QLIT_QSTR("[90]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("91") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("set_password") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("92") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("expire_password") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("93") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("screendump") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-spice") },
        { "ret-type", QLIT_QSTR("94") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("95") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("SPICE_CONNECTED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("96") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("SPICE_INITIALIZED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("97") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("SPICE_DISCONNECTED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("SPICE_MIGRATE_COMPLETED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-vnc") },
        { "ret-type", QLIT_QSTR("98") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-vnc-servers") },
        { "ret-type", QLIT_QSTR("[99]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("100") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("change-vnc-password") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("101") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("VNC_CONNECTED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("102") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("VNC_INITIALIZED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("103") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("VNC_DISCONNECTED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-mice") },
        { "ret-type", QLIT_QSTR("[104]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("105") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("send-key") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("106") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("input-send-event") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-migrate") },
        { "ret-type", QLIT_QSTR("107") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("108") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate-set-capabilities") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-migrate-capabilities") },
        { "ret-type", QLIT_QSTR("[109]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("110") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate-set-parameters") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-migrate-parameters") },
        { "ret-type", QLIT_QSTR("111") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("112") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("client_migrate_info") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate-start-postcopy") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("113") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("MIGRATION") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("114") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("MIGRATION_PASS") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("x-colo-lost-heartbeat") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate_cancel") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("115") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate-continue") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("116") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate_set_downtime") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("117") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate_set_speed") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("118") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate-set-cache-size") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-migrate-cache-size") },
        { "ret-type", QLIT_QSTR("int") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("119") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("120") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("migrate-incoming") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("121") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("xen-save-devices-state") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("122") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("xen-set-replication") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-xen-replication-status") },
        { "ret-type", QLIT_QSTR("123") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("xen-colo-do-checkpoint") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("124") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("transaction") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("125") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("trace-event-get-state") },
        { "ret-type", QLIT_QSTR("[126]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("127") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("trace-event-set-state") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-qmp-schema") },
        { "ret-type", QLIT_QSTR("[128]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("129") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("qmp_capabilities") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-version") },
        { "ret-type", QLIT_QSTR("130") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-commands") },
        { "ret-type", QLIT_QSTR("[131]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("132") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("add_client") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-name") },
        { "ret-type", QLIT_QSTR("133") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-kvm") },
        { "ret-type", QLIT_QSTR("134") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-uuid") },
        { "ret-type", QLIT_QSTR("135") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-events") },
        { "ret-type", QLIT_QSTR("[136]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-cpus") },
        { "ret-type", QLIT_QSTR("[137]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-cpus-fast") },
        { "ret-type", QLIT_QSTR("[138]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-iothreads") },
        { "ret-type", QLIT_QSTR("[139]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-balloon") },
        { "ret-type", QLIT_QSTR("140") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("141") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("BALLOON_CHANGE") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-pci") },
        { "ret-type", QLIT_QSTR("[142]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("quit") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("stop") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("system_reset") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("system_powerdown") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("143") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("cpu-add") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("144") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("memsave") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("145") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("pmemsave") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("cont") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("system_wakeup") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("inject-nmi") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("146") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("balloon") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("147") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("human-monitor-command") },
        { "ret-type", QLIT_QSTR("str") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("148") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("qom-list") },
        { "ret-type", QLIT_QSTR("[149]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("150") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("qom-get") },
        { "ret-type", QLIT_QSTR("any") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("151") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("qom-set") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("152") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("change") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("153") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("qom-list-types") },
        { "ret-type", QLIT_QSTR("[154]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("155") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("device-list-properties") },
        { "ret-type", QLIT_QSTR("[149]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("156") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("qom-list-properties") },
        { "ret-type", QLIT_QSTR("[149]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("157") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("xen-set-global-dirty-log") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("158") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("device_add") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("159") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("device_del") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("160") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("DEVICE_DELETED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("161") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("dump-guest-memory") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-dump") },
        { "ret-type", QLIT_QSTR("162") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("163") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("DUMP_COMPLETED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-dump-guest-memory-capability") },
        { "ret-type", QLIT_QSTR("164") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("165") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("dump-skeys") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("166") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("object-add") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("167") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("object-del") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("168") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("getfd") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("169") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("closefd") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-machines") },
        { "ret-type", QLIT_QSTR("[170]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-memory-size-summary") },
        { "ret-type", QLIT_QSTR("171") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-cpu-definitions") },
        { "ret-type", QLIT_QSTR("[172]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("173") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-cpu-model-expansion") },
        { "ret-type", QLIT_QSTR("174") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("175") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-cpu-model-comparison") },
        { "ret-type", QLIT_QSTR("176") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("177") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-cpu-model-baseline") },
        { "ret-type", QLIT_QSTR("178") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("179") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("add-fd") },
        { "ret-type", QLIT_QSTR("180") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("181") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("remove-fd") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-fdsets") },
        { "ret-type", QLIT_QSTR("[182]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-target") },
        { "ret-type", QLIT_QSTR("183") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("184") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-command-line-options") },
        { "ret-type", QLIT_QSTR("[185]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-memdev") },
        { "ret-type", QLIT_QSTR("[186]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-memory-devices") },
        { "ret-type", QLIT_QSTR("[187]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("188") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("MEM_UNPLUG_ERROR") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-acpi-ospm-status") },
        { "ret-type", QLIT_QSTR("[189]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("190") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("ACPI_DEVICE_OST") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("rtc-reset-reinjection") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("191") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("RTC_CHANGE") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("192") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("xen-load-devices-state") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-gic-capabilities") },
        { "ret-type", QLIT_QSTR("[193]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-hotpluggable-cpus") },
        { "ret-type", QLIT_QSTR("[194]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-vm-generation-id") },
        { "ret-type", QLIT_QSTR("195") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-sev") },
        { "ret-type", QLIT_QSTR("196") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-sev-launch-measure") },
        { "ret-type", QLIT_QSTR("197") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(false) },
        { "arg-type", QLIT_QSTR("0") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("query-sev-capabilities") },
        { "ret-type", QLIT_QSTR("198") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("199") },
        { "meta-type", QLIT_QSTR("event") },
        { "name", QLIT_QSTR("COMMAND_DROPPED") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(true) },
        { "arg-type", QLIT_QSTR("200") },
        { "meta-type", QLIT_QSTR("command") },
        { "name", QLIT_QSTR("x-oob-test") },
        { "ret-type", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("0") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("running") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("singlestep") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("status") },
                { "type", QLIT_QSTR("201") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("1") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("guest") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("2") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("guest") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("3") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("action") },
                { "type", QLIT_QSTR("202") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("4") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("action") },
                { "type", QLIT_QSTR("202") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("5") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("action") },
                { "type", QLIT_QSTR("203") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("info") },
                { "type", QLIT_QSTR("204") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("6") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("boundaries") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("boundaries-read") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("boundaries-write") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("boundaries-flush") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("7") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("8") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[8]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("qdev") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("removable") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("locked") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("inserted") },
                { "type", QLIT_QSTR("20") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tray_open") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("io-status") },
                { "type", QLIT_QSTR("205") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("dirty-bitmaps") },
                { "type", QLIT_QSTR("[206]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("8") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("query-nodes") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("9") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("10") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[10]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("stats") },
                { "type", QLIT_QSTR("207") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("parent") },
                { "type", QLIT_QSTR("10") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing") },
                { "type", QLIT_QSTR("10") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("10") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("11") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[11]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("len") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("busy") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("paused") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("io-status") },
                { "type", QLIT_QSTR("205") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ready") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("status") },
                { "type", QLIT_QSTR("208") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("auto-finalize") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("auto-dismiss") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("11") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("password") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("12") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("13") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("snapshot-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("snapshot-node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("mode") },
                { "type", QLIT_QSTR("209") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("14") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("overlay") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("15") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("image-node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("backing-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("16") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("job-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("base") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("top") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("filter-node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("17") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("job-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("target") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sync") },
                { "type", QLIT_QSTR("210") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("mode") },
                { "type", QLIT_QSTR("209") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bitmap") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compress") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-source-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-target-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auto-finalize") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auto-dismiss") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("18") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("job-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("target") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sync") },
                { "type", QLIT_QSTR("210") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compress") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-source-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-target-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auto-finalize") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auto-dismiss") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("19") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("20") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[20]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ro") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("drv") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing_file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("backing_file_depth") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("encrypted") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("encryption_key_missing") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("detect_zeroes") },
                { "type", QLIT_QSTR("212") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bps") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bps_rd") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bps_wr") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("iops") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("iops_rd") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("iops_wr") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("image") },
                { "type", QLIT_QSTR("213") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_rd_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_wr_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_rd_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_wr_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_rd_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_wr_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_rd_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_wr_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("group") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cache") },
                { "type", QLIT_QSTR("214") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("write_threshold") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("20") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("job-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("target") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("replaces") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sync") },
                { "type", QLIT_QSTR("210") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("mode") },
                { "type", QLIT_QSTR("209") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("granularity") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("buf-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-source-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-target-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("unmap") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("21") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("granularity") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("persistent") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("autoload") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("22") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("23") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sha256") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("24") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("job-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("target") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("replaces") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sync") },
                { "type", QLIT_QSTR("210") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("granularity") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("buf-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-source-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-target-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("filter-node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("25") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bps") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bps_rd") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bps_wr") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("iops") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("iops_rd") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("iops_wr") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_rd_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_wr_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_rd_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_wr_max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_rd_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bps_wr_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_rd_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_wr_max_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iops_size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("group") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("26") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("job-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("base") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("base-node") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("on-error") },
                { "type", QLIT_QSTR("211") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("27") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("28") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("force") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("29") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("30") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("31") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("32") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("33") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("34") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("driver") },
                { "type", QLIT_QSTR("215") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("discard") },
                { "type", QLIT_QSTR("216") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cache") },
                { "type", QLIT_QSTR("217") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("read-only") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("force-share") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("detect-zeroes") },
                { "type", QLIT_QSTR("212") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("35") },
        { "tag", QLIT_QSTR("driver") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blkdebug") },
                { "type", QLIT_QSTR("218") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blkverify") },
                { "type", QLIT_QSTR("219") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("bochs") },
                { "type", QLIT_QSTR("220") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("cloop") },
                { "type", QLIT_QSTR("220") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("dmg") },
                { "type", QLIT_QSTR("220") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("221") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ftp") },
                { "type", QLIT_QSTR("222") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ftps") },
                { "type", QLIT_QSTR("223") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("gluster") },
                { "type", QLIT_QSTR("224") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("host_cdrom") },
                { "type", QLIT_QSTR("221") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("host_device") },
                { "type", QLIT_QSTR("221") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("http") },
                { "type", QLIT_QSTR("225") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("https") },
                { "type", QLIT_QSTR("226") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("iscsi") },
                { "type", QLIT_QSTR("227") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("luks") },
                { "type", QLIT_QSTR("228") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("nbd") },
                { "type", QLIT_QSTR("229") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("nfs") },
                { "type", QLIT_QSTR("230") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("null-aio") },
                { "type", QLIT_QSTR("231") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("null-co") },
                { "type", QLIT_QSTR("231") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("nvme") },
                { "type", QLIT_QSTR("232") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("parallels") },
                { "type", QLIT_QSTR("220") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qcow2") },
                { "type", QLIT_QSTR("233") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qcow") },
                { "type", QLIT_QSTR("234") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qed") },
                { "type", QLIT_QSTR("235") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("quorum") },
                { "type", QLIT_QSTR("236") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("raw") },
                { "type", QLIT_QSTR("237") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("rbd") },
                { "type", QLIT_QSTR("238") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("replication") },
                { "type", QLIT_QSTR("239") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("sheepdog") },
                { "type", QLIT_QSTR("240") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ssh") },
                { "type", QLIT_QSTR("241") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("throttle") },
                { "type", QLIT_QSTR("242") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vdi") },
                { "type", QLIT_QSTR("220") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vhdx") },
                { "type", QLIT_QSTR("220") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vmdk") },
                { "type", QLIT_QSTR("235") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vpc") },
                { "type", QLIT_QSTR("220") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vvfat") },
                { "type", QLIT_QSTR("243") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vxhs") },
                { "type", QLIT_QSTR("244") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("36") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("driver") },
                { "type", QLIT_QSTR("215") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("37") },
        { "tag", QLIT_QSTR("driver") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blkdebug") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blkverify") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("bochs") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("cloop") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("dmg") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("246") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ftp") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ftps") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("gluster") },
                { "type", QLIT_QSTR("247") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("host_cdrom") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("host_device") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("http") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("https") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("iscsi") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("luks") },
                { "type", QLIT_QSTR("248") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("nbd") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("nfs") },
                { "type", QLIT_QSTR("249") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("null-aio") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("null-co") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("nvme") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("parallels") },
                { "type", QLIT_QSTR("250") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qcow") },
                { "type", QLIT_QSTR("251") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qcow2") },
                { "type", QLIT_QSTR("252") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qed") },
                { "type", QLIT_QSTR("253") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("quorum") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("raw") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("rbd") },
                { "type", QLIT_QSTR("254") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("replication") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("sheepdog") },
                { "type", QLIT_QSTR("255") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ssh") },
                { "type", QLIT_QSTR("256") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("throttle") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vdi") },
                { "type", QLIT_QSTR("257") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vhdx") },
                { "type", QLIT_QSTR("258") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vmdk") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vpc") },
                { "type", QLIT_QSTR("259") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vvfat") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vxhs") },
                { "type", QLIT_QSTR("245") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("force") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("38") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("39") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("40") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("41") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("read-only-mode") },
                { "type", QLIT_QSTR("260") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("42") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("msg") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fatal") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("43") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("operation") },
                { "type", QLIT_QSTR("261") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("action") },
                { "type", QLIT_QSTR("262") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("nospace") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("reason") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("44") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("263") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("len") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("error") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("45") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("263") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("len") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("46") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("operation") },
                { "type", QLIT_QSTR("261") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("action") },
                { "type", QLIT_QSTR("262") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("47") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("263") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("len") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("48") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("263") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("49") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("amount-exceeded") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("write-threshold") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("50") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("write-threshold") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("51") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("parent") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("child") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("52") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("iothread") },
                { "type", QLIT_QSTR("264") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("force") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("53") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("54") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("55") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vm-state-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("date-sec") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("date-nsec") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vm-clock-sec") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vm-clock-nsec") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("56") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("force") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("57") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("addr") },
                { "type", QLIT_QSTR("265") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-creds") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("58") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("writable") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("59") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("mode") },
                { "type", QLIT_QSTR("266") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("60") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("tray-open") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("61") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("reference") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sector-num") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sectors-count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("62") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("267") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("error") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sector-num") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("sectors-count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("63") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("64") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[64]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("label") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("frontend-open") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("64") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("65") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[65]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("65") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("268") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("66") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("268") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("67") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("string") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("str") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("backend") },
                { "type", QLIT_QSTR("269") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("68") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pty") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("69") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("backend") },
                { "type", QLIT_QSTR("269") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("70") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("71") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("72") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("open") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("73") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("up") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("74") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("75") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("76") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("77") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("78") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[78]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("promiscuous") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("multicast") },
                { "type", QLIT_QSTR("270") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("unicast") },
                { "type", QLIT_QSTR("270") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vlan") },
                { "type", QLIT_QSTR("270") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("broadcast-allowed") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("multicast-overflow") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("unicast-overflow") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("main-mac") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vlan-table") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("unicast-table") },
                { "type", QLIT_QSTR("[str]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("multicast-table") },
                { "type", QLIT_QSTR("[str]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("78") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("79") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("80") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ports") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("81") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("82") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("83") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[83]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enabled") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("link-up") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("speed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("duplex") },
                { "type", QLIT_QSTR("271") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("autoneg") },
                { "type", QLIT_QSTR("272") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("83") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tbl-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("84") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("85") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[85]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cookie") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("hits") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("key") },
                { "type", QLIT_QSTR("273") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("mask") },
                { "type", QLIT_QSTR("274") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("action") },
                { "type", QLIT_QSTR("275") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("85") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("86") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("87") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[87]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vlan-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pport") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("index") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("out-pport") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("group-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("set-vlan-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pop-vlan") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("group-ids") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("set-eth-src") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("set-eth-dst") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ttl-check") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("87") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("88") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[88]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("88") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("tpm-tis"),
            QLIT_QSTR("tpm-crb"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("89") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[89]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("89") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("passthrough"),
            QLIT_QSTR("emulator"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("90") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[90]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("model") },
                { "type", QLIT_QSTR("88") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("options") },
                { "type", QLIT_QSTR("276") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("90") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("protocol") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("password") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("connected") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("91") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("protocol") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("time") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("92") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("head") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("93") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enabled") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("migrated") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-port") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auth") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compiled-version") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("mouse-mode") },
                { "type", QLIT_QSTR("277") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("channels") },
                { "type", QLIT_QSTR("[278]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("94") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("279") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("client") },
                { "type", QLIT_QSTR("279") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("95") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("280") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("client") },
                { "type", QLIT_QSTR("278") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("96") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("279") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("client") },
                { "type", QLIT_QSTR("279") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("97") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enabled") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("service") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auth") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("clients") },
                { "type", QLIT_QSTR("[282]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("98") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("99") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[99]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("[283]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("clients") },
                { "type", QLIT_QSTR("[282]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("auth") },
                { "type", QLIT_QSTR("284") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vencrypt") },
                { "type", QLIT_QSTR("285") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("display") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("99") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("password") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("100") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("286") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("client") },
                { "type", QLIT_QSTR("287") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("101") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("286") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("client") },
                { "type", QLIT_QSTR("282") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("102") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("286") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("client") },
                { "type", QLIT_QSTR("282") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("103") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("104") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[104]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("index") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("current") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("absolute") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("104") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("keys") },
                { "type", QLIT_QSTR("[288]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("hold-time") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("105") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("head") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("events") },
                { "type", QLIT_QSTR("[289]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("106") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("status") },
                { "type", QLIT_QSTR("290") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ram") },
                { "type", QLIT_QSTR("291") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("disk") },
                { "type", QLIT_QSTR("291") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("xbzrle-cache") },
                { "type", QLIT_QSTR("292") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("total-time") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("expected-downtime") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("downtime") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("setup-time") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cpu-throttle-percentage") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("error-desc") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("107") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("capabilities") },
                { "type", QLIT_QSTR("[109]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("108") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("109") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[109]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("capability") },
                { "type", QLIT_QSTR("293") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("state") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("109") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compress-level") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compress-threads") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("decompress-threads") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cpu-throttle-initial") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cpu-throttle-increment") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-creds") },
                { "type", QLIT_QSTR("264") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-hostname") },
                { "type", QLIT_QSTR("264") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("max-bandwidth") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("downtime-limit") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x-checkpoint-delay") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("block-incremental") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x-multifd-channels") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x-multifd-page-count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("xbzrle-cache-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("110") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compress-level") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compress-threads") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("decompress-threads") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cpu-throttle-initial") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cpu-throttle-increment") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-creds") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-hostname") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("max-bandwidth") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("downtime-limit") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x-checkpoint-delay") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("block-incremental") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x-multifd-channels") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x-multifd-page-count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("xbzrle-cache-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("111") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("protocol") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("hostname") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-port") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cert-subject") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("112") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("status") },
                { "type", QLIT_QSTR("290") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("113") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("pass") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("114") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("state") },
                { "type", QLIT_QSTR("290") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("115") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("value") },
                { "type", QLIT_QSTR("number") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("116") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("value") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("117") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("value") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("118") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("int") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("int") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("uri") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("blk") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("inc") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("detach") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("119") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("uri") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("120") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("live") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("121") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enable") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("primary") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("failover") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("122") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("error") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("desc") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("123") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("actions") },
                { "type", QLIT_QSTR("[294]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("properties") },
                { "type", QLIT_QSTR("295") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("124") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vcpu") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("125") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("126") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[126]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("state") },
                { "type", QLIT_QSTR("296") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vcpu") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("126") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enable") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ignore-unavailable") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vcpu") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("127") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("128") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[128]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("meta-type") },
                { "type", QLIT_QSTR("297") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("128") },
        { "tag", QLIT_QSTR("meta-type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("builtin") },
                { "type", QLIT_QSTR("298") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("enum") },
                { "type", QLIT_QSTR("299") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("array") },
                { "type", QLIT_QSTR("300") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("object") },
                { "type", QLIT_QSTR("301") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("alternate") },
                { "type", QLIT_QSTR("302") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("command") },
                { "type", QLIT_QSTR("303") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("event") },
                { "type", QLIT_QSTR("304") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("enable") },
                { "type", QLIT_QSTR("[305]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("129") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("qemu") },
                { "type", QLIT_QSTR("306") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("package") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("130") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("131") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[131]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("131") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("protocol") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fdname") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("skipauth") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("132") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("133") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enabled") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("present") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("134") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("UUID") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("135") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("136") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[136]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("136") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("137") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[137]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("CPU") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("current") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("halted") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("qom_path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("thread_id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("props") },
                { "type", QLIT_QSTR("307") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arch") },
                { "type", QLIT_QSTR("308") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("137") },
        { "tag", QLIT_QSTR("arch") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("x86") },
                { "type", QLIT_QSTR("309") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("sparc") },
                { "type", QLIT_QSTR("310") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ppc") },
                { "type", QLIT_QSTR("311") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("mips") },
                { "type", QLIT_QSTR("312") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("tricore") },
                { "type", QLIT_QSTR("313") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s390") },
                { "type", QLIT_QSTR("314") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("riscv") },
                { "type", QLIT_QSTR("315") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("other") },
                { "type", QLIT_QSTR("316") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("138") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[138]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cpu-index") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("qom-path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("thread-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("props") },
                { "type", QLIT_QSTR("307") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arch") },
                { "type", QLIT_QSTR("308") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("138") },
        { "tag", QLIT_QSTR("arch") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("x86") },
                { "type", QLIT_QSTR("316") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("sparc") },
                { "type", QLIT_QSTR("316") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ppc") },
                { "type", QLIT_QSTR("316") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("mips") },
                { "type", QLIT_QSTR("316") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("tricore") },
                { "type", QLIT_QSTR("316") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s390") },
                { "type", QLIT_QSTR("314") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("riscv") },
                { "type", QLIT_QSTR("315") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("other") },
                { "type", QLIT_QSTR("316") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("139") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[139]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("thread-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("poll-max-ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("poll-grow") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("poll-shrink") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("139") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("actual") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("140") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("actual") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("141") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("142") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[142]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bus") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("devices") },
                { "type", QLIT_QSTR("[317]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("142") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("143") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("val") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cpu-index") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("144") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("val") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("145") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("value") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("146") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("command-line") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cpu-index") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("147") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("148") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("149") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[149]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("description") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("149") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("property") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("150") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("value") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("any") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("property") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("value") },
                { "type", QLIT_QSTR("any") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("151") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("target") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("arg") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("152") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("implements") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("abstract") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("153") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("154") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[154]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("abstract") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("parent") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("154") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("typename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("155") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("typename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("156") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enable") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("157") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("driver") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("bus") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("158") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("159") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("160") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("paging") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("protocol") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("detach") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("begin") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("318") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("161") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("status") },
                { "type", QLIT_QSTR("319") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("completed") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("total") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("162") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("result") },
                { "type", QLIT_QSTR("162") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("error") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("163") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("formats") },
                { "type", QLIT_QSTR("[318]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("164") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("165") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("qom-type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("props") },
                { "type", QLIT_QSTR("any") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("166") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("167") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fdname") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("168") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fdname") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("169") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("170") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[170]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("alias") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("is-default") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cpu-max") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("hotpluggable-cpus") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("170") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("base-memory") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("plugged-memory") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("171") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("172") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[172]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("migration-safe") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("static") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("unavailable-features") },
                { "type", QLIT_QSTR("[str]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("typename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("172") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("320") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("model") },
                { "type", QLIT_QSTR("321") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("173") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("model") },
                { "type", QLIT_QSTR("321") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("174") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("modela") },
                { "type", QLIT_QSTR("321") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("modelb") },
                { "type", QLIT_QSTR("321") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("175") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("result") },
                { "type", QLIT_QSTR("322") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("responsible-properties") },
                { "type", QLIT_QSTR("[str]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("176") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("modela") },
                { "type", QLIT_QSTR("321") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("modelb") },
                { "type", QLIT_QSTR("321") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("177") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("model") },
                { "type", QLIT_QSTR("321") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("178") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("fdset-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("opaque") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("179") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fdset-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fd") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("180") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fdset-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("fd") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("181") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("182") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[182]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fdset-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fds") },
                { "type", QLIT_QSTR("[323]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("182") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arch") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("183") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("option") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("184") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("185") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[185]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("option") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("parameters") },
                { "type", QLIT_QSTR("[324]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("185") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("186") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[186]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("merge") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dump") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("prealloc") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host-nodes") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("policy") },
                { "type", QLIT_QSTR("325") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("186") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("187") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[187]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("326") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("187") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("dimm") },
                { "type", QLIT_QSTR("327") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("nvdimm") },
                { "type", QLIT_QSTR("327") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("msg") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("188") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("189") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[189]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("slot") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("slot-type") },
                { "type", QLIT_QSTR("328") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("source") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("status") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("189") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("info") },
                { "type", QLIT_QSTR("189") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("190") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("191") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("192") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("193") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[193]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("version") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("emulated") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("kernel") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("193") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("194") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[194]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vcpus-count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("props") },
                { "type", QLIT_QSTR("307") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("qom-path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("194") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("guid") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("195") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enabled") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("api-major") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("api-minor") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("build-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("policy") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("state") },
                { "type", QLIT_QSTR("329") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("handle") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("196") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("197") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("pdh") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cert-chain") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cbitpos") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("reduced-phys-bits") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("198") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("any") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("reason") },
                { "type", QLIT_QSTR("330") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("199") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("lock") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("200") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("boolean") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("bool") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("201") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("debug"),
            QLIT_QSTR("inmigrate"),
            QLIT_QSTR("internal-error"),
            QLIT_QSTR("io-error"),
            QLIT_QSTR("paused"),
            QLIT_QSTR("postmigrate"),
            QLIT_QSTR("prelaunch"),
            QLIT_QSTR("finish-migrate"),
            QLIT_QSTR("restore-vm"),
            QLIT_QSTR("running"),
            QLIT_QSTR("save-vm"),
            QLIT_QSTR("shutdown"),
            QLIT_QSTR("suspended"),
            QLIT_QSTR("watchdog"),
            QLIT_QSTR("guest-panicked"),
            QLIT_QSTR("colo"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("202") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("reset"),
            QLIT_QSTR("shutdown"),
            QLIT_QSTR("poweroff"),
            QLIT_QSTR("pause"),
            QLIT_QSTR("debug"),
            QLIT_QSTR("none"),
            QLIT_QSTR("inject-nmi"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("203") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("pause"),
            QLIT_QSTR("poweroff"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("331") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("204") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("hyper-v") },
                { "type", QLIT_QSTR("332") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s390") },
                { "type", QLIT_QSTR("333") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("int") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[int]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("205") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("ok"),
            QLIT_QSTR("failed"),
            QLIT_QSTR("nospace"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("206") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[206]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("granularity") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("status") },
                { "type", QLIT_QSTR("334") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("206") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("rd_bytes") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("wr_bytes") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("rd_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("wr_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("flush_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("flush_total_time_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("wr_total_time_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("rd_total_time_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("wr_highest_offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("rd_merged") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("wr_merged") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("idle_time_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("failed_rd_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("failed_wr_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("failed_flush_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("invalid_rd_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("invalid_wr_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("invalid_flush_operations") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("account_invalid") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("account_failed") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("timed_stats") },
                { "type", QLIT_QSTR("[335]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x_rd_latency_histogram") },
                { "type", QLIT_QSTR("336") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x_wr_latency_histogram") },
                { "type", QLIT_QSTR("336") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x_flush_latency_histogram") },
                { "type", QLIT_QSTR("336") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("207") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("208") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("undefined"),
            QLIT_QSTR("created"),
            QLIT_QSTR("running"),
            QLIT_QSTR("paused"),
            QLIT_QSTR("ready"),
            QLIT_QSTR("standby"),
            QLIT_QSTR("waiting"),
            QLIT_QSTR("pending"),
            QLIT_QSTR("aborting"),
            QLIT_QSTR("concluded"),
            QLIT_QSTR("null"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("209") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("existing"),
            QLIT_QSTR("absolute-paths"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("210") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("top"),
            QLIT_QSTR("full"),
            QLIT_QSTR("none"),
            QLIT_QSTR("incremental"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("211") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("report"),
            QLIT_QSTR("ignore"),
            QLIT_QSTR("enospc"),
            QLIT_QSTR("stop"),
            QLIT_QSTR("auto"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("212") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("off"),
            QLIT_QSTR("on"),
            QLIT_QSTR("unmap"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("dirty-flag") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("actual-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("virtual-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cluster-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("encrypted") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("compressed") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("full-backing-filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-filename-format") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("snapshots") },
                { "type", QLIT_QSTR("[56]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-image") },
                { "type", QLIT_QSTR("213") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("format-specific") },
                { "type", QLIT_QSTR("337") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("213") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("writeback") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("direct") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("no-flush") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("214") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("215") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("blkdebug"),
            QLIT_QSTR("blkverify"),
            QLIT_QSTR("bochs"),
            QLIT_QSTR("cloop"),
            QLIT_QSTR("dmg"),
            QLIT_QSTR("file"),
            QLIT_QSTR("ftp"),
            QLIT_QSTR("ftps"),
            QLIT_QSTR("gluster"),
            QLIT_QSTR("host_cdrom"),
            QLIT_QSTR("host_device"),
            QLIT_QSTR("http"),
            QLIT_QSTR("https"),
            QLIT_QSTR("iscsi"),
            QLIT_QSTR("luks"),
            QLIT_QSTR("nbd"),
            QLIT_QSTR("nfs"),
            QLIT_QSTR("null-aio"),
            QLIT_QSTR("null-co"),
            QLIT_QSTR("nvme"),
            QLIT_QSTR("parallels"),
            QLIT_QSTR("qcow"),
            QLIT_QSTR("qcow2"),
            QLIT_QSTR("qed"),
            QLIT_QSTR("quorum"),
            QLIT_QSTR("raw"),
            QLIT_QSTR("rbd"),
            QLIT_QSTR("replication"),
            QLIT_QSTR("sheepdog"),
            QLIT_QSTR("ssh"),
            QLIT_QSTR("throttle"),
            QLIT_QSTR("vdi"),
            QLIT_QSTR("vhdx"),
            QLIT_QSTR("vmdk"),
            QLIT_QSTR("vpc"),
            QLIT_QSTR("vvfat"),
            QLIT_QSTR("vxhs"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("216") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("ignore"),
            QLIT_QSTR("unmap"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("direct") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("no-flush") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("217") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("image") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("config") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("align") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("max-transfer") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("opt-write-zero") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("max-write-zero") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("opt-discard") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("max-discard") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("inject-error") },
                { "type", QLIT_QSTR("[339]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("set-state") },
                { "type", QLIT_QSTR("[340]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("218") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("test") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("raw") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("219") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("220") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pr-manager") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("locking") },
                { "type", QLIT_QSTR("341") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("aio") },
                { "type", QLIT_QSTR("342") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("221") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("url") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("readahead") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("timeout") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("222") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("url") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("readahead") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("timeout") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("sslverify") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("223") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("volume") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("[343]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("debug") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("224") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("url") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("readahead") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("timeout") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cookie") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cookie-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("225") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("url") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("readahead") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("timeout") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("proxy-password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cookie") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("sslverify") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cookie-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("226") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("transport") },
                { "type", QLIT_QSTR("344") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("portal") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("target") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("lun") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("user") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("password-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("initiator-name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("header-digest") },
                { "type", QLIT_QSTR("345") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("timeout") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("227") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("key-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("228") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("343") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("exp") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-creds") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("229") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("346") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("user") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("group") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tcp-syn-count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("readahead-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("page-cache-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("debug") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("230") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("latency-ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("231") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("namespace") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("232") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing") },
                { "type", QLIT_QSTR("347") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("lazy-refcounts") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pass-discard-request") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pass-discard-snapshot") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pass-discard-other") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("overlap-check") },
                { "type", QLIT_QSTR("348") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cache-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("l2-cache-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("l2-cache-entry-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("refcount-cache-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cache-clean-interval") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("encrypt") },
                { "type", QLIT_QSTR("349") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("233") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing") },
                { "type", QLIT_QSTR("347") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("encrypt") },
                { "type", QLIT_QSTR("350") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("234") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing") },
                { "type", QLIT_QSTR("347") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("235") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("blkverify") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("children") },
                { "type", QLIT_QSTR("[338]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vote-threshold") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("rewrite-corrupted") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("read-pattern") },
                { "type", QLIT_QSTR("351") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("236") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("237") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("pool") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("image") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("conf") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("snapshot") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("user") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("[352]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("238") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("mode") },
                { "type", QLIT_QSTR("353") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("top-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("239") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("343") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vdi") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("snap-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tag") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("240") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("354") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("user") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("host-key-check") },
                { "type", QLIT_QSTR("355") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("241") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("throttle-group") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("242") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dir") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("fat-type") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("floppy") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("label") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("rw") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("243") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vdisk-id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("352") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-creds") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("244") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("245") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("filename") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("preallocation") },
                { "type", QLIT_QSTR("356") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("nocow") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("246") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("location") },
                { "type", QLIT_QSTR("224") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("preallocation") },
                { "type", QLIT_QSTR("356") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("247") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("key-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cipher-alg") },
                { "type", QLIT_QSTR("357") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cipher-mode") },
                { "type", QLIT_QSTR("358") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ivgen-alg") },
                { "type", QLIT_QSTR("359") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ivgen-hash-alg") },
                { "type", QLIT_QSTR("360") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("hash-alg") },
                { "type", QLIT_QSTR("360") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iter-time") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("248") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("location") },
                { "type", QLIT_QSTR("230") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("249") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cluster-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("250") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("encrypt") },
                { "type", QLIT_QSTR("361") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("251") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("version") },
                { "type", QLIT_QSTR("362") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-fmt") },
                { "type", QLIT_QSTR("215") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("encrypt") },
                { "type", QLIT_QSTR("361") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cluster-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("preallocation") },
                { "type", QLIT_QSTR("356") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("lazy-refcounts") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("refcount-bits") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("252") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-fmt") },
                { "type", QLIT_QSTR("215") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cluster-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("table-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("253") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("location") },
                { "type", QLIT_QSTR("238") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cluster-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("254") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("location") },
                { "type", QLIT_QSTR("240") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("backing-file") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("preallocation") },
                { "type", QLIT_QSTR("356") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("redundancy") },
                { "type", QLIT_QSTR("363") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("object-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("255") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("location") },
                { "type", QLIT_QSTR("241") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("256") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("preallocation") },
                { "type", QLIT_QSTR("356") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("257") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("log-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("block-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("subformat") },
                { "type", QLIT_QSTR("364") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("block-state-zero") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("258") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("338") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("subformat") },
                { "type", QLIT_QSTR("365") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("force-size") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("259") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("260") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("retain"),
            QLIT_QSTR("read-only"),
            QLIT_QSTR("read-write"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("261") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("read"),
            QLIT_QSTR("write"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("262") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("ignore"),
            QLIT_QSTR("report"),
            QLIT_QSTR("stop"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("263") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("commit"),
            QLIT_QSTR("stream"),
            QLIT_QSTR("mirror"),
            QLIT_QSTR("backup"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("null") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("alternate") },
        { "name", QLIT_QSTR("264") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("366") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("265") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("inet") },
                { "type", QLIT_QSTR("367") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("unix") },
                { "type", QLIT_QSTR("368") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vsock") },
                { "type", QLIT_QSTR("369") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("fd") },
                { "type", QLIT_QSTR("370") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("266") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("safe"),
            QLIT_QSTR("hard"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("267") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("read"),
            QLIT_QSTR("write"),
            QLIT_QSTR("flush"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("268") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("utf8"),
            QLIT_QSTR("base64"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("371") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("269") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("file") },
                { "type", QLIT_QSTR("372") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("serial") },
                { "type", QLIT_QSTR("373") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("parallel") },
                { "type", QLIT_QSTR("373") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("pipe") },
                { "type", QLIT_QSTR("373") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("socket") },
                { "type", QLIT_QSTR("374") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("udp") },
                { "type", QLIT_QSTR("375") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("pty") },
                { "type", QLIT_QSTR("376") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("null") },
                { "type", QLIT_QSTR("376") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("mux") },
                { "type", QLIT_QSTR("377") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("msmouse") },
                { "type", QLIT_QSTR("376") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("wctablet") },
                { "type", QLIT_QSTR("376") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("braille") },
                { "type", QLIT_QSTR("376") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("testdev") },
                { "type", QLIT_QSTR("376") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("stdio") },
                { "type", QLIT_QSTR("378") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("console") },
                { "type", QLIT_QSTR("376") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("spicevmc") },
                { "type", QLIT_QSTR("379") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("spiceport") },
                { "type", QLIT_QSTR("380") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vc") },
                { "type", QLIT_QSTR("381") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("ringbuf") },
                { "type", QLIT_QSTR("382") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("memory") },
                { "type", QLIT_QSTR("382") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("270") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("normal"),
            QLIT_QSTR("none"),
            QLIT_QSTR("all"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("str") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[str]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("271") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("half"),
            QLIT_QSTR("full"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("272") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("off"),
            QLIT_QSTR("on"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("priority") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("tbl-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("in-pport") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tunnel-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vlan-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("eth-type") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("eth-src") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("eth-dst") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ip-proto") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ip-tos") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ip-dst") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("273") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("in-pport") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tunnel-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vlan-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("eth-src") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("eth-dst") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ip-proto") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ip-tos") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("274") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("goto-tbl") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("group-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tunnel-lport") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vlan-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("new-vlan-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("out-pport") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("275") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("383") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("276") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("passthrough") },
                { "type", QLIT_QSTR("384") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("emulator") },
                { "type", QLIT_QSTR("385") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("277") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("client"),
            QLIT_QSTR("server"),
            QLIT_QSTR("unknown"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("278") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[278]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("connection-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("channel-type") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("channel-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("tls") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("278") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("279") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auth") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("280") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("281") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("ipv4"),
            QLIT_QSTR("ipv6"),
            QLIT_QSTR("unix"),
            QLIT_QSTR("vsock"),
            QLIT_QSTR("unknown"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("282") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[282]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("service") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("websocket") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("x509_dname") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("sasl_username") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("282") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("283") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[283]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("service") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("websocket") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("auth") },
                { "type", QLIT_QSTR("284") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("vencrypt") },
                { "type", QLIT_QSTR("285") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("283") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("284") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("none"),
            QLIT_QSTR("vnc"),
            QLIT_QSTR("ra2"),
            QLIT_QSTR("ra2ne"),
            QLIT_QSTR("tight"),
            QLIT_QSTR("ultra"),
            QLIT_QSTR("tls"),
            QLIT_QSTR("vencrypt"),
            QLIT_QSTR("sasl"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("285") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("plain"),
            QLIT_QSTR("tls-none"),
            QLIT_QSTR("x509-none"),
            QLIT_QSTR("tls-vnc"),
            QLIT_QSTR("x509-vnc"),
            QLIT_QSTR("tls-plain"),
            QLIT_QSTR("x509-plain"),
            QLIT_QSTR("tls-sasl"),
            QLIT_QSTR("x509-sasl"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("service") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("websocket") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("auth") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("286") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("service") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("family") },
                { "type", QLIT_QSTR("281") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("websocket") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("287") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("288") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[288]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("386") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("288") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("number") },
                { "type", QLIT_QSTR("387") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qcode") },
                { "type", QLIT_QSTR("388") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("289") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[289]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("389") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("289") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("key") },
                { "type", QLIT_QSTR("390") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("btn") },
                { "type", QLIT_QSTR("391") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("rel") },
                { "type", QLIT_QSTR("392") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("abs") },
                { "type", QLIT_QSTR("392") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("290") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("none"),
            QLIT_QSTR("setup"),
            QLIT_QSTR("cancelling"),
            QLIT_QSTR("cancelled"),
            QLIT_QSTR("active"),
            QLIT_QSTR("postcopy-active"),
            QLIT_QSTR("completed"),
            QLIT_QSTR("failed"),
            QLIT_QSTR("colo"),
            QLIT_QSTR("pre-switchover"),
            QLIT_QSTR("device"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("transferred") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("remaining") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("total") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("duplicate") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("skipped") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("normal") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("normal-bytes") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dirty-pages-rate") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("mbps") },
                { "type", QLIT_QSTR("number") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dirty-sync-count") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("postcopy-requests") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("page-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("291") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cache-size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bytes") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("pages") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cache-miss") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cache-miss-rate") },
                { "type", QLIT_QSTR("number") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("overflow") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("292") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("293") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("xbzrle"),
            QLIT_QSTR("rdma-pin-all"),
            QLIT_QSTR("auto-converge"),
            QLIT_QSTR("zero-blocks"),
            QLIT_QSTR("compress"),
            QLIT_QSTR("events"),
            QLIT_QSTR("postcopy-ram"),
            QLIT_QSTR("x-colo"),
            QLIT_QSTR("release-ram"),
            QLIT_QSTR("block"),
            QLIT_QSTR("return-path"),
            QLIT_QSTR("pause-before-switchover"),
            QLIT_QSTR("x-multifd"),
            QLIT_QSTR("dirty-bitmaps"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("number") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("number") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("294") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[294]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("393") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("294") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("abort") },
                { "type", QLIT_QSTR("394") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("block-dirty-bitmap-add") },
                { "type", QLIT_QSTR("395") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("block-dirty-bitmap-clear") },
                { "type", QLIT_QSTR("396") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blockdev-backup") },
                { "type", QLIT_QSTR("397") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blockdev-snapshot") },
                { "type", QLIT_QSTR("398") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blockdev-snapshot-internal-sync") },
                { "type", QLIT_QSTR("399") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("blockdev-snapshot-sync") },
                { "type", QLIT_QSTR("400") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("drive-backup") },
                { "type", QLIT_QSTR("401") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("completion-mode") },
                { "type", QLIT_QSTR("402") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("295") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("296") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("unavailable"),
            QLIT_QSTR("disabled"),
            QLIT_QSTR("enabled"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("297") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("builtin"),
            QLIT_QSTR("enum"),
            QLIT_QSTR("array"),
            QLIT_QSTR("object"),
            QLIT_QSTR("alternate"),
            QLIT_QSTR("command"),
            QLIT_QSTR("event"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("json-type") },
                { "type", QLIT_QSTR("403") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("298") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("values") },
                { "type", QLIT_QSTR("[str]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("299") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("element-type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("300") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("members") },
                { "type", QLIT_QSTR("[404]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tag") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("variants") },
                { "type", QLIT_QSTR("[405]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("301") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("members") },
                { "type", QLIT_QSTR("[406]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("302") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg-type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ret-type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("allow-oob") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("303") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg-type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("304") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("305") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[305]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("305") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("oob"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("major") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("minor") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("micro") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("306") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("node-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("socket-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("core-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("thread-id") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("307") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("308") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("x86"),
            QLIT_QSTR("sparc"),
            QLIT_QSTR("ppc"),
            QLIT_QSTR("mips"),
            QLIT_QSTR("tricore"),
            QLIT_QSTR("s390"),
            QLIT_QSTR("riscv"),
            QLIT_QSTR("other"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("pc") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("309") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("pc") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("npc") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("310") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("nip") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("311") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("PC") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("312") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("PC") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("313") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cpu-state") },
                { "type", QLIT_QSTR("407") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("314") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("pc") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("315") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("316") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("317") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[317]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bus") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("slot") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("function") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("class_info") },
                { "type", QLIT_QSTR("408") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("409") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("irq") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("qdev_id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("pci_bridge") },
                { "type", QLIT_QSTR("410") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("regions") },
                { "type", QLIT_QSTR("[411]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("317") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("318") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("elf"),
            QLIT_QSTR("kdump-zlib"),
            QLIT_QSTR("kdump-lzo"),
            QLIT_QSTR("kdump-snappy"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("319") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("none"),
            QLIT_QSTR("active"),
            QLIT_QSTR("completed"),
            QLIT_QSTR("failed"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("318") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[318]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("320") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("static"),
            QLIT_QSTR("full"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("props") },
                { "type", QLIT_QSTR("any") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("321") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("322") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("incompatible"),
            QLIT_QSTR("identical"),
            QLIT_QSTR("superset"),
            QLIT_QSTR("subset"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("323") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[323]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fd") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("opaque") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("323") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("324") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[324]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("412") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("help") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("default") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("324") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("325") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("default"),
            QLIT_QSTR("preferred"),
            QLIT_QSTR("bind"),
            QLIT_QSTR("interleave"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("326") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("dimm"),
            QLIT_QSTR("nvdimm"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("413") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("327") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("328") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("DIMM"),
            QLIT_QSTR("CPU"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("329") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("uninit"),
            QLIT_QSTR("launch-update"),
            QLIT_QSTR("launch-secret"),
            QLIT_QSTR("running"),
            QLIT_QSTR("send-update"),
            QLIT_QSTR("receive-update"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("330") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("queue-full"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("331") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("hyper-v"),
            QLIT_QSTR("s390"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg1") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg2") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg3") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg4") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg5") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("332") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("core") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("psw-mask") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("psw-addr") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("reason") },
                { "type", QLIT_QSTR("414") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("333") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("334") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("active"),
            QLIT_QSTR("disabled"),
            QLIT_QSTR("frozen"),
            QLIT_QSTR("locked"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("335") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[335]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("interval_length") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("min_rd_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("max_rd_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("avg_rd_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("min_wr_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("max_wr_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("avg_wr_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("min_flush_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("max_flush_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("avg_flush_latency_ns") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("avg_rd_queue_depth") },
                { "type", QLIT_QSTR("number") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("avg_wr_queue_depth") },
                { "type", QLIT_QSTR("number") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("335") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("boundaries") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bins") },
                { "type", QLIT_QSTR("[int]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("336") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("56") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[56]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("415") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("337") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qcow2") },
                { "type", QLIT_QSTR("416") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vmdk") },
                { "type", QLIT_QSTR("417") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("luks") },
                { "type", QLIT_QSTR("418") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("35") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("alternate") },
        { "name", QLIT_QSTR("338") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("339") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[339]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("event") },
                { "type", QLIT_QSTR("419") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("state") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("errno") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("sector") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("once") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("immediately") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("339") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("340") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[340]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("event") },
                { "type", QLIT_QSTR("419") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("state") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("new_state") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("340") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("341") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("auto"),
            QLIT_QSTR("on"),
            QLIT_QSTR("off"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("342") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("threads"),
            QLIT_QSTR("native"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("343") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[343]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("420") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("343") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("inet") },
                { "type", QLIT_QSTR("354") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("unix") },
                { "type", QLIT_QSTR("421") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("vsock") },
                { "type", QLIT_QSTR("422") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("fd") },
                { "type", QLIT_QSTR("423") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("344") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("tcp"),
            QLIT_QSTR("iser"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("345") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("crc32c"),
            QLIT_QSTR("none"),
            QLIT_QSTR("crc32c-none"),
            QLIT_QSTR("none-crc32c"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("424") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("346") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("35") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("null") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("alternate") },
        { "name", QLIT_QSTR("347") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("425") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("426") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("alternate") },
        { "name", QLIT_QSTR("348") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("427") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("349") },
        { "tag", QLIT_QSTR("format") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("aes") },
                { "type", QLIT_QSTR("428") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("luks") },
                { "type", QLIT_QSTR("429") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("430") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("350") },
        { "tag", QLIT_QSTR("format") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("aes") },
                { "type", QLIT_QSTR("428") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("338") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[338]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("351") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("quorum"),
            QLIT_QSTR("fifo"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("352") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[352]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("352") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("353") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("primary"),
            QLIT_QSTR("secondary"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("host") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("numeric") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("to") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ipv4") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ipv6") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("354") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("mode") },
                { "type", QLIT_QSTR("431") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("355") },
        { "tag", QLIT_QSTR("mode") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("none") },
                { "type", QLIT_QSTR("432") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("hash") },
                { "type", QLIT_QSTR("433") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("known_hosts") },
                { "type", QLIT_QSTR("432") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("356") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("off"),
            QLIT_QSTR("metadata"),
            QLIT_QSTR("falloc"),
            QLIT_QSTR("full"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("357") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("aes-128"),
            QLIT_QSTR("aes-192"),
            QLIT_QSTR("aes-256"),
            QLIT_QSTR("des-rfb"),
            QLIT_QSTR("3des"),
            QLIT_QSTR("cast5-128"),
            QLIT_QSTR("serpent-128"),
            QLIT_QSTR("serpent-192"),
            QLIT_QSTR("serpent-256"),
            QLIT_QSTR("twofish-128"),
            QLIT_QSTR("twofish-192"),
            QLIT_QSTR("twofish-256"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("358") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("ecb"),
            QLIT_QSTR("cbc"),
            QLIT_QSTR("xts"),
            QLIT_QSTR("ctr"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("359") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("plain"),
            QLIT_QSTR("plain64"),
            QLIT_QSTR("essiv"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("360") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("md5"),
            QLIT_QSTR("sha1"),
            QLIT_QSTR("sha224"),
            QLIT_QSTR("sha256"),
            QLIT_QSTR("sha384"),
            QLIT_QSTR("sha512"),
            QLIT_QSTR("ripemd160"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("434") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("361") },
        { "tag", QLIT_QSTR("format") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("qcow") },
                { "type", QLIT_QSTR("428") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("luks") },
                { "type", QLIT_QSTR("435") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("362") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("v2"),
            QLIT_QSTR("v3"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("436") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("363") },
        { "tag", QLIT_QSTR("type") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("full") },
                { "type", QLIT_QSTR("437") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("erasure-coded") },
                { "type", QLIT_QSTR("438") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("364") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("dynamic"),
            QLIT_QSTR("fixed"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("365") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("dynamic"),
            QLIT_QSTR("fixed"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("null") },
        { "meta-type", QLIT_QSTR("builtin") },
        { "name", QLIT_QSTR("null") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("366") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("inet"),
            QLIT_QSTR("unix"),
            QLIT_QSTR("vsock"),
            QLIT_QSTR("fd"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("354") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("367") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("421") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("368") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("422") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("369") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("423") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("370") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("371") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("file"),
            QLIT_QSTR("serial"),
            QLIT_QSTR("parallel"),
            QLIT_QSTR("pipe"),
            QLIT_QSTR("socket"),
            QLIT_QSTR("udp"),
            QLIT_QSTR("pty"),
            QLIT_QSTR("null"),
            QLIT_QSTR("mux"),
            QLIT_QSTR("msmouse"),
            QLIT_QSTR("wctablet"),
            QLIT_QSTR("braille"),
            QLIT_QSTR("testdev"),
            QLIT_QSTR("stdio"),
            QLIT_QSTR("console"),
            QLIT_QSTR("spicevmc"),
            QLIT_QSTR("spiceport"),
            QLIT_QSTR("vc"),
            QLIT_QSTR("ringbuf"),
            QLIT_QSTR("memory"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("439") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("372") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("440") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("373") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("441") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("374") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("442") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("375") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("443") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("376") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("444") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("377") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("445") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("378") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("446") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("379") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("447") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("380") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("448") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("381") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("449") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("382") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("383") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("passthrough"),
            QLIT_QSTR("emulator"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("450") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("384") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("451") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("385") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("386") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("number"),
            QLIT_QSTR("qcode"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("387") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("452") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("388") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("389") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("key"),
            QLIT_QSTR("btn"),
            QLIT_QSTR("rel"),
            QLIT_QSTR("abs"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("453") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("390") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("454") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("391") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("455") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("392") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("393") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("abort"),
            QLIT_QSTR("block-dirty-bitmap-add"),
            QLIT_QSTR("block-dirty-bitmap-clear"),
            QLIT_QSTR("blockdev-backup"),
            QLIT_QSTR("blockdev-snapshot"),
            QLIT_QSTR("blockdev-snapshot-internal-sync"),
            QLIT_QSTR("blockdev-snapshot-sync"),
            QLIT_QSTR("drive-backup"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("456") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("394") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("22") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("395") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("23") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("396") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("19") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("397") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("15") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("398") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("54") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("399") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("14") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("400") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("18") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("401") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("402") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("individual"),
            QLIT_QSTR("grouped"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("403") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("string"),
            QLIT_QSTR("number"),
            QLIT_QSTR("int"),
            QLIT_QSTR("boolean"),
            QLIT_QSTR("null"),
            QLIT_QSTR("object"),
            QLIT_QSTR("array"),
            QLIT_QSTR("value"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("404") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[404]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("name") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("default") },
                { "type", QLIT_QSTR("any") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("404") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("405") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[405]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("case") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("405") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("406") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[406]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("406") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("407") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("uninitialized"),
            QLIT_QSTR("stopped"),
            QLIT_QSTR("check-stop"),
            QLIT_QSTR("operating"),
            QLIT_QSTR("load"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("desc") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("class") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("408") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("vendor") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("409") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bus") },
                { "type", QLIT_QSTR("457") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("devices") },
                { "type", QLIT_QSTR("[317]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("410") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("411") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[411]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bar") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("address") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("prefetch") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("mem_type_64") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("411") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("412") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("string"),
            QLIT_QSTR("boolean"),
            QLIT_QSTR("number"),
            QLIT_QSTR("size"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("id") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("addr") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("slot") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("node") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("memdev") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("hotplugged") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("hotpluggable") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("413") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("414") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("unknown"),
            QLIT_QSTR("disabled-wait"),
            QLIT_QSTR("extint-loop"),
            QLIT_QSTR("pgmint-loop"),
            QLIT_QSTR("opint-loop"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("415") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("qcow2"),
            QLIT_QSTR("vmdk"),
            QLIT_QSTR("luks"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("458") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("416") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("459") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("417") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data") },
                { "type", QLIT_QSTR("460") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("418") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("419") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("l1_update"),
            QLIT_QSTR("l1_grow_alloc_table"),
            QLIT_QSTR("l1_grow_write_table"),
            QLIT_QSTR("l1_grow_activate_table"),
            QLIT_QSTR("l2_load"),
            QLIT_QSTR("l2_update"),
            QLIT_QSTR("l2_update_compressed"),
            QLIT_QSTR("l2_alloc_cow_read"),
            QLIT_QSTR("l2_alloc_write"),
            QLIT_QSTR("read_aio"),
            QLIT_QSTR("read_backing_aio"),
            QLIT_QSTR("read_compressed"),
            QLIT_QSTR("write_aio"),
            QLIT_QSTR("write_compressed"),
            QLIT_QSTR("vmstate_load"),
            QLIT_QSTR("vmstate_save"),
            QLIT_QSTR("cow_read"),
            QLIT_QSTR("cow_write"),
            QLIT_QSTR("reftable_load"),
            QLIT_QSTR("reftable_grow"),
            QLIT_QSTR("reftable_update"),
            QLIT_QSTR("refblock_load"),
            QLIT_QSTR("refblock_update"),
            QLIT_QSTR("refblock_update_part"),
            QLIT_QSTR("refblock_alloc"),
            QLIT_QSTR("refblock_alloc_hookup"),
            QLIT_QSTR("refblock_alloc_write"),
            QLIT_QSTR("refblock_alloc_write_blocks"),
            QLIT_QSTR("refblock_alloc_write_table"),
            QLIT_QSTR("refblock_alloc_switch_table"),
            QLIT_QSTR("cluster_alloc"),
            QLIT_QSTR("cluster_alloc_bytes"),
            QLIT_QSTR("cluster_free"),
            QLIT_QSTR("flush_to_os"),
            QLIT_QSTR("flush_to_disk"),
            QLIT_QSTR("pwritev_rmw_head"),
            QLIT_QSTR("pwritev_rmw_after_head"),
            QLIT_QSTR("pwritev_rmw_tail"),
            QLIT_QSTR("pwritev_rmw_after_tail"),
            QLIT_QSTR("pwritev"),
            QLIT_QSTR("pwritev_zero"),
            QLIT_QSTR("pwritev_done"),
            QLIT_QSTR("empty_image_prepare"),
            QLIT_QSTR("l1_shrink_write_table"),
            QLIT_QSTR("l1_shrink_free_l2_clusters"),
            QLIT_QSTR("cor_write"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("420") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("inet"),
            QLIT_QSTR("unix"),
            QLIT_QSTR("vsock"),
            QLIT_QSTR("fd"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("421") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cid") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("port") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("422") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("str") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("423") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("424") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("inet"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("template") },
                { "type", QLIT_QSTR("426") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("main-header") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("active-l1") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("active-l2") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("refcount-table") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("refcount-block") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("snapshot-table") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("inactive-l1") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("inactive-l2") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("425") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("426") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("none"),
            QLIT_QSTR("constant"),
            QLIT_QSTR("cached"),
            QLIT_QSTR("all"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("427") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("aes"),
            QLIT_QSTR("luks"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("key-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("428") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("key-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("429") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("430") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("aes"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("431") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("none"),
            QLIT_QSTR("hash"),
            QLIT_QSTR("known_hosts"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("432") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("461") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("hash") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("433") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("434") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("qcow"),
            QLIT_QSTR("luks"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("key-secret") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cipher-alg") },
                { "type", QLIT_QSTR("357") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cipher-mode") },
                { "type", QLIT_QSTR("358") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ivgen-alg") },
                { "type", QLIT_QSTR("359") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ivgen-hash-alg") },
                { "type", QLIT_QSTR("360") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("hash-alg") },
                { "type", QLIT_QSTR("360") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iter-time") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("435") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("436") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("full"),
            QLIT_QSTR("erasure-coded"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("copies") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("437") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data-strips") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("parity-strips") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("438") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("in") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("out") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("append") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("439") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("device") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("440") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("addr") },
                { "type", QLIT_QSTR("265") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tls-creds") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("server") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("wait") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("nodelay") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("telnet") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("tn3270") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("reconnect") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("441") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("remote") },
                { "type", QLIT_QSTR("265") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("local") },
                { "type", QLIT_QSTR("265") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("442") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("443") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("chardev") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("444") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("signal") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("445") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("446") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fqdn") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("447") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("width") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("height") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cols") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("rows") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("448") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logfile") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("logappend") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("size") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("449") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("cancel-path") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("450") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("chardev") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("451") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("452") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("unmapped"),
            QLIT_QSTR("shift"),
            QLIT_QSTR("shift_r"),
            QLIT_QSTR("alt"),
            QLIT_QSTR("alt_r"),
            QLIT_QSTR("ctrl"),
            QLIT_QSTR("ctrl_r"),
            QLIT_QSTR("menu"),
            QLIT_QSTR("esc"),
            QLIT_QSTR("1"),
            QLIT_QSTR("2"),
            QLIT_QSTR("3"),
            QLIT_QSTR("4"),
            QLIT_QSTR("5"),
            QLIT_QSTR("6"),
            QLIT_QSTR("7"),
            QLIT_QSTR("8"),
            QLIT_QSTR("9"),
            QLIT_QSTR("0"),
            QLIT_QSTR("minus"),
            QLIT_QSTR("equal"),
            QLIT_QSTR("backspace"),
            QLIT_QSTR("tab"),
            QLIT_QSTR("q"),
            QLIT_QSTR("w"),
            QLIT_QSTR("e"),
            QLIT_QSTR("r"),
            QLIT_QSTR("t"),
            QLIT_QSTR("y"),
            QLIT_QSTR("u"),
            QLIT_QSTR("i"),
            QLIT_QSTR("o"),
            QLIT_QSTR("p"),
            QLIT_QSTR("bracket_left"),
            QLIT_QSTR("bracket_right"),
            QLIT_QSTR("ret"),
            QLIT_QSTR("a"),
            QLIT_QSTR("s"),
            QLIT_QSTR("d"),
            QLIT_QSTR("f"),
            QLIT_QSTR("g"),
            QLIT_QSTR("h"),
            QLIT_QSTR("j"),
            QLIT_QSTR("k"),
            QLIT_QSTR("l"),
            QLIT_QSTR("semicolon"),
            QLIT_QSTR("apostrophe"),
            QLIT_QSTR("grave_accent"),
            QLIT_QSTR("backslash"),
            QLIT_QSTR("z"),
            QLIT_QSTR("x"),
            QLIT_QSTR("c"),
            QLIT_QSTR("v"),
            QLIT_QSTR("b"),
            QLIT_QSTR("n"),
            QLIT_QSTR("m"),
            QLIT_QSTR("comma"),
            QLIT_QSTR("dot"),
            QLIT_QSTR("slash"),
            QLIT_QSTR("asterisk"),
            QLIT_QSTR("spc"),
            QLIT_QSTR("caps_lock"),
            QLIT_QSTR("f1"),
            QLIT_QSTR("f2"),
            QLIT_QSTR("f3"),
            QLIT_QSTR("f4"),
            QLIT_QSTR("f5"),
            QLIT_QSTR("f6"),
            QLIT_QSTR("f7"),
            QLIT_QSTR("f8"),
            QLIT_QSTR("f9"),
            QLIT_QSTR("f10"),
            QLIT_QSTR("num_lock"),
            QLIT_QSTR("scroll_lock"),
            QLIT_QSTR("kp_divide"),
            QLIT_QSTR("kp_multiply"),
            QLIT_QSTR("kp_subtract"),
            QLIT_QSTR("kp_add"),
            QLIT_QSTR("kp_enter"),
            QLIT_QSTR("kp_decimal"),
            QLIT_QSTR("sysrq"),
            QLIT_QSTR("kp_0"),
            QLIT_QSTR("kp_1"),
            QLIT_QSTR("kp_2"),
            QLIT_QSTR("kp_3"),
            QLIT_QSTR("kp_4"),
            QLIT_QSTR("kp_5"),
            QLIT_QSTR("kp_6"),
            QLIT_QSTR("kp_7"),
            QLIT_QSTR("kp_8"),
            QLIT_QSTR("kp_9"),
            QLIT_QSTR("less"),
            QLIT_QSTR("f11"),
            QLIT_QSTR("f12"),
            QLIT_QSTR("print"),
            QLIT_QSTR("home"),
            QLIT_QSTR("pgup"),
            QLIT_QSTR("pgdn"),
            QLIT_QSTR("end"),
            QLIT_QSTR("left"),
            QLIT_QSTR("up"),
            QLIT_QSTR("down"),
            QLIT_QSTR("right"),
            QLIT_QSTR("insert"),
            QLIT_QSTR("delete"),
            QLIT_QSTR("stop"),
            QLIT_QSTR("again"),
            QLIT_QSTR("props"),
            QLIT_QSTR("undo"),
            QLIT_QSTR("front"),
            QLIT_QSTR("copy"),
            QLIT_QSTR("open"),
            QLIT_QSTR("paste"),
            QLIT_QSTR("find"),
            QLIT_QSTR("cut"),
            QLIT_QSTR("lf"),
            QLIT_QSTR("help"),
            QLIT_QSTR("meta_l"),
            QLIT_QSTR("meta_r"),
            QLIT_QSTR("compose"),
            QLIT_QSTR("pause"),
            QLIT_QSTR("ro"),
            QLIT_QSTR("hiragana"),
            QLIT_QSTR("henkan"),
            QLIT_QSTR("yen"),
            QLIT_QSTR("muhenkan"),
            QLIT_QSTR("katakanahiragana"),
            QLIT_QSTR("kp_comma"),
            QLIT_QSTR("kp_equals"),
            QLIT_QSTR("power"),
            QLIT_QSTR("sleep"),
            QLIT_QSTR("wake"),
            QLIT_QSTR("audionext"),
            QLIT_QSTR("audioprev"),
            QLIT_QSTR("audiostop"),
            QLIT_QSTR("audioplay"),
            QLIT_QSTR("audiomute"),
            QLIT_QSTR("volumeup"),
            QLIT_QSTR("volumedown"),
            QLIT_QSTR("mediaselect"),
            QLIT_QSTR("mail"),
            QLIT_QSTR("calculator"),
            QLIT_QSTR("computer"),
            QLIT_QSTR("ac_home"),
            QLIT_QSTR("ac_back"),
            QLIT_QSTR("ac_forward"),
            QLIT_QSTR("ac_refresh"),
            QLIT_QSTR("ac_bookmarks"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("key") },
                { "type", QLIT_QSTR("288") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("down") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("453") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("button") },
                { "type", QLIT_QSTR("462") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("down") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("454") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("axis") },
                { "type", QLIT_QSTR("463") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("value") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("455") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("456") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("number") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("secondary") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("subordinate") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("io_range") },
                { "type", QLIT_QSTR("464") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("memory_range") },
                { "type", QLIT_QSTR("464") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("prefetchable_range") },
                { "type", QLIT_QSTR("464") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("457") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("compat") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("lazy-refcounts") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("corrupt") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("refcount-bits") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("encrypt") },
                { "type", QLIT_QSTR("465") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("458") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("create-type") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cid") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("parent-cid") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("extents") },
                { "type", QLIT_QSTR("[213]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("459") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cipher-alg") },
                { "type", QLIT_QSTR("357") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cipher-mode") },
                { "type", QLIT_QSTR("358") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ivgen-alg") },
                { "type", QLIT_QSTR("359") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("ivgen-hash-alg") },
                { "type", QLIT_QSTR("360") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("hash-alg") },
                { "type", QLIT_QSTR("360") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("payload-offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("master-key-iters") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("uuid") },
                { "type", QLIT_QSTR("str") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("slots") },
                { "type", QLIT_QSTR("[466]") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("460") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("461") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("md5"),
            QLIT_QSTR("sha1"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("462") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("left"),
            QLIT_QSTR("middle"),
            QLIT_QSTR("right"),
            QLIT_QSTR("wheel-up"),
            QLIT_QSTR("wheel-down"),
            QLIT_QSTR("side"),
            QLIT_QSTR("extra"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum") },
        { "name", QLIT_QSTR("463") },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("x"),
            QLIT_QSTR("y"),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("base") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("limit") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("464") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("format") },
                { "type", QLIT_QSTR("427") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("465") },
        { "tag", QLIT_QSTR("format") },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("aes") },
                { "type", QLIT_QSTR("467") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("luks") },
                { "type", QLIT_QSTR("460") },
                {}
            })),
            {}
        })) },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("213") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[213]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("466") },
        { "meta-type", QLIT_QSTR("array") },
        { "name", QLIT_QSTR("[466]") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("active") },
                { "type", QLIT_QSTR("bool") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("iters") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL },
                { "name", QLIT_QSTR("stripes") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("key-offset") },
                { "type", QLIT_QSTR("int") },
                {}
            })),
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("466") },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })) },
        { "meta-type", QLIT_QSTR("object") },
        { "name", QLIT_QSTR("467") },
        {}
    })),
    {}
}));
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_introspect_c;
