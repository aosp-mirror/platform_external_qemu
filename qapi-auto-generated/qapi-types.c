/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * deallocation functions for schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *  Michael Roth      <mdroth@linux.vnet.ibm.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 *
 */

#include "qapi/dealloc-visitor.h"
#include "qapi-types.h"
#include "qapi-visit.h"

const char *ErrorClass_lookup[] = {
    "GenericError",
    "CommandNotFound",
    "DeviceEncrypted",
    "DeviceNotActive",
    "DeviceNotFound",
    "KVMMissingCap",
    NULL,
};

const char *RunState_lookup[] = {
    "debug",
    "inmigrate",
    "internal-error",
    "io-error",
    "paused",
    "postmigrate",
    "prelaunch",
    "finish-migrate",
    "restore-vm",
    "running",
    "save-vm",
    "shutdown",
    "suspended",
    "watchdog",
    "guest-panicked",
    NULL,
};

const char *ImageInfoSpecificKind_lookup[] = {
    "qcow2",
    "vmdk",
    NULL,
};

const char *DataFormat_lookup[] = {
    "utf8",
    "base64",
    NULL,
};

const char *MigrationCapability_lookup[] = {
    "xbzrle",
    "x-rdma-pin-all",
    "auto-converge",
    "zero-blocks",
    NULL,
};

const char *BlockDeviceIoStatus_lookup[] = {
    "ok",
    "failed",
    "nospace",
    NULL,
};

const char *SpiceQueryMouseMode_lookup[] = {
    "client",
    "server",
    "unknown",
    NULL,
};

const char *BlockdevOnError_lookup[] = {
    "report",
    "ignore",
    "enospc",
    "stop",
    NULL,
};

const char *MirrorSyncMode_lookup[] = {
    "top",
    "full",
    "none",
    NULL,
};

const char *BlockJobType_lookup[] = {
    "commit",
    "stream",
    "mirror",
    "backup",
    NULL,
};

const char *NewImageMode_lookup[] = {
    "existing",
    "absolute-paths",
    NULL,
};

const char *TransactionActionKind_lookup[] = {
    "blockdev-snapshot-sync",
    "drive-backup",
    "abort",
    "blockdev-snapshot-internal-sync",
    NULL,
};

const char *NetClientOptionsKind_lookup[] = {
    "none",
    "nic",
    "user",
    "tap",
    "socket",
    "vde",
    "dump",
    "bridge",
    "hubport",
    "netmap",
    NULL,
};

const char *SocketAddressKind_lookup[] = {
    "inet",
    "unix",
    "fd",
    NULL,
};

const char *QKeyCode_lookup[] = {
    "shift",
    "shift_r",
    "alt",
    "alt_r",
    "altgr",
    "altgr_r",
    "ctrl",
    "ctrl_r",
    "menu",
    "esc",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "minus",
    "equal",
    "backspace",
    "tab",
    "q",
    "w",
    "e",
    "r",
    "t",
    "y",
    "u",
    "i",
    "o",
    "p",
    "bracket_left",
    "bracket_right",
    "ret",
    "a",
    "s",
    "d",
    "f",
    "g",
    "h",
    "j",
    "k",
    "l",
    "semicolon",
    "apostrophe",
    "grave_accent",
    "backslash",
    "z",
    "x",
    "c",
    "v",
    "b",
    "n",
    "m",
    "comma",
    "dot",
    "slash",
    "asterisk",
    "spc",
    "caps_lock",
    "f1",
    "f2",
    "f3",
    "f4",
    "f5",
    "f6",
    "f7",
    "f8",
    "f9",
    "f10",
    "num_lock",
    "scroll_lock",
    "kp_divide",
    "kp_multiply",
    "kp_subtract",
    "kp_add",
    "kp_enter",
    "kp_decimal",
    "sysrq",
    "kp_0",
    "kp_1",
    "kp_2",
    "kp_3",
    "kp_4",
    "kp_5",
    "kp_6",
    "kp_7",
    "kp_8",
    "kp_9",
    "less",
    "f11",
    "f12",
    "print",
    "home",
    "pgup",
    "pgdn",
    "end",
    "left",
    "up",
    "down",
    "right",
    "insert",
    "delete",
    "stop",
    "again",
    "props",
    "undo",
    "front",
    "copy",
    "open",
    "paste",
    "find",
    "cut",
    "lf",
    "help",
    "meta_l",
    "meta_r",
    "compose",
    NULL,
};

const char *KeyValueKind_lookup[] = {
    "number",
    "qcode",
    NULL,
};

const char *ChardevBackendKind_lookup[] = {
    "file",
    "serial",
    "parallel",
    "pipe",
    "socket",
    "udp",
    "pty",
    "null",
    "mux",
    "msmouse",
    "braille",
    "stdio",
    "console",
    "spicevmc",
    "spiceport",
    "vc",
    "ringbuf",
    "memory",
    NULL,
};

const char *TpmModel_lookup[] = {
    "tpm-tis",
    NULL,
};

const char *TpmType_lookup[] = {
    "passthrough",
    NULL,
};

const char *TpmTypeOptionsKind_lookup[] = {
    "passthrough",
    NULL,
};

const char *CommandLineParameterType_lookup[] = {
    "string",
    "boolean",
    "number",
    "size",
    NULL,
};

const char *X86CPURegister32_lookup[] = {
    "EAX",
    "EBX",
    "ECX",
    "EDX",
    "ESP",
    "EBP",
    "ESI",
    "EDI",
    NULL,
};

const char *RxState_lookup[] = {
    "normal",
    "none",
    "all",
    NULL,
};

const char *BlockdevDiscardOptions_lookup[] = {
    "ignore",
    "unmap",
    NULL,
};

const char *BlockdevAioOptions_lookup[] = {
    "threads",
    "native",
    NULL,
};

const char *BlockdevOptionsKind_lookup[] = {
    "file",
    "http",
    "https",
    "ftp",
    "ftps",
    "tftp",
    "vvfat",
    "bochs",
    "cloop",
    "cow",
    "dmg",
    "parallels",
    "qcow",
    "qcow2",
    "qed",
    "raw",
    "vdi",
    "vhdx",
    "vmdk",
    "vpc",
    NULL,
};

const char *BlockdevRefKind_lookup[] = {
    "definition",
    "reference",
    NULL,
};

const int BlockdevRef_qtypes[QTYPE_MAX] = {
    [ QTYPE_QDICT ] = BLOCKDEV_REF_KIND_DEFINITION,
    [ QTYPE_QSTRING ] = BLOCKDEV_REF_KIND_REFERENCE,
};

#ifndef QAPI_TYPES_BUILTIN_CLEANUP_DEF_H
#define QAPI_TYPES_BUILTIN_CLEANUP_DEF_H


void qapi_free_strList(strList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_strList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_intList(intList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_intList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_numberList(numberList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_numberList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_boolList(boolList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_boolList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_int8List(int8List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_int8List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_int16List(int16List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_int16List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_int32List(int32List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_int32List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_int64List(int64List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_int64List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_uint8List(uint8List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_uint8List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_uint16List(uint16List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_uint16List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_uint32List(uint32List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_uint32List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

void qapi_free_uint64List(uint64List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_uint64List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

#endif /* QAPI_TYPES_BUILTIN_CLEANUP_DEF_H */


void qapi_free_ErrorClassList(ErrorClassList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ErrorClassList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NameInfoList(NameInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NameInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NameInfo(NameInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NameInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_VersionInfoList(VersionInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VersionInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_VersionInfo(VersionInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VersionInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_KvmInfoList(KvmInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_KvmInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_KvmInfo(KvmInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_KvmInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_RunStateList(RunStateList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_RunStateList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SnapshotInfoList(SnapshotInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SnapshotInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SnapshotInfo(SnapshotInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SnapshotInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfoSpecificQCow2List(ImageInfoSpecificQCow2List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfoSpecificQCow2List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfoSpecificQCow2(ImageInfoSpecificQCow2 * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfoSpecificQCow2(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfoSpecificVmdkList(ImageInfoSpecificVmdkList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfoSpecificVmdkList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfoSpecificVmdk(ImageInfoSpecificVmdk * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfoSpecificVmdk(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfoSpecificList(ImageInfoSpecificList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfoSpecificList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfoSpecific(ImageInfoSpecific * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfoSpecific(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfoList(ImageInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageInfo(ImageInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageCheckList(ImageCheckList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageCheckList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ImageCheck(ImageCheck * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ImageCheck(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_StatusInfoList(StatusInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_StatusInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_StatusInfo(StatusInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_StatusInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_UuidInfoList(UuidInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_UuidInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_UuidInfo(UuidInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_UuidInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevInfoList(ChardevInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevInfo(ChardevInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_DataFormatList(DataFormatList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_DataFormatList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CommandInfoList(CommandInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CommandInfo(CommandInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_EventInfoList(EventInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_EventInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_EventInfo(EventInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_EventInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MigrationStatsList(MigrationStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MigrationStats(MigrationStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_XBZRLECacheStatsList(XBZRLECacheStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_XBZRLECacheStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_XBZRLECacheStats(XBZRLECacheStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_XBZRLECacheStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MigrationInfoList(MigrationInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MigrationInfo(MigrationInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MigrationCapabilityList(MigrationCapabilityList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationCapabilityList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MigrationCapabilityStatusList(MigrationCapabilityStatusList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationCapabilityStatusList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MigrationCapabilityStatus(MigrationCapabilityStatus * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MigrationCapabilityStatus(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MouseInfoList(MouseInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MouseInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MouseInfo(MouseInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MouseInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CpuInfoList(CpuInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CpuInfo(CpuInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDeviceInfoList(BlockDeviceInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDeviceInfo(BlockDeviceInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDeviceIoStatusList(BlockDeviceIoStatusList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceIoStatusList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDeviceMapEntryList(BlockDeviceMapEntryList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceMapEntryList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDeviceMapEntry(BlockDeviceMapEntry * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceMapEntry(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDirtyInfoList(BlockDirtyInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDirtyInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDirtyInfo(BlockDirtyInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDirtyInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockInfoList(BlockInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockInfo(BlockInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDeviceStatsList(BlockDeviceStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockDeviceStats(BlockDeviceStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockDeviceStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockStatsList(BlockStatsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockStatsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockStats(BlockStats * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockStats(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_VncClientInfoList(VncClientInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncClientInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_VncClientInfo(VncClientInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncClientInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_VncInfoList(VncInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_VncInfo(VncInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_VncInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SpiceChannelList(SpiceChannelList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceChannelList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SpiceChannel(SpiceChannel * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceChannel(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SpiceQueryMouseModeList(SpiceQueryMouseModeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceQueryMouseModeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SpiceInfoList(SpiceInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SpiceInfo(SpiceInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SpiceInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BalloonInfoList(BalloonInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BalloonInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BalloonInfo(BalloonInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BalloonInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciMemoryRangeList(PciMemoryRangeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRangeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciMemoryRange(PciMemoryRange * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRange(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciMemoryRegionList(PciMemoryRegionList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRegionList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciMemoryRegion(PciMemoryRegion * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciMemoryRegion(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciBridgeInfoList(PciBridgeInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciBridgeInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciBridgeInfo(PciBridgeInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciBridgeInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciDeviceInfoList(PciDeviceInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciDeviceInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciDeviceInfo(PciDeviceInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciDeviceInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciInfoList(PciInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_PciInfo(PciInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_PciInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOnErrorList(BlockdevOnErrorList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOnErrorList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MirrorSyncModeList(MirrorSyncModeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MirrorSyncModeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockJobTypeList(BlockJobTypeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockJobTypeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockJobInfoList(BlockJobInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockJobInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockJobInfo(BlockJobInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockJobInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NewImageModeList(NewImageModeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NewImageModeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevSnapshotList(BlockdevSnapshotList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevSnapshotList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevSnapshot(BlockdevSnapshot * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevSnapshot(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevSnapshotInternalList(BlockdevSnapshotInternalList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevSnapshotInternalList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevSnapshotInternal(BlockdevSnapshotInternal * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevSnapshotInternal(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_DriveBackupList(DriveBackupList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_DriveBackupList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_DriveBackup(DriveBackup * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_DriveBackup(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_AbortList(AbortList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_AbortList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_Abort(Abort * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_Abort(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TransactionActionList(TransactionActionList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TransactionActionList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TransactionAction(TransactionAction * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TransactionAction(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ObjectPropertyInfoList(ObjectPropertyInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectPropertyInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ObjectPropertyInfo(ObjectPropertyInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectPropertyInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ObjectTypeInfoList(ObjectTypeInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectTypeInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ObjectTypeInfo(ObjectTypeInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ObjectTypeInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_DevicePropertyInfoList(DevicePropertyInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_DevicePropertyInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_DevicePropertyInfo(DevicePropertyInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_DevicePropertyInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevNoneOptionsList(NetdevNoneOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevNoneOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevNoneOptions(NetdevNoneOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevNoneOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetLegacyNicOptionsList(NetLegacyNicOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacyNicOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetLegacyNicOptions(NetLegacyNicOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacyNicOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_StringList(StringList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_StringList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_String(String * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_String(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevUserOptionsList(NetdevUserOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevUserOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevUserOptions(NetdevUserOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevUserOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevTapOptionsList(NetdevTapOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevTapOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevTapOptions(NetdevTapOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevTapOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevSocketOptionsList(NetdevSocketOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevSocketOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevSocketOptions(NetdevSocketOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevSocketOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevVdeOptionsList(NetdevVdeOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevVdeOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevVdeOptions(NetdevVdeOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevVdeOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevDumpOptionsList(NetdevDumpOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevDumpOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevDumpOptions(NetdevDumpOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevDumpOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevBridgeOptionsList(NetdevBridgeOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevBridgeOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevBridgeOptions(NetdevBridgeOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevBridgeOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevHubPortOptionsList(NetdevHubPortOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevHubPortOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevHubPortOptions(NetdevHubPortOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevHubPortOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevNetmapOptionsList(NetdevNetmapOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevNetmapOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevNetmapOptions(NetdevNetmapOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevNetmapOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetClientOptionsList(NetClientOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetClientOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetClientOptions(NetClientOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetClientOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetLegacyList(NetLegacyList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacyList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetLegacy(NetLegacy * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetLegacy(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_NetdevList(NetdevList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_NetdevList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_Netdev(Netdev * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_Netdev(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_InetSocketAddressList(InetSocketAddressList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_InetSocketAddressList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_InetSocketAddress(InetSocketAddress * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_InetSocketAddress(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_UnixSocketAddressList(UnixSocketAddressList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_UnixSocketAddressList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_UnixSocketAddress(UnixSocketAddress * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_UnixSocketAddress(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SocketAddressList(SocketAddressList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SocketAddressList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_SocketAddress(SocketAddress * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_SocketAddress(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MachineInfoList(MachineInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MachineInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_MachineInfo(MachineInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_MachineInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CpuDefinitionInfoList(CpuDefinitionInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuDefinitionInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CpuDefinitionInfo(CpuDefinitionInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CpuDefinitionInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_AddfdInfoList(AddfdInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_AddfdInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_AddfdInfo(AddfdInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_AddfdInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_FdsetFdInfoList(FdsetFdInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetFdInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_FdsetFdInfo(FdsetFdInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetFdInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_FdsetInfoList(FdsetInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_FdsetInfo(FdsetInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_FdsetInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TargetInfoList(TargetInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TargetInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TargetInfo(TargetInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TargetInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_QKeyCodeList(QKeyCodeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_QKeyCodeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_KeyValueList(KeyValueList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_KeyValueList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_KeyValue(KeyValue * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_KeyValue(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevFileList(ChardevFileList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevFileList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevFile(ChardevFile * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevFile(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevHostdevList(ChardevHostdevList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevHostdevList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevHostdev(ChardevHostdev * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevHostdev(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevSocketList(ChardevSocketList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevSocketList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevSocket(ChardevSocket * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevSocket(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevUdpList(ChardevUdpList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevUdpList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevUdp(ChardevUdp * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevUdp(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevMuxList(ChardevMuxList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevMuxList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevMux(ChardevMux * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevMux(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevStdioList(ChardevStdioList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevStdioList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevStdio(ChardevStdio * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevStdio(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevSpiceChannelList(ChardevSpiceChannelList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevSpiceChannelList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevSpiceChannel(ChardevSpiceChannel * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevSpiceChannel(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevSpicePortList(ChardevSpicePortList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevSpicePortList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevSpicePort(ChardevSpicePort * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevSpicePort(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevVCList(ChardevVCList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevVCList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevVC(ChardevVC * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevVC(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevRingbufList(ChardevRingbufList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevRingbufList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevRingbuf(ChardevRingbuf * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevRingbuf(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevDummyList(ChardevDummyList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevDummyList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevDummy(ChardevDummy * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevDummy(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevBackendList(ChardevBackendList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevBackendList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevBackend(ChardevBackend * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevBackend(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevReturnList(ChardevReturnList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevReturnList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_ChardevReturn(ChardevReturn * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_ChardevReturn(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TpmModelList(TpmModelList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TpmModelList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TpmTypeList(TpmTypeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TpmTypeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TPMPassthroughOptionsList(TPMPassthroughOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TPMPassthroughOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TPMPassthroughOptions(TPMPassthroughOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TPMPassthroughOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TpmTypeOptionsList(TpmTypeOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TpmTypeOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TpmTypeOptions(TpmTypeOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TpmTypeOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TPMInfoList(TPMInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TPMInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_TPMInfo(TPMInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_TPMInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_AcpiTableOptionsList(AcpiTableOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_AcpiTableOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_AcpiTableOptions(AcpiTableOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_AcpiTableOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CommandLineParameterTypeList(CommandLineParameterTypeList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandLineParameterTypeList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CommandLineParameterInfoList(CommandLineParameterInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandLineParameterInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CommandLineParameterInfo(CommandLineParameterInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandLineParameterInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CommandLineOptionInfoList(CommandLineOptionInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandLineOptionInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_CommandLineOptionInfo(CommandLineOptionInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_CommandLineOptionInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_X86CPURegister32List(X86CPURegister32List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_X86CPURegister32List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_X86CPUFeatureWordInfoList(X86CPUFeatureWordInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_X86CPUFeatureWordInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_X86CPUFeatureWordInfo(X86CPUFeatureWordInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_X86CPUFeatureWordInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_RxStateList(RxStateList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_RxStateList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_RxFilterInfoList(RxFilterInfoList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_RxFilterInfoList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_RxFilterInfo(RxFilterInfo * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_RxFilterInfo(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevDiscardOptionsList(BlockdevDiscardOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevDiscardOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevAioOptionsList(BlockdevAioOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevAioOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevCacheOptionsList(BlockdevCacheOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevCacheOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevCacheOptions(BlockdevCacheOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevCacheOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsBaseList(BlockdevOptionsBaseList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsBaseList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsBase(BlockdevOptionsBase * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsBase(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsFileList(BlockdevOptionsFileList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsFileList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsFile(BlockdevOptionsFile * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsFile(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsVVFATList(BlockdevOptionsVVFATList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsVVFATList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsVVFAT(BlockdevOptionsVVFAT * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsVVFAT(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsGenericFormatList(BlockdevOptionsGenericFormatList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsGenericFormatList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsGenericFormat(BlockdevOptionsGenericFormat * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsGenericFormat(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsGenericCOWFormatList(BlockdevOptionsGenericCOWFormatList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsGenericCOWFormatList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsGenericCOWFormat(BlockdevOptionsGenericCOWFormat * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsGenericCOWFormat(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsQcow2List(BlockdevOptionsQcow2List * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsQcow2List(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsQcow2(BlockdevOptionsQcow2 * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsQcow2(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptionsList(BlockdevOptionsList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptionsList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevOptions(BlockdevOptions * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevOptions(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevRefList(BlockdevRefList * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevRefList(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}


void qapi_free_BlockdevRef(BlockdevRef * obj)
{
    QapiDeallocVisitor *md;
    Visitor *v;

    if (!obj) {
        return;
    }

    md = qapi_dealloc_visitor_new();
    v = qapi_dealloc_get_visitor(md);
    visit_type_BlockdevRef(v, &obj, NULL, NULL);
    qapi_dealloc_visitor_cleanup(md);
}

