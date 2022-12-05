/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 * Copyright (c) 2009-2015 Red Hat Inc
 *
 * Authors:
 *  Juan Quintela <quintela@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "hw/boards.h"
#include "hw/xen/xen.h"
#include "net/net.h"
#include "migration.h"
#include "migration/snapshot.h"
#include "migration/misc.h"
#include "migration/register.h"
#include "migration/global_state.h"
#include "ram.h"
#include "qemu-file-channel.h"
#include "qemu-file.h"
#include "savevm.h"
#include "postcopy-ram.h"
#include "qapi/error.h"
#include "qapi/qapi-commands-migration.h"
#include "qapi/qapi-commands-misc.h"
#include "qapi/qmp/qerror.h"
#include "qemu/error-report.h"
#include "qemu/queue.h"
#include "monitor/monitor.h"
#include "block/qapi.h"
#include "sysemu/cpus.h"
#include "exec/memory.h"
#include "exec/target_page.h"
#include "trace.h"
#include "qemu/bitops.h"
#include "qemu/iov.h"
#include "block/snapshot.h"
#include "qemu/cutils.h"
#include "io/channel-buffer.h"
#include "io/channel-file.h"
#include "qapi/qmp/qdict.h"
#include "sysemu/replay.h"

#ifndef ETH_P_RARP
#define ETH_P_RARP 0x8035
#endif
#define ARP_HTYPE_ETH 0x0001
#define ARP_PTYPE_IP 0x0800
#define ARP_OP_REQUEST_REV 0x3

const unsigned int postcopy_ram_discard_version = 0;

/* Subcommands for QEMU_VM_COMMAND */
enum qemu_vm_cmd {
    MIG_CMD_INVALID = 0,   /* Must be 0 */
    MIG_CMD_OPEN_RETURN_PATH,  /* Tell the dest to open the Return path */
    MIG_CMD_PING,              /* Request a PONG on the RP */

    MIG_CMD_POSTCOPY_ADVISE,       /* Prior to any page transfers, just
                                      warn we might want to do PC */
    MIG_CMD_POSTCOPY_LISTEN,       /* Start listening for incoming
                                      pages as it's running. */
    MIG_CMD_POSTCOPY_RUN,          /* Start execution */

    MIG_CMD_POSTCOPY_RAM_DISCARD,  /* A list of pages to discard that
                                      were previously sent during
                                      precopy but are dirty. */
    MIG_CMD_PACKAGED,          /* Send a wrapped stream within this stream */
    MIG_CMD_MAX
};

#define MAX_VM_CMD_PACKAGED_SIZE UINT32_MAX
static struct mig_cmd_args {
    ssize_t     len; /* -1 = variable */
    const char *name;
} mig_cmd_args[] = {
    [MIG_CMD_INVALID]          = { .len = -1, .name = "INVALID" },
    [MIG_CMD_OPEN_RETURN_PATH] = { .len =  0, .name = "OPEN_RETURN_PATH" },
    [MIG_CMD_PING]             = { .len = sizeof(uint32_t), .name = "PING" },
    [MIG_CMD_POSTCOPY_ADVISE]  = { .len = -1, .name = "POSTCOPY_ADVISE" },
    [MIG_CMD_POSTCOPY_LISTEN]  = { .len =  0, .name = "POSTCOPY_LISTEN" },
    [MIG_CMD_POSTCOPY_RUN]     = { .len =  0, .name = "POSTCOPY_RUN" },
    [MIG_CMD_POSTCOPY_RAM_DISCARD] = {
                                   .len = -1, .name = "POSTCOPY_RAM_DISCARD" },
    [MIG_CMD_PACKAGED]         = { .len =  4, .name = "PACKAGED" },
    [MIG_CMD_MAX]              = { .len = -1, .name = "MAX" },
};

/* Note for MIG_CMD_POSTCOPY_ADVISE:
 * The format of arguments is depending on postcopy mode:
 * - postcopy RAM only
 *   uint64_t host page size
 *   uint64_t taget page size
 *
 * - postcopy RAM and postcopy dirty bitmaps
 *   format is the same as for postcopy RAM only
 *
 * - postcopy dirty bitmaps only
 *   Nothing. Command length field is 0.
 *
 * Be careful: adding a new postcopy entity with some other parameters should
 * not break format self-description ability. Good way is to introduce some
 * generic extendable format with an exception for two old entities.
 */

static int announce_self_create(uint8_t *buf,
                                uint8_t *mac_addr)
{
    /* Ethernet header. */
    memset(buf, 0xff, 6);         /* destination MAC addr */
    memcpy(buf + 6, mac_addr, 6); /* source MAC addr */
    *(uint16_t *)(buf + 12) = htons(ETH_P_RARP); /* ethertype */

    /* RARP header. */
    *(uint16_t *)(buf + 14) = htons(ARP_HTYPE_ETH); /* hardware addr space */
    *(uint16_t *)(buf + 16) = htons(ARP_PTYPE_IP); /* protocol addr space */
    *(buf + 18) = 6; /* hardware addr length (ethernet) */
    *(buf + 19) = 4; /* protocol addr length (IPv4) */
    *(uint16_t *)(buf + 20) = htons(ARP_OP_REQUEST_REV); /* opcode */
    memcpy(buf + 22, mac_addr, 6); /* source hw addr */
    memset(buf + 28, 0x00, 4);     /* source protocol addr */
    memcpy(buf + 32, mac_addr, 6); /* target hw addr */
    memset(buf + 38, 0x00, 4);     /* target protocol addr */

    /* Padding to get up to 60 bytes (ethernet min packet size, minus FCS). */
    memset(buf + 42, 0x00, 18);

    return 60; /* len (FCS will be added by hardware) */
}

static void qemu_announce_self_iter(NICState *nic, void *opaque)
{
    uint8_t buf[60];
    int len;

    trace_qemu_announce_self_iter(qemu_ether_ntoa(&nic->conf->macaddr));
    len = announce_self_create(buf, nic->conf->macaddr.a);

    qemu_send_packet_raw(qemu_get_queue(nic), buf, len);
}


static void qemu_announce_self_once(void *opaque)
{
    static int count = SELF_ANNOUNCE_ROUNDS;
    QEMUTimer *timer = *(QEMUTimer **)opaque;

    qemu_foreach_nic(qemu_announce_self_iter, NULL);

    if (--count) {
        /* delay 50ms, 150ms, 250ms, ... */
        timer_mod(timer, qemu_clock_get_ms(QEMU_CLOCK_REALTIME) +
                  self_announce_delay(count));
    } else {
            timer_del(timer);
            timer_free(timer);
    }
}

void qemu_announce_self(void)
{
    static QEMUTimer *timer;
    timer = timer_new_ms(QEMU_CLOCK_REALTIME, qemu_announce_self_once, &timer);
    qemu_announce_self_once(&timer);
}

/***********************************************************/
/* savevm/loadvm support */

static ssize_t block_writev_buffer(void *opaque, struct iovec *iov, int iovcnt,
                                   int64_t pos)
{
    int ret;
    QEMUIOVector qiov;

    qemu_iovec_init_external(&qiov, iov, iovcnt);
    ret = bdrv_writev_vmstate(opaque, &qiov, pos);
    if (ret < 0) {
        return ret;
    }

    return qiov.size;
}

static ssize_t block_get_buffer(void *opaque, uint8_t *buf, int64_t pos,
                                size_t size)
{
    return bdrv_load_vmstate(opaque, buf, pos, size);
}

static int bdrv_fclose(void *opaque)
{
    return bdrv_flush(opaque);
}

static const QEMUFileOps bdrv_read_ops = {
    .get_buffer = block_get_buffer,
    .close =      bdrv_fclose
};

static const QEMUFileOps bdrv_write_ops = {
    .writev_buffer  = block_writev_buffer,
    .close          = bdrv_fclose
};

static QEMUFile *qemu_fopen_bdrv(BlockDriverState *bs, int is_writable)
{
    if (is_writable) {
        return qemu_fopen_ops(bs, &bdrv_write_ops);
    }
    return qemu_fopen_ops(bs, &bdrv_read_ops);
}


/* QEMUFile timer support.
 * Not in qemu-file.c to not add qemu-timer.c as dependency to qemu-file.c
 */

void timer_put(QEMUFile *f, QEMUTimer *ts)
{
    uint64_t expire_time;

    expire_time = timer_expire_time_ns(ts);
    qemu_put_be64(f, expire_time);
}

void timer_get(QEMUFile *f, QEMUTimer *ts)
{
    uint64_t expire_time;

    expire_time = qemu_get_be64(f);
    if (expire_time != -1) {
        timer_mod_ns(ts, expire_time);
    } else {
        timer_del(ts);
    }
}


/* VMState timer support.
 * Not in vmstate.c to not add qemu-timer.c as dependency to vmstate.c
 */

static int get_timer(QEMUFile *f, void *pv, size_t size, VMStateField *field)
{
    QEMUTimer *v = pv;
    timer_get(f, v);
    return 0;
}

static int put_timer(QEMUFile *f, void *pv, size_t size, VMStateField *field,
                     QJSON *vmdesc)
{
    QEMUTimer *v = pv;
    timer_put(f, v);

    return 0;
}

const VMStateInfo vmstate_info_timer = {
    .name = "timer",
    .get  = get_timer,
    .put  = put_timer,
};


typedef struct CompatEntry {
    char idstr[256];
    int instance_id;
} CompatEntry;

typedef struct SaveStateEntry {
    QTAILQ_ENTRY(SaveStateEntry) entry;
    char idstr[256];
    int instance_id;
    int alias_id;
    int version_id;
    /* version id read from the stream */
    int load_version_id;
    int section_id;
    /* section id read from the stream */
    int load_section_id;
    SaveVMHandlers *ops;
    const VMStateDescription *vmsd;
    void *opaque;
    CompatEntry *compat;
    int is_ram;
} SaveStateEntry;

typedef struct SaveState {
    QTAILQ_HEAD(, SaveStateEntry) handlers;
    int global_section_id;
    uint32_t len;
    const char *name;
    uint32_t target_page_bits;
} SaveState;

static SaveState savevm_state = {
    .handlers = QTAILQ_HEAD_INITIALIZER(savevm_state.handlers),
    .global_section_id = 0,
};

static int configuration_pre_save(void *opaque)
{
    SaveState *state = opaque;
    const char *current_name = MACHINE_GET_CLASS(current_machine)->name;

    state->len = strlen(current_name);
    state->name = current_name;
    state->target_page_bits = qemu_target_page_bits();

    return 0;
}

static int configuration_pre_load(void *opaque)
{
    SaveState *state = opaque;

    /* If there is no target-page-bits subsection it means the source
     * predates the variable-target-page-bits support and is using the
     * minimum possible value for this CPU.
     */
    state->target_page_bits = qemu_target_page_bits_min();
    return 0;
}

static int configuration_post_load(void *opaque, int version_id)
{
    SaveState *state = opaque;
    const char *current_name = MACHINE_GET_CLASS(current_machine)->name;

    if (strncmp(state->name, current_name, state->len) != 0) {
        error_report("Machine type received is '%.*s' and local is '%s'",
                     (int) state->len, state->name, current_name);
        return -EINVAL;
    }

    if (state->target_page_bits != qemu_target_page_bits()) {
        error_report("Received TARGET_PAGE_BITS is %d but local is %d",
                     state->target_page_bits, qemu_target_page_bits());
        return -EINVAL;
    }

    return 0;
}

/* The target-page-bits subsection is present only if the
 * target page size is not the same as the default (ie the
 * minimum page size for a variable-page-size guest CPU).
 * If it is present then it contains the actual target page
 * bits for the machine, and migration will fail if the
 * two ends don't agree about it.
 */
static bool vmstate_target_page_bits_needed(void *opaque)
{
    return qemu_target_page_bits()
        > qemu_target_page_bits_min();
}

static const VMStateDescription vmstate_target_page_bits = {
    .name = "configuration/target-page-bits",
    .version_id = 1,
    .minimum_version_id = 1,
    .needed = vmstate_target_page_bits_needed,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(target_page_bits, SaveState),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_configuration = {
    .name = "configuration",
    .version_id = 1,
    .pre_load = configuration_pre_load,
    .post_load = configuration_post_load,
    .pre_save = configuration_pre_save,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(len, SaveState),
        VMSTATE_VBUFFER_ALLOC_UINT32(name, SaveState, 0, NULL, len),
        VMSTATE_END_OF_LIST()
    },
    .subsections = (const VMStateDescription*[]) {
        &vmstate_target_page_bits,
        NULL
    }
};

static void dump_vmstate_vmsd(FILE *out_file,
                              const VMStateDescription *vmsd, int indent,
                              bool is_subsection);

static void dump_vmstate_vmsf(FILE *out_file, const VMStateField *field,
                              int indent)
{
    fprintf(out_file, "%*s{\n", indent, "");
    indent += 2;
    fprintf(out_file, "%*s\"field\": \"%s\",\n", indent, "", field->name);
    fprintf(out_file, "%*s\"version_id\": %d,\n", indent, "",
            field->version_id);
    fprintf(out_file, "%*s\"field_exists\": %s,\n", indent, "",
            field->field_exists ? "true" : "false");
    fprintf(out_file, "%*s\"size\": %zu", indent, "", field->size);
    if (field->vmsd != NULL) {
        fprintf(out_file, ",\n");
        dump_vmstate_vmsd(out_file, field->vmsd, indent, false);
    }
    fprintf(out_file, "\n%*s}", indent - 2, "");
}

static void dump_vmstate_vmss(FILE *out_file,
                              const VMStateDescription **subsection,
                              int indent)
{
    if (*subsection != NULL) {
        dump_vmstate_vmsd(out_file, *subsection, indent, true);
    }
}

static void dump_vmstate_vmsd(FILE *out_file,
                              const VMStateDescription *vmsd, int indent,
                              bool is_subsection)
{
    if (is_subsection) {
        fprintf(out_file, "%*s{\n", indent, "");
    } else {
        fprintf(out_file, "%*s\"%s\": {\n", indent, "", "Description");
    }
    indent += 2;
    fprintf(out_file, "%*s\"name\": \"%s\",\n", indent, "", vmsd->name);
    fprintf(out_file, "%*s\"version_id\": %d,\n", indent, "",
            vmsd->version_id);
    fprintf(out_file, "%*s\"minimum_version_id\": %d", indent, "",
            vmsd->minimum_version_id);
    if (vmsd->fields != NULL) {
        const VMStateField *field = vmsd->fields;
        bool first;

        fprintf(out_file, ",\n%*s\"Fields\": [\n", indent, "");
        first = true;
        while (field->name != NULL) {
            if (field->flags & VMS_MUST_EXIST) {
                /* Ignore VMSTATE_VALIDATE bits; these don't get migrated */
                field++;
                continue;
            }
            if (!first) {
                fprintf(out_file, ",\n");
            }
            dump_vmstate_vmsf(out_file, field, indent + 2);
            field++;
            first = false;
        }
        fprintf(out_file, "\n%*s]", indent, "");
    }
    if (vmsd->subsections != NULL) {
        const VMStateDescription **subsection = vmsd->subsections;
        bool first;

        fprintf(out_file, ",\n%*s\"Subsections\": [\n", indent, "");
        first = true;
        while (*subsection != NULL) {
            if (!first) {
                fprintf(out_file, ",\n");
            }
            dump_vmstate_vmss(out_file, subsection, indent + 2);
            subsection++;
            first = false;
        }
        fprintf(out_file, "\n%*s]", indent, "");
    }
    fprintf(out_file, "\n%*s}", indent - 2, "");
}

static void dump_machine_type(FILE *out_file)
{
    MachineClass *mc;

    mc = MACHINE_GET_CLASS(current_machine);

    fprintf(out_file, "  \"vmschkmachine\": {\n");
    fprintf(out_file, "    \"Name\": \"%s\"\n", mc->name);
    fprintf(out_file, "  },\n");
}

void dump_vmstate_json_to_file(FILE *out_file)
{
    GSList *list, *elt;
    bool first;

    fprintf(out_file, "{\n");
    dump_machine_type(out_file);

    first = true;
    list = object_class_get_list(TYPE_DEVICE, true);
    for (elt = list; elt; elt = elt->next) {
        DeviceClass *dc = OBJECT_CLASS_CHECK(DeviceClass, elt->data,
                                             TYPE_DEVICE);
        const char *name;
        int indent = 2;

        if (!dc->vmsd) {
            continue;
        }

        if (!first) {
            fprintf(out_file, ",\n");
        }
        name = object_class_get_name(OBJECT_CLASS(dc));
        fprintf(out_file, "%*s\"%s\": {\n", indent, "", name);
        indent += 2;
        fprintf(out_file, "%*s\"Name\": \"%s\",\n", indent, "", name);
        fprintf(out_file, "%*s\"version_id\": %d,\n", indent, "",
                dc->vmsd->version_id);
        fprintf(out_file, "%*s\"minimum_version_id\": %d,\n", indent, "",
                dc->vmsd->minimum_version_id);

        dump_vmstate_vmsd(out_file, dc->vmsd, indent, false);

        fprintf(out_file, "\n%*s}", indent - 2, "");
        first = false;
    }
    fprintf(out_file, "\n}\n");
    fclose(out_file);
}

static int calculate_new_instance_id(const char *idstr)
{
    SaveStateEntry *se;
    int instance_id = 0;

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (strcmp(idstr, se->idstr) == 0
            && instance_id <= se->instance_id) {
            instance_id = se->instance_id + 1;
        }
    }
    return instance_id;
}

static int calculate_compat_instance_id(const char *idstr)
{
    SaveStateEntry *se;
    int instance_id = 0;

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (!se->compat) {
            continue;
        }

        if (strcmp(idstr, se->compat->idstr) == 0
            && instance_id <= se->compat->instance_id) {
            instance_id = se->compat->instance_id + 1;
        }
    }
    return instance_id;
}

static inline MigrationPriority save_state_priority(SaveStateEntry *se)
{
    if (se->vmsd) {
        return se->vmsd->priority;
    }
    return MIG_PRI_DEFAULT;
}

static void savevm_state_handler_insert(SaveStateEntry *nse)
{
    trace_savevm_state_handler_insert(nse->idstr, nse->section_id, nse->vmsd ? nse->vmsd->name : "(old)");
    MigrationPriority priority = save_state_priority(nse);
    SaveStateEntry *se;

    assert(priority <= MIG_PRI_MAX);

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (save_state_priority(se) < priority) {
            break;
        }
    }

    if (se) {
        QTAILQ_INSERT_BEFORE(se, nse, entry);
    } else {
        QTAILQ_INSERT_TAIL(&savevm_state.handlers, nse, entry);
    }
}

/* TODO: Individual devices generally have very little idea about the rest
   of the system, so instance_id should be removed/replaced.
   Meanwhile pass -1 as instance_id if you do not already have a clearly
   distinguishing id for all instances of your device class. */
int register_savevm_live(DeviceState *dev,
                         const char *idstr,
                         int instance_id,
                         int version_id,
                         SaveVMHandlers *ops,
                         void *opaque)
{
    SaveStateEntry *se;

    se = g_new0(SaveStateEntry, 1);
    se->version_id = version_id;
    se->section_id = savevm_state.global_section_id++;
    se->ops = ops;
    se->opaque = opaque;
    se->vmsd = NULL;
    /* if this is a live_savem then set is_ram */
    if (ops->save_setup != NULL) {
        se->is_ram = 1;
    }

    if (dev) {
        char *id = qdev_get_dev_path(dev);
        if (id) {
            if (snprintf(se->idstr, sizeof(se->idstr), "%s/", id) >=
                sizeof(se->idstr)) {
                error_report("Path too long for VMState (%s)", id);
                g_free(id);
                g_free(se);

                return -1;
            }
            g_free(id);

            se->compat = g_new0(CompatEntry, 1);
            pstrcpy(se->compat->idstr, sizeof(se->compat->idstr), idstr);
            se->compat->instance_id = instance_id == -1 ?
                         calculate_compat_instance_id(idstr) : instance_id;
            instance_id = -1;
        }
    }
    pstrcat(se->idstr, sizeof(se->idstr), idstr);

    if (instance_id == -1) {
        se->instance_id = calculate_new_instance_id(se->idstr);
    } else {
        se->instance_id = instance_id;
    }
    assert(!se->compat || se->instance_id == 0);
    savevm_state_handler_insert(se);
    return 0;
}

int register_savevm(DeviceState *dev,
                    const char *idstr,
                    int instance_id,
                    int version_id,
                    SaveStateHandler *save_state,
                    LoadStateHandler *load_state,
                    void *opaque)
{
    SaveVMHandlers *ops = g_new0(SaveVMHandlers, 1);
    ops->save_state = save_state;
    ops->load_state = load_state;
    return register_savevm_live(dev, idstr, instance_id, version_id,
                                ops, opaque);
}

int register_savevm_with_post_load(DeviceState *dev,
                                   const char *idstr,
                                   int instance_id,
                                   int version_id,
                                   SaveStateHandler *save_state,
                                   LoadStateHandler *load_state,
                                   PostLoadHandler *post_load,
                                   void *opaque)
{
    SaveVMHandlers *ops = g_new0(SaveVMHandlers, 1);
    ops->save_state = save_state;
    ops->load_state = load_state;
    ops->post_load = post_load;
    return register_savevm_live(dev, idstr, instance_id, version_id,
                                ops, opaque);
}

void unregister_savevm(DeviceState *dev, const char *idstr, void *opaque)
{
    SaveStateEntry *se, *new_se;
    char id[256] = "";

    if (dev) {
        char *path = qdev_get_dev_path(dev);
        if (path) {
            pstrcpy(id, sizeof(id), path);
            pstrcat(id, sizeof(id), "/");
            g_free(path);
        }
    }
    pstrcat(id, sizeof(id), idstr);

    QTAILQ_FOREACH_SAFE(se, &savevm_state.handlers, entry, new_se) {
        if (strcmp(se->idstr, id) == 0 && se->opaque == opaque) {
            QTAILQ_REMOVE(&savevm_state.handlers, se, entry);
            g_free(se->compat);
            g_free(se);
        }
    }
}

int vmstate_register_with_alias_id(DeviceState *dev, int instance_id,
                                   const VMStateDescription *vmsd,
                                   void *opaque, int alias_id,
                                   int required_for_version,
                                   Error **errp)
{
    SaveStateEntry *se;

    /* If this triggers, alias support can be dropped for the vmsd. */
    assert(alias_id == -1 || required_for_version >= vmsd->minimum_version_id);

    se = g_new0(SaveStateEntry, 1);
    se->version_id = vmsd->version_id;
    se->section_id = savevm_state.global_section_id++;
    se->opaque = opaque;
    se->vmsd = vmsd;
    se->alias_id = alias_id;

    if (dev) {
        char *id = qdev_get_dev_path(dev);
        if (id) {
            if (snprintf(se->idstr, sizeof(se->idstr), "%s/", id) >=
                sizeof(se->idstr)) {
                error_setg(errp, "Path too long for VMState (%s)", id);
                g_free(id);
                g_free(se);

                return -1;
            }
            g_free(id);

            se->compat = g_new0(CompatEntry, 1);
            pstrcpy(se->compat->idstr, sizeof(se->compat->idstr), vmsd->name);
            se->compat->instance_id = instance_id == -1 ?
                         calculate_compat_instance_id(vmsd->name) : instance_id;
            instance_id = -1;
        }
    }
    pstrcat(se->idstr, sizeof(se->idstr), vmsd->name);

    if (instance_id == -1) {
        se->instance_id = calculate_new_instance_id(se->idstr);
    } else {
        se->instance_id = instance_id;
    }
    assert(!se->compat || se->instance_id == 0);
    savevm_state_handler_insert(se);
    return 0;
}

void vmstate_unregister(DeviceState *dev, const VMStateDescription *vmsd,
                        void *opaque)
{
    SaveStateEntry *se, *new_se;

    QTAILQ_FOREACH_SAFE(se, &savevm_state.handlers, entry, new_se) {
        if (se->vmsd == vmsd && se->opaque == opaque) {
            QTAILQ_REMOVE(&savevm_state.handlers, se, entry);
            g_free(se->compat);
            g_free(se);
        }
    }
}

static int vmstate_load(QEMUFile *f, SaveStateEntry *se)
{
    int res = 0;
#if SNAPSHOT_PROFILE > 1
    int64_t beginTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#endif

    trace_vmstate_load(se->idstr, se->vmsd ? se->vmsd->name : "(old)");
    if (!se->vmsd) {         /* Old style */
        res = se->ops->load_state(f, se->opaque, se->load_version_id);
    } else {
        res = vmstate_load_state(f, se->vmsd, se->opaque, se->load_version_id);
    }
#if SNAPSHOT_PROFILE > 1
    int64_t endTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("load_state(%s) time %.03f ms\n", se->idstr,
            (endTime - beginTime)/1000000.0);
#endif
    return res;
}

static void vmstate_save_old_style(QEMUFile *f, SaveStateEntry *se, QJSON *vmdesc)
{
    int64_t old_offset, size;

    old_offset = qemu_ftell_fast(f);
    se->ops->save_state(f, se->opaque);
    size = qemu_ftell_fast(f) - old_offset;

    if (vmdesc) {
        json_prop_int(vmdesc, "size", size);
        json_start_array(vmdesc, "fields");
        json_start_object(vmdesc, NULL);
        json_prop_str(vmdesc, "name", "data");
        json_prop_int(vmdesc, "size", size);
        json_prop_str(vmdesc, "type", "buffer");
        json_end_object(vmdesc);
        json_end_array(vmdesc);
    }
}

static int vmstate_save(QEMUFile *f, SaveStateEntry *se, QJSON *vmdesc)
{
    trace_vmstate_save(se->idstr, se->vmsd ? se->vmsd->name : "(old)");
    if (!se->vmsd) {
        vmstate_save_old_style(f, se, vmdesc);
        return 0;
    }
    return vmstate_save_state(f, se->vmsd, se->opaque, vmdesc);
}

/*
 * Write the header for device section (QEMU_VM_SECTION START/END/PART/FULL)
 */
static void save_section_header(QEMUFile *f, SaveStateEntry *se,
                                uint8_t section_type)
{
    qemu_put_byte(f, section_type);
    qemu_put_be32(f, se->section_id);

    if (section_type == QEMU_VM_SECTION_FULL ||
        section_type == QEMU_VM_SECTION_START) {
        /* ID string */
        size_t len = strlen(se->idstr);
        qemu_put_byte(f, len);
        qemu_put_buffer(f, (uint8_t *)se->idstr, len);

        qemu_put_be32(f, se->instance_id);
        qemu_put_be32(f, se->version_id);
    }
}

/*
 * Write a footer onto device sections that catches cases misformatted device
 * sections.
 */
static void save_section_footer(QEMUFile *f, SaveStateEntry *se)
{
    trace_save_section_footer(se->section_id, se->idstr, migrate_get_current()->send_section_footer);
    if (migrate_get_current()->send_section_footer) {
        qemu_put_byte(f, QEMU_VM_SECTION_FOOTER);
        qemu_put_be32(f, se->section_id);
    }
}

/**
 * qemu_savevm_command_send: Send a 'QEMU_VM_COMMAND' type element with the
 *                           command and associated data.
 *
 * @f: File to send command on
 * @command: Command type to send
 * @len: Length of associated data
 * @data: Data associated with command.
 */
static void qemu_savevm_command_send(QEMUFile *f,
                                     enum qemu_vm_cmd command,
                                     uint16_t len,
                                     uint8_t *data)
{
    trace_savevm_command_send(command, len);
    qemu_put_byte(f, QEMU_VM_COMMAND);
    qemu_put_be16(f, (uint16_t)command);
    qemu_put_be16(f, len);
    qemu_put_buffer(f, data, len);
    qemu_fflush(f);
}

void qemu_savevm_send_ping(QEMUFile *f, uint32_t value)
{
    uint32_t buf;

    trace_savevm_send_ping(value);
    buf = cpu_to_be32(value);
    qemu_savevm_command_send(f, MIG_CMD_PING, sizeof(value), (uint8_t *)&buf);
}

void qemu_savevm_send_open_return_path(QEMUFile *f)
{
    trace_savevm_send_open_return_path();
    qemu_savevm_command_send(f, MIG_CMD_OPEN_RETURN_PATH, 0, NULL);
}

/* We have a buffer of data to send; we don't want that all to be loaded
 * by the command itself, so the command contains just the length of the
 * extra buffer that we then send straight after it.
 * TODO: Must be a better way to organise that
 *
 * Returns:
 *    0 on success
 *    -ve on error
 */
int qemu_savevm_send_packaged(QEMUFile *f, const uint8_t *buf, size_t len)
{
    uint32_t tmp;

    if (len > MAX_VM_CMD_PACKAGED_SIZE) {
        error_report("%s: Unreasonably large packaged state: %zu",
                     __func__, len);
        return -1;
    }

    tmp = cpu_to_be32(len);

    trace_qemu_savevm_send_packaged();
    qemu_savevm_command_send(f, MIG_CMD_PACKAGED, 4, (uint8_t *)&tmp);

    qemu_put_buffer(f, buf, len);

    return 0;
}

/* Send prior to any postcopy transfer */
void qemu_savevm_send_postcopy_advise(QEMUFile *f)
{
    if (migrate_postcopy_ram()) {
        uint64_t tmp[2];
        tmp[0] = cpu_to_be64(ram_pagesize_summary());
        tmp[1] = cpu_to_be64(qemu_target_page_size());

        trace_qemu_savevm_send_postcopy_advise();
        qemu_savevm_command_send(f, MIG_CMD_POSTCOPY_ADVISE,
                                 16, (uint8_t *)tmp);
    } else {
        qemu_savevm_command_send(f, MIG_CMD_POSTCOPY_ADVISE, 0, NULL);
    }
}

/* Sent prior to starting the destination running in postcopy, discard pages
 * that have already been sent but redirtied on the source.
 * CMD_POSTCOPY_RAM_DISCARD consist of:
 *      byte   version (0)
 *      byte   Length of name field (not including 0)
 *  n x byte   RAM block name
 *      byte   0 terminator (just for safety)
 *  n x        Byte ranges within the named RAMBlock
 *      be64   Start of the range
 *      be64   Length
 *
 *  name:  RAMBlock name that these entries are part of
 *  len: Number of page entries
 *  start_list: 'len' addresses
 *  length_list: 'len' addresses
 *
 */
void qemu_savevm_send_postcopy_ram_discard(QEMUFile *f, const char *name,
                                           uint16_t len,
                                           uint64_t *start_list,
                                           uint64_t *length_list)
{
    uint8_t *buf;
    uint16_t tmplen;
    uint16_t t;
    size_t name_len = strlen(name);

    trace_qemu_savevm_send_postcopy_ram_discard(name, len);
    assert(name_len < 256);
    buf = g_malloc0(1 + 1 + name_len + 1 + (8 + 8) * len);
    buf[0] = postcopy_ram_discard_version;
    buf[1] = name_len;
    memcpy(buf + 2, name, name_len);
    tmplen = 2 + name_len;
    buf[tmplen++] = '\0';

    for (t = 0; t < len; t++) {
        stq_be_p(buf + tmplen, start_list[t]);
        tmplen += 8;
        stq_be_p(buf + tmplen, length_list[t]);
        tmplen += 8;
    }
    qemu_savevm_command_send(f, MIG_CMD_POSTCOPY_RAM_DISCARD, tmplen, buf);
    g_free(buf);
}

/* Get the destination into a state where it can receive postcopy data. */
void qemu_savevm_send_postcopy_listen(QEMUFile *f)
{
    trace_savevm_send_postcopy_listen();
    qemu_savevm_command_send(f, MIG_CMD_POSTCOPY_LISTEN, 0, NULL);
}

/* Kick the destination into running */
void qemu_savevm_send_postcopy_run(QEMUFile *f)
{
    trace_savevm_send_postcopy_run();
    qemu_savevm_command_send(f, MIG_CMD_POSTCOPY_RUN, 0, NULL);
}

bool qemu_savevm_state_blocked(Error **errp)
{
    SaveStateEntry *se;

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (se->vmsd && se->vmsd->unmigratable) {
            error_setg(errp, "State blocked by non-migratable device '%s'",
                       se->idstr);
            return true;
        }
    }
    return false;
}

void qemu_savevm_state_header(QEMUFile *f)
{
    trace_savevm_state_header();
    qemu_put_be32(f, QEMU_VM_FILE_MAGIC);
    qemu_put_be32(f, QEMU_VM_FILE_VERSION);

    if (migrate_get_current()->send_configuration) {
        qemu_put_byte(f, QEMU_VM_CONFIGURATION);
        vmstate_save_state(f, &vmstate_configuration, &savevm_state, 0);
    }
}

void qemu_savevm_state_setup(QEMUFile *f)
{
    SaveStateEntry *se;
    int ret;

    trace_savevm_state_setup();
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (!se->ops || !se->ops->save_setup) {
            continue;
        }
        if (se->ops && se->ops->is_active) {
            if (!se->ops->is_active(se->opaque)) {
                continue;
            }
        }
        save_section_header(f, se, QEMU_VM_SECTION_START);

        ret = se->ops->save_setup(f, se->opaque);
        save_section_footer(f, se);
        if (ret < 0) {
            qemu_file_set_error(f, ret);
            break;
        }
    }
}

static QEMUSnapshotCallbacks s_snapshot_callbacks = {};

/*
 * this function has three return values:
 *   negative: there was one error, and we have -errno.
 *   0 : We haven't finished, caller have to go again
 *   1 : We have finished, we can go to complete phase
 */
int qemu_savevm_state_iterate(QEMUFile *f, bool postcopy)
{
    SaveStateEntry *se;
    int ret = 1;

    trace_savevm_state_iterate();
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (s_snapshot_callbacks.savevm.is_canceled) {
            if (s_snapshot_callbacks.savevm.is_canceled(0)) {
                return 0;
            }
        }
        if (!se->ops || !se->ops->save_live_iterate) {
            continue;
        }
        if (se->ops && se->ops->is_active) {
            if (!se->ops->is_active(se->opaque)) {
                continue;
            }
        }
        if (se->ops && se->ops->is_active_iterate) {
            if (!se->ops->is_active_iterate(se->opaque)) {
                continue;
            }
        }
        /*
         * In the postcopy phase, any device that doesn't know how to
         * do postcopy should have saved it's state in the _complete
         * call that's already run, it might get confused if we call
         * iterate afterwards.
         */
        if (postcopy &&
            !(se->ops->has_postcopy && se->ops->has_postcopy(se->opaque))) {
            continue;
        }
        if (qemu_file_rate_limit(f)) {
            return 0;
        }
        trace_savevm_section_start(se->idstr, se->section_id);

#if SNAPSHOT_PROFILE > 1
        int64_t beginTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#endif

        save_section_header(f, se, QEMU_VM_SECTION_PART);

        ret = se->ops->save_live_iterate(f, se->opaque);
        trace_savevm_section_end(se->idstr, se->section_id, ret);
        save_section_footer(f, se);

#if SNAPSHOT_PROFILE > 1
        int64_t endTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
        printf("savevm_state_iterate(%s) time %.03f ms\n", se->idstr,
                (endTime - beginTime)/1000000.0);
#endif

        if (ret < 0) {
            qemu_file_set_error(f, ret);
        }
        if (ret <= 0) {
            /* Do not proceed to the next vmstate before this one reported
               completion of the current stage. This serializes the migration
               and reduces the probability that a faster changing state is
               synchronized over and over again. */
            break;
        }
    }
    return ret;
}

static bool should_send_vmdesc(void)
{
    MachineState *machine = MACHINE(qdev_get_machine());
    bool in_postcopy = migration_in_postcopy();
    return !machine->suppress_vmdesc && !in_postcopy;
}

/*
 * Calls the save_live_complete_postcopy methods
 * causing the last few pages to be sent immediately and doing any associated
 * cleanup.
 * Note postcopy also calls qemu_savevm_state_complete_precopy to complete
 * all the other devices, but that happens at the point we switch to postcopy.
 */
void qemu_savevm_state_complete_postcopy(QEMUFile *f)
{
    SaveStateEntry *se;
    int ret;

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (!se->ops || !se->ops->save_live_complete_postcopy) {
            continue;
        }
        if (se->ops && se->ops->is_active) {
            if (!se->ops->is_active(se->opaque)) {
                continue;
            }
        }
        trace_savevm_section_start(se->idstr, se->section_id);
        /* Section type */
        qemu_put_byte(f, QEMU_VM_SECTION_END);
        qemu_put_be32(f, se->section_id);

        ret = se->ops->save_live_complete_postcopy(f, se->opaque);
        trace_savevm_section_end(se->idstr, se->section_id, ret);
        save_section_footer(f, se);
        if (ret < 0) {
            qemu_file_set_error(f, ret);
            return;
        }
    }

    qemu_put_byte(f, QEMU_VM_EOF);
    qemu_fflush(f);
}

int qemu_savevm_state_complete_precopy(QEMUFile *f, bool iterable_only,
                                       bool inactivate_disks)
{
    QJSON *vmdesc;
    int vmdesc_len;
    SaveStateEntry *se;
    int ret;
    bool in_postcopy = migration_in_postcopy();

    trace_savevm_state_complete_precopy();

    cpu_synchronize_all_states();

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (!se->ops ||
            (in_postcopy && se->ops->has_postcopy &&
             se->ops->has_postcopy(se->opaque)) ||
            (in_postcopy && !iterable_only) ||
            !se->ops->save_live_complete_precopy) {
            continue;
        }

        if (se->ops && se->ops->is_active) {
            if (!se->ops->is_active(se->opaque)) {
                continue;
            }
        }
        trace_savevm_section_start(se->idstr, se->section_id);

#if SNAPSHOT_PROFILE > 1
        int64_t beginTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#endif

        save_section_header(f, se, QEMU_VM_SECTION_END);

        ret = se->ops->save_live_complete_precopy(f, se->opaque);
        trace_savevm_section_end(se->idstr, se->section_id, ret);
        save_section_footer(f, se);

#if SNAPSHOT_PROFILE > 1
        int64_t endTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
        printf("savevm_complete_precopy(%s) time %.03f ms\n", se->idstr,
                (endTime - beginTime)/1000000.0);
#endif

        if (ret < 0) {
            qemu_file_set_error(f, ret);
            return -1;
        }
    }

    if (iterable_only) {
        return 0;
    }

    vmdesc = qjson_new();
    json_prop_int(vmdesc, "page_size", qemu_target_page_size());
    json_start_array(vmdesc, "devices");
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {

        if ((!se->ops || !se->ops->save_state) && !se->vmsd) {
            continue;
        }
        if (se->vmsd && !vmstate_save_needed(se->vmsd, se->opaque)) {
            trace_savevm_section_skip(se->idstr, se->section_id);
            continue;
        }

        trace_savevm_section_start(se->idstr, se->section_id);

#if SNAPSHOT_PROFILE > 1
        int64_t beginTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#endif

        json_start_object(vmdesc, NULL);
        json_prop_str(vmdesc, "name", se->idstr);
        json_prop_int(vmdesc, "instance_id", se->instance_id);

        save_section_header(f, se, QEMU_VM_SECTION_FULL);
        ret = vmstate_save(f, se, vmdesc);
        if (ret) {
            qemu_file_set_error(f, ret);
            return ret;
        }
        trace_savevm_section_end(se->idstr, se->section_id, 0);
        save_section_footer(f, se);

#if SNAPSHOT_PROFILE > 1
        int64_t endTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
        printf("vmstate_save, old style (%s) time %.03f ms\n", se->idstr,
                (endTime - beginTime)/1000000.0);
#endif

        json_end_object(vmdesc);
    }

    if (inactivate_disks) {
        /* Inactivate before sending QEMU_VM_EOF so that the
         * bdrv_invalidate_cache_all() on the other end won't fail. */
        ret = bdrv_inactivate_all();
        if (ret) {
            error_report("%s: bdrv_inactivate_all() failed (%d)",
                         __func__, ret);
            qemu_file_set_error(f, ret);
            return ret;
        }
    }
    if (!in_postcopy) {
        /* Postcopy stream will still be going */
        qemu_put_byte(f, QEMU_VM_EOF);
    }

    json_end_array(vmdesc);
    qjson_finish(vmdesc);
    vmdesc_len = strlen(qjson_get_str(vmdesc));

    if (should_send_vmdesc()) {
        qemu_put_byte(f, QEMU_VM_VMDESCRIPTION);
        qemu_put_be32(f, vmdesc_len);
        qemu_put_buffer(f, (uint8_t *)qjson_get_str(vmdesc), vmdesc_len);
    }
    qjson_destroy(vmdesc);

    qemu_fflush(f);
    return 0;
}

/* Give an estimate of the amount left to be transferred,
 * the result is split into the amount for units that can and
 * for units that can't do postcopy.
 */
void qemu_savevm_state_pending(QEMUFile *f, uint64_t threshold_size,
                               uint64_t *res_precopy_only,
                               uint64_t *res_compatible,
                               uint64_t *res_postcopy_only)
{
    SaveStateEntry *se;

    *res_precopy_only = 0;
    *res_compatible = 0;
    *res_postcopy_only = 0;


    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (!se->ops || !se->ops->save_live_pending) {
            continue;
        }
        if (se->ops && se->ops->is_active) {
            if (!se->ops->is_active(se->opaque)) {
                continue;
            }
        }
        se->ops->save_live_pending(f, se->opaque, threshold_size,
                                   res_precopy_only, res_compatible,
                                   res_postcopy_only);
    }
}

void qemu_savevm_state_cleanup(void)
{
    SaveStateEntry *se;

    trace_savevm_state_cleanup();
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (se->ops && se->ops->save_cleanup) {
            se->ops->save_cleanup(se->opaque);
        }
    }
}

static const QEMUFileHooks* sSaveFileHooks = NULL;
static const QEMUFileHooks* sLoadFileHooks = NULL;

void migrate_set_file_hooks(const QEMUFileHooks* save_hooks,
                            const QEMUFileHooks* load_hooks)
{
    sSaveFileHooks = save_hooks;
    sLoadFileHooks = load_hooks;
}

static void *s_protobuf = NULL;

void qemu_set_snapshot_protobuf(void *pb) { s_protobuf = pb; }

static int qemu_savevm_state(QEMUFile *f, Error **errp)
{
    int ret;
    MigrationState *ms = migrate_get_current();
    MigrationStatus status;

    migrate_init(ms);

    ms->to_dst_file = f;

    if (ms->enabled_capabilities[MIGRATION_CAPABILITY_COMPRESS]) {
        compress_threads_save_setup();
    }

    if (migration_is_blocked(errp)) {
        ret = -EINVAL;
        goto done;
    }

    if (migrate_use_block()) {
        error_setg(errp, "Block migration and snapshots are incompatible");
        ret = -EINVAL;
        goto done;
    }

    qemu_mutex_unlock_iothread();
    qemu_savevm_state_header(f);
    qemu_savevm_state_setup(f);
    qemu_mutex_lock_iothread();

    while (qemu_file_get_error(f) == 0) {
        if (qemu_savevm_state_iterate(f, false) > 0) {
            break;
        }
    }

    ret = qemu_file_get_error(f);
    if (ret == 0) {
        qemu_savevm_state_complete_precopy(f, false, false);
        ret = qemu_file_get_error(f);
    }
    qemu_savevm_state_cleanup();
    if (ret != 0) {
        error_setg_errno(errp, -ret, "Error while writing VM state");
    }

done:
    if (ret != 0) {
        status = MIGRATION_STATUS_FAILED;
    } else {
        status = MIGRATION_STATUS_COMPLETED;
    }
    migrate_set_state(&ms->state, MIGRATION_STATUS_SETUP, status);
    if (ms->enabled_capabilities[MIGRATION_CAPABILITY_COMPRESS]) {
        compress_threads_load_cleanup();
    }

    /* f is outer parameter, it should not stay in global migration state after
     * this function finished */
    ms->to_dst_file = NULL;
    return ret;
}

static int qemu_save_device_state(QEMUFile *f)
{
    SaveStateEntry *se;

    qemu_put_be32(f, QEMU_VM_FILE_MAGIC);
    qemu_put_be32(f, QEMU_VM_FILE_VERSION);

    cpu_synchronize_all_states();

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        int ret;

        if (se->is_ram) {
            continue;
        }
        if ((!se->ops || !se->ops->save_state) && !se->vmsd) {
            continue;
        }
        if (se->vmsd && !vmstate_save_needed(se->vmsd, se->opaque)) {
            continue;
        }

        save_section_header(f, se, QEMU_VM_SECTION_FULL);

        ret = vmstate_save(f, se, NULL);
        if (ret) {
            return ret;
        }

        save_section_footer(f, se);
    }

    qemu_put_byte(f, QEMU_VM_EOF);

    return qemu_file_get_error(f);
}

static SaveStateEntry *find_se(const char *idstr, int instance_id)
{
    SaveStateEntry *se;

    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (!strcmp(se->idstr, idstr) &&
            (instance_id == se->instance_id ||
             instance_id == se->alias_id))
            return se;
        /* Migrating from an older version? */
        if (strstr(se->idstr, idstr) && se->compat) {
            if (!strcmp(se->compat->idstr, idstr) &&
                (instance_id == se->compat->instance_id ||
                 instance_id == se->alias_id))
                return se;
        }
    }
    return NULL;
}

enum LoadVMExitCodes {
    /* Allow a command to quit all layers of nested loadvm loops */
    LOADVM_QUIT     =  1,
};

static int qemu_loadvm_state_main(QEMUFile *f, MigrationIncomingState *mis);

/* ------ incoming postcopy messages ------ */
/* 'advise' arrives before any transfers just to tell us that a postcopy
 * *might* happen - it might be skipped if precopy transferred everything
 * quickly.
 */
static int loadvm_postcopy_handle_advise(MigrationIncomingState *mis,
                                         uint16_t len)
{
    PostcopyState ps = postcopy_state_set(POSTCOPY_INCOMING_ADVISE);
    uint64_t remote_pagesize_summary, local_pagesize_summary, remote_tps;
    Error *local_err = NULL;

    trace_loadvm_postcopy_handle_advise();
    if (ps != POSTCOPY_INCOMING_NONE) {
        error_report("CMD_POSTCOPY_ADVISE in wrong postcopy state (%d)", ps);
        return -1;
    }

    switch (len) {
    case 0:
        if (migrate_postcopy_ram()) {
            error_report("RAM postcopy is enabled but have 0 byte advise");
            return -EINVAL;
        }
        return 0;
    case 8 + 8:
        if (!migrate_postcopy_ram()) {
            error_report("RAM postcopy is disabled but have 16 byte advise");
            return -EINVAL;
        }
        break;
    default:
        error_report("CMD_POSTCOPY_ADVISE invalid length (%d)", len);
        return -EINVAL;
    }

    if (!postcopy_ram_supported_by_host(mis)) {
        postcopy_state_set(POSTCOPY_INCOMING_NONE);
        return -1;
    }

    remote_pagesize_summary = qemu_get_be64(mis->from_src_file);
    local_pagesize_summary = ram_pagesize_summary();

    if (remote_pagesize_summary != local_pagesize_summary)  {
        /*
         * This detects two potential causes of mismatch:
         *   a) A mismatch in host page sizes
         *      Some combinations of mismatch are probably possible but it gets
         *      a bit more complicated.  In particular we need to place whole
         *      host pages on the dest at once, and we need to ensure that we
         *      handle dirtying to make sure we never end up sending part of
         *      a hostpage on it's own.
         *   b) The use of different huge page sizes on source/destination
         *      a more fine grain test is performed during RAM block migration
         *      but this test here causes a nice early clear failure, and
         *      also fails when passed to an older qemu that doesn't
         *      do huge pages.
         */
        error_report("Postcopy needs matching RAM page sizes (s=%" PRIx64
                                                             " d=%" PRIx64 ")",
                     remote_pagesize_summary, local_pagesize_summary);
        return -1;
    }

    remote_tps = qemu_get_be64(mis->from_src_file);
    if (remote_tps != qemu_target_page_size()) {
        /*
         * Again, some differences could be dealt with, but for now keep it
         * simple.
         */
        error_report("Postcopy needs matching target page sizes (s=%d d=%zd)",
                     (int)remote_tps, qemu_target_page_size());
        return -1;
    }

    if (postcopy_notify(POSTCOPY_NOTIFY_INBOUND_ADVISE, &local_err)) {
        error_report_err(local_err);
        return -1;
    }

    if (ram_postcopy_incoming_init(mis)) {
        return -1;
    }

    postcopy_state_set(POSTCOPY_INCOMING_ADVISE);

    return 0;
}

/* After postcopy we will be told to throw some pages away since they're
 * dirty and will have to be demand fetched.  Must happen before CPU is
 * started.
 * There can be 0..many of these messages, each encoding multiple pages.
 */
static int loadvm_postcopy_ram_handle_discard(MigrationIncomingState *mis,
                                              uint16_t len)
{
    int tmp;
    char ramid[256];
    PostcopyState ps = postcopy_state_get();

    trace_loadvm_postcopy_ram_handle_discard();

    switch (ps) {
    case POSTCOPY_INCOMING_ADVISE:
        /* 1st discard */
        tmp = postcopy_ram_prepare_discard(mis);
        if (tmp) {
            return tmp;
        }
        break;

    case POSTCOPY_INCOMING_DISCARD:
        /* Expected state */
        break;

    default:
        error_report("CMD_POSTCOPY_RAM_DISCARD in wrong postcopy state (%d)",
                     ps);
        return -1;
    }
    /* We're expecting a
     *    Version (0)
     *    a RAM ID string (length byte, name, 0 term)
     *    then at least 1 16 byte chunk
    */
    if (len < (1 + 1 + 1 + 1 + 2 * 8)) {
        error_report("CMD_POSTCOPY_RAM_DISCARD invalid length (%d)", len);
        return -1;
    }

    tmp = qemu_get_byte(mis->from_src_file);
    if (tmp != postcopy_ram_discard_version) {
        error_report("CMD_POSTCOPY_RAM_DISCARD invalid version (%d)", tmp);
        return -1;
    }

    if (!qemu_get_counted_string(mis->from_src_file, ramid)) {
        error_report("CMD_POSTCOPY_RAM_DISCARD Failed to read RAMBlock ID");
        return -1;
    }
    tmp = qemu_get_byte(mis->from_src_file);
    if (tmp != 0) {
        error_report("CMD_POSTCOPY_RAM_DISCARD missing nil (%d)", tmp);
        return -1;
    }

    len -= 3 + strlen(ramid);
    if (len % 16) {
        error_report("CMD_POSTCOPY_RAM_DISCARD invalid length (%d)", len);
        return -1;
    }
    trace_loadvm_postcopy_ram_handle_discard_header(ramid, len);
    while (len) {
        uint64_t start_addr, block_length;
        start_addr = qemu_get_be64(mis->from_src_file);
        block_length = qemu_get_be64(mis->from_src_file);

        len -= 16;
        int ret = ram_discard_range(ramid, start_addr, block_length);
        if (ret) {
            return ret;
        }
    }
    trace_loadvm_postcopy_ram_handle_discard_end();

    return 0;
}

/*
 * Triggered by a postcopy_listen command; this thread takes over reading
 * the input stream, leaving the main thread free to carry on loading the rest
 * of the device state (from RAM).
 * (TODO:This could do with being in a postcopy file - but there again it's
 * just another input loop, not that postcopy specific)
 */
static void *postcopy_ram_listen_thread(void *opaque)
{
    QEMUFile *f = opaque;
    MigrationIncomingState *mis = migration_incoming_get_current();
    int load_res;

    migrate_set_state(&mis->state, MIGRATION_STATUS_ACTIVE,
                                   MIGRATION_STATUS_POSTCOPY_ACTIVE);
    qemu_sem_post(&mis->listen_thread_sem);
    trace_postcopy_ram_listen_thread_start();

    /*
     * Because we're a thread and not a coroutine we can't yield
     * in qemu_file, and thus we must be blocking now.
     */
    qemu_file_set_blocking(f, true);
    load_res = qemu_loadvm_state_main(f, mis);
    /* And non-blocking again so we don't block in any cleanup */
    qemu_file_set_blocking(f, false);

    trace_postcopy_ram_listen_thread_exit();
    if (load_res < 0) {
        error_report("%s: loadvm failed: %d", __func__, load_res);
        qemu_file_set_error(f, load_res);
        migrate_set_state(&mis->state, MIGRATION_STATUS_POSTCOPY_ACTIVE,
                                       MIGRATION_STATUS_FAILED);
    } else {
        /*
         * This looks good, but it's possible that the device loading in the
         * main thread hasn't finished yet, and so we might not be in 'RUN'
         * state yet; wait for the end of the main thread.
         */
        qemu_event_wait(&mis->main_thread_load_event);
    }
    postcopy_ram_incoming_cleanup(mis);

    if (load_res < 0) {
        /*
         * If something went wrong then we have a bad state so exit;
         * depending how far we got it might be possible at this point
         * to leave the guest running and fire MCEs for pages that never
         * arrived as a desperate recovery step.
         */
        exit(EXIT_FAILURE);
    }

    migrate_set_state(&mis->state, MIGRATION_STATUS_POSTCOPY_ACTIVE,
                                   MIGRATION_STATUS_COMPLETED);
    /*
     * If everything has worked fine, then the main thread has waited
     * for us to start, and we're the last use of the mis.
     * (If something broke then qemu will have to exit anyway since it's
     * got a bad migration state).
     */
    migration_incoming_state_destroy();
    qemu_loadvm_state_cleanup();

    return NULL;
}

/* After this message we must be able to immediately receive postcopy data */
static int loadvm_postcopy_handle_listen(MigrationIncomingState *mis)
{
    PostcopyState ps = postcopy_state_set(POSTCOPY_INCOMING_LISTENING);
    trace_loadvm_postcopy_handle_listen();
    Error *local_err = NULL;

    if (ps != POSTCOPY_INCOMING_ADVISE && ps != POSTCOPY_INCOMING_DISCARD) {
        error_report("CMD_POSTCOPY_LISTEN in wrong postcopy state (%d)", ps);
        return -1;
    }
    if (ps == POSTCOPY_INCOMING_ADVISE) {
        /*
         * A rare case, we entered listen without having to do any discards,
         * so do the setup that's normally done at the time of the 1st discard.
         */
        if (migrate_postcopy_ram()) {
            postcopy_ram_prepare_discard(mis);
        }
    }

    /*
     * Sensitise RAM - can now generate requests for blocks that don't exist
     * However, at this point the CPU shouldn't be running, and the IO
     * shouldn't be doing anything yet so don't actually expect requests
     */
    if (migrate_postcopy_ram()) {
        if (postcopy_ram_enable_notify(mis)) {
            return -1;
        }
    }

    if (postcopy_notify(POSTCOPY_NOTIFY_INBOUND_LISTEN, &local_err)) {
        error_report_err(local_err);
        return -1;
    }

    if (mis->have_listen_thread) {
        error_report("CMD_POSTCOPY_RAM_LISTEN already has a listen thread");
        return -1;
    }

    mis->have_listen_thread = true;
    /* Start up the listening thread and wait for it to signal ready */
    qemu_sem_init(&mis->listen_thread_sem, 0);
    qemu_thread_create(&mis->listen_thread, "postcopy/listen",
                       postcopy_ram_listen_thread, mis->from_src_file,
                       QEMU_THREAD_DETACHED);
    qemu_sem_wait(&mis->listen_thread_sem);
    qemu_sem_destroy(&mis->listen_thread_sem);

    return 0;
}


typedef struct {
    QEMUBH *bh;
} HandleRunBhData;

static void loadvm_postcopy_handle_run_bh(void *opaque)
{
    Error *local_err = NULL;
    HandleRunBhData *data = opaque;

    /* TODO we should move all of this lot into postcopy_ram.c or a shared code
     * in migration.c
     */
    cpu_synchronize_all_post_init();

    qemu_announce_self();

    /* Make sure all file formats flush their mutable metadata.
     * If we get an error here, just don't restart the VM yet. */
    bdrv_invalidate_cache_all(&local_err);
    if (local_err) {
        error_report_err(local_err);
        local_err = NULL;
        autostart = false;
    }

    trace_loadvm_postcopy_handle_run_cpu_sync();
    cpu_synchronize_all_post_init();

    trace_loadvm_postcopy_handle_run_vmstart();

    dirty_bitmap_mig_before_vm_start();

    if (autostart) {
        /* Hold onto your hats, starting the CPU */
        vm_start();
    } else {
        /* leave it paused and let management decide when to start the CPU */
        runstate_set(RUN_STATE_PAUSED);
    }

    qemu_bh_delete(data->bh);
    g_free(data);
}

/* After all discards we can start running and asking for pages */
static int loadvm_postcopy_handle_run(MigrationIncomingState *mis)
{
    PostcopyState ps = postcopy_state_set(POSTCOPY_INCOMING_RUNNING);
    HandleRunBhData *data;

    trace_loadvm_postcopy_handle_run();
    if (ps != POSTCOPY_INCOMING_LISTENING) {
        error_report("CMD_POSTCOPY_RUN in wrong postcopy state (%d)", ps);
        return -1;
    }

    data = g_new(HandleRunBhData, 1);
    data->bh = qemu_bh_new(loadvm_postcopy_handle_run_bh, data);
    qemu_bh_schedule(data->bh);

    /* We need to finish reading the stream from the package
     * and also stop reading anything more from the stream that loaded the
     * package (since it's now being read by the listener thread).
     * LOADVM_QUIT will quit all the layers of nested loadvm loops.
     */
    return LOADVM_QUIT;
}

/**
 * Immediately following this command is a blob of data containing an embedded
 * chunk of migration stream; read it and load it.
 *
 * @mis: Incoming state
 * @length: Length of packaged data to read
 *
 * Returns: Negative values on error
 *
 */
static int loadvm_handle_cmd_packaged(MigrationIncomingState *mis)
{
    int ret;
    size_t length;
    QIOChannelBuffer *bioc;

    length = qemu_get_be32(mis->from_src_file);
    trace_loadvm_handle_cmd_packaged(length);

    if (length > MAX_VM_CMD_PACKAGED_SIZE) {
        error_report("Unreasonably large packaged state: %zu", length);
        return -1;
    }

    bioc = qio_channel_buffer_new(length);
    qio_channel_set_name(QIO_CHANNEL(bioc), "migration-loadvm-buffer");
    ret = qemu_get_buffer(mis->from_src_file,
                          bioc->data,
                          length);
    if (ret != length) {
        object_unref(OBJECT(bioc));
        error_report("CMD_PACKAGED: Buffer receive fail ret=%d length=%zu",
                     ret, length);
        return (ret < 0) ? ret : -EAGAIN;
    }
    bioc->usage += length;
    trace_loadvm_handle_cmd_packaged_received(ret);

    QEMUFile *packf = qemu_fopen_channel_input(QIO_CHANNEL(bioc));

    ret = qemu_loadvm_state_main(packf, mis);
    trace_loadvm_handle_cmd_packaged_main(ret);
    qemu_fclose(packf);
    object_unref(OBJECT(bioc));

    return ret;
}

/*
 * Process an incoming 'QEMU_VM_COMMAND'
 * 0           just a normal return
 * LOADVM_QUIT All good, but exit the loop
 * <0          Error
 */
static int loadvm_process_command(QEMUFile *f)
{
    MigrationIncomingState *mis = migration_incoming_get_current();
    uint16_t cmd;
    uint16_t len;
    uint32_t tmp32;

    cmd = qemu_get_be16(f);
    len = qemu_get_be16(f);

    /* Check validity before continue processing of cmds */
    if (qemu_file_get_error(f)) {
        return qemu_file_get_error(f);
    }

    trace_loadvm_process_command(cmd, len);
    if (cmd >= MIG_CMD_MAX || cmd == MIG_CMD_INVALID) {
        error_report("MIG_CMD 0x%x unknown (len 0x%x)", cmd, len);
        return -EINVAL;
    }

    if (mig_cmd_args[cmd].len != -1 && mig_cmd_args[cmd].len != len) {
        error_report("%s received with bad length - expecting %zu, got %d",
                     mig_cmd_args[cmd].name,
                     (size_t)mig_cmd_args[cmd].len, len);
        return -ERANGE;
    }

    switch (cmd) {
    case MIG_CMD_OPEN_RETURN_PATH:
        if (mis->to_src_file) {
            error_report("CMD_OPEN_RETURN_PATH called when RP already open");
            /* Not really a problem, so don't give up */
            return 0;
        }
        mis->to_src_file = qemu_file_get_return_path(f);
        if (!mis->to_src_file) {
            error_report("CMD_OPEN_RETURN_PATH failed");
            return -1;
        }
        break;

    case MIG_CMD_PING:
        tmp32 = qemu_get_be32(f);
        trace_loadvm_process_command_ping(tmp32);
        if (!mis->to_src_file) {
            error_report("CMD_PING (0x%x) received with no return path",
                         tmp32);
            return -1;
        }
        migrate_send_rp_pong(mis, tmp32);
        break;

    case MIG_CMD_PACKAGED:
        return loadvm_handle_cmd_packaged(mis);

    case MIG_CMD_POSTCOPY_ADVISE:
        return loadvm_postcopy_handle_advise(mis, len);

    case MIG_CMD_POSTCOPY_LISTEN:
        return loadvm_postcopy_handle_listen(mis);

    case MIG_CMD_POSTCOPY_RUN:
        return loadvm_postcopy_handle_run(mis);

    case MIG_CMD_POSTCOPY_RAM_DISCARD:
        return loadvm_postcopy_ram_handle_discard(mis, len);
    }

    return 0;
}

/*
 * Read a footer off the wire and check that it matches the expected section
 *
 * Returns: true if the footer was good
 *          false if there is a problem (and calls error_report to say why)
 */
static bool check_section_footer(QEMUFile *f, SaveStateEntry *se)
{
    int ret;
    uint8_t read_mark;
    uint32_t read_section_id;

    if (!migrate_get_current()->send_section_footer) {
        /* No footer to check */
        return true;
    }

    read_mark = qemu_get_byte(f);

    ret = qemu_file_get_error(f);
    if (ret) {
        error_report("%s: Read section footer failed: %d",
                     __func__, ret);
        return false;
    }

    if (read_mark != QEMU_VM_SECTION_FOOTER) {
      trace_qemu_check_section_footer(se->section_id, se->idstr,
                                      se->load_section_id,
                                      "Missing section footer byte.");
      error_report("Missing section footer for %s, expecting: 0x%x", se->idstr,
                   se->load_section_id);
      return false;
    }

    read_section_id = qemu_get_be32(f);
    if (read_section_id != se->load_section_id) {
      trace_qemu_check_section_footer(se->load_section_id, se->idstr,
                                      read_section_id,
                                      "Mismatched section id in footer.");
      error_report(
          "Mismatched section id in footer for %s -"
          " read 0x%x expected 0x%x",
          se->idstr, read_section_id, se->load_section_id);
      return false;
    }

    /* All good */
    return true;
}

static int
qemu_loadvm_section_start_full(QEMUFile *f, MigrationIncomingState *mis)
{
    uint32_t instance_id, version_id, section_id;
    SaveStateEntry *se;
    char idstr[256];
    int ret;

    /* Read section start */
    section_id = qemu_get_be32(f);
    if (!qemu_get_counted_string(f, idstr)) {
        error_report("Unable to read ID string for section %u",
                     section_id);
        return -EINVAL;
    }
    instance_id = qemu_get_be32(f);
    version_id = qemu_get_be32(f);

    ret = qemu_file_get_error(f);
    if (ret) {
        error_report("%s: Failed to read instance/version ID: %d",
                     __func__, ret);
        return ret;
    }

    trace_qemu_loadvm_state_section_startfull(section_id, idstr,
            instance_id, version_id);
    /* Find savevm section */
    se = find_se(idstr, instance_id);
    if (se == NULL) {
        error_report("Unknown savevm section or instance '%s' %d",
                     idstr, instance_id);
        return -EINVAL;
    }

    /* Validate version */
    if (version_id > se->version_id) {
        error_report("savevm: unsupported version %d for '%s' v%d",
                     version_id, idstr, se->version_id);
        return -EINVAL;
    }
    se->load_version_id = version_id;
    se->load_section_id = section_id;

    /* Validate if it is a device's state */
    if (xen_enabled() && se->is_ram) {
        error_report("loadvm: %s RAM loading not allowed on Xen", idstr);
        return -EINVAL;
    }

    ret = vmstate_load(f, se);
    if (ret < 0) {
        error_report("error while loading state for instance 0x%x of"
                     " device '%s'", instance_id, idstr);
        return ret;
    }
    if (!check_section_footer(f, se)) {
        return -EINVAL;
    }

    return 0;
}

static int
qemu_loadvm_section_part_end(QEMUFile *f, MigrationIncomingState *mis)
{
    uint32_t section_id;
    SaveStateEntry *se;
    int ret;

    section_id = qemu_get_be32(f);

    ret = qemu_file_get_error(f);
    if (ret) {
        error_report("%s: Failed to read section ID: %d",
                     __func__, ret);
        return ret;
    }

    trace_qemu_loadvm_state_section_partend(section_id);
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (se->load_section_id == section_id) {
            break;
        }
    }
    if (se == NULL) {
        error_report("Unknown savevm section %d", section_id);
        return -EINVAL;
    }

    ret = vmstate_load(f, se);
    if (ret < 0) {
        error_report("error while loading state section id %d(%s)",
                     section_id, se->idstr);
        return ret;
    }
    if (!check_section_footer(f, se)) {
        return -EINVAL;
    }

    return 0;
}

static int qemu_loadvm_state_setup(QEMUFile *f)
{
    SaveStateEntry *se;
    int ret;

    trace_loadvm_state_setup();
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (!se->ops || !se->ops->load_setup) {
            continue;
        }
        if (se->ops && se->ops->is_active) {
            if (!se->ops->is_active(se->opaque)) {
                continue;
            }
        }

        ret = se->ops->load_setup(f, se->opaque);
        if (ret < 0) {
            qemu_file_set_error(f, ret);
            error_report("Load state of device %s failed", se->idstr);
            return ret;
        }
    }
    return 0;
}

void qemu_loadvm_state_cleanup(void)
{
    SaveStateEntry *se;

    trace_loadvm_state_cleanup();
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (se->ops && se->ops->load_cleanup) {
            se->ops->load_cleanup(se->opaque);
        }
    }
}

static int qemu_loadvm_state_main(QEMUFile *f, MigrationIncomingState *mis)
{
    uint8_t section_type;
    int ret = 0;

    while (true) {
        section_type = qemu_get_byte(f);

        if (qemu_file_get_error(f)) {
            ret = qemu_file_get_error(f);
            break;
        }

        trace_qemu_loadvm_state_section(section_type);
        switch (section_type) {
        case QEMU_VM_SECTION_START:
        case QEMU_VM_SECTION_FULL:
            ret = qemu_loadvm_section_start_full(f, mis);
            if (ret < 0) {
                goto out;
            }
            break;
        case QEMU_VM_SECTION_PART:
        case QEMU_VM_SECTION_END:
            ret = qemu_loadvm_section_part_end(f, mis);
            if (ret < 0) {
                goto out;
            }
            break;
        case QEMU_VM_COMMAND:
            ret = loadvm_process_command(f);
            trace_qemu_loadvm_state_section_command(ret);
            if ((ret < 0) || (ret & LOADVM_QUIT)) {
                goto out;
            }
            break;
        case QEMU_VM_EOF:
            /* This is the end of migration */
            goto out;
        default:
            error_report("Unknown savevm section type %d", section_type);
            ret = -EINVAL;
            goto out;
        }
    }

out:
    if (ret < 0) {
        qemu_file_set_error(f, ret);
    }
    return ret;
}

int qemu_loadvm_state(QEMUFile *f)
{
#if SNAPSHOT_PROFILE > 1
    int64_t beginTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#endif

    MigrationIncomingState *mis = migration_incoming_get_current();
    Error *local_err = NULL;
    unsigned int v;
    int ret;

    if (qemu_savevm_state_blocked(&local_err)) {
        error_report_err(local_err);
        return -EINVAL;
    }

    v = qemu_get_be32(f);
    if (v != QEMU_VM_FILE_MAGIC) {
        error_report("Not a migration stream");
        return -EINVAL;
    }

    v = qemu_get_be32(f);
    if (v == QEMU_VM_FILE_VERSION_COMPAT) {
        error_report("SaveVM v2 format is obsolete and don't work anymore");
        return -ENOTSUP;
    }
    if (v != QEMU_VM_FILE_VERSION) {
        error_report("Unsupported migration stream version");
        return -ENOTSUP;
    }

    if (qemu_loadvm_state_setup(f) != 0) {
        return -EINVAL;
    }

    if (migrate_get_current()->send_configuration) {
        if (qemu_get_byte(f) != QEMU_VM_CONFIGURATION) {
            error_report("Configuration section missing");
            return -EINVAL;
        }
        ret = vmstate_load_state(f, &vmstate_configuration, &savevm_state, 0);

        if (ret) {
            return ret;
        }
    }

#if SNAPSHOT_PROFILE > 1
    int64_t vmstateLoadedTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("vmstate load time %.03f ms\n",
           (vmstateLoadedTime - beginTime)/1000000.0);
#endif

    cpu_synchronize_all_pre_loadvm();
    ret = qemu_loadvm_state_main(f, mis);
    qemu_event_set(&mis->main_thread_load_event);

    trace_qemu_loadvm_state_post_main(ret);

    if (mis->have_listen_thread) {
        /* Listen thread still going, can't clean up yet */
        return ret;
    }

    if (ret == 0) {
        ret = qemu_file_get_error(f);
    }

    /*
     * Try to read in the VMDESC section as well, so that dumping tools that
     * intercept our migration stream have the chance to see it.
     */

    /* We've got to be careful; if we don't read the data and just shut the fd
     * then the sender can error if we close while it's still sending.
     * We also mustn't read data that isn't there; some transports (RDMA)
     * will stall waiting for that data when the source has already closed.
     */
    if (ret == 0 && should_send_vmdesc()) {
        uint8_t *buf;
        uint32_t size;
        uint8_t  section_type = qemu_get_byte(f);

        if (section_type != QEMU_VM_VMDESCRIPTION) {
            error_report("Expected vmdescription section, but got %d",
                         section_type);
            /*
             * It doesn't seem worth failing at this point since
             * we apparently have an otherwise valid VM state
             */
        } else {
            buf = g_malloc(0x1000);
            size = qemu_get_be32(f);

            while (size > 0) {
                uint32_t read_chunk = MIN(size, 0x1000);
                qemu_get_buffer(f, buf, read_chunk);
                size -= read_chunk;
            }
            g_free(buf);
        }
    }

    qemu_loadvm_state_cleanup();
    cpu_synchronize_all_post_init();

#if SNAPSHOT_PROFILE > 1
    int64_t endTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("loadvm_state() time %.03f ms\n",
           (endTime - beginTime)/1000000.0);
#endif


    return ret;
}

void qemu_set_snapshot_callbacks(const QEMUSnapshotCallbacks* callbacks)
{
    if (callbacks) {
        memcpy(&s_snapshot_callbacks, callbacks, sizeof(s_snapshot_callbacks));
    } else {
        memset(&s_snapshot_callbacks, 0, sizeof(s_snapshot_callbacks));
    }
}

/* Default error reporting callback for snapshot functions */
static void report_error_err_cb(void* dummy, Error* err, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (err) {
        if (fmt) {
            error_vprepend(&err, fmt, ap);
        }
        error_report_err(err);
    } else if (fmt) {
        error_vreport(fmt, ap);
    }
    va_end(ap);
}

/* Fill the message callback struct to report messages into |mon| */
static QEMUMessageCallback make_monitor_message_cb(Monitor* mon)
{
    typedef void (*print_func)(void* opaque, const char* fmt, ...);

    QEMUMessageCallback result = {
            .opaque = mon,
            .out = (print_func)monitor_printf,
            .err = report_error_err_cb,
    };
    return result;
}

int save_snapshot(const char *name, Error **errp) {
    // TODO(jansene): Merge artifact that needs to be cleaned up, callbacks are
    // no longer really needed
    QEMUMessageCallback cb = make_monitor_message_cb(NULL);
    return qemu_savevm(name, &cb);
}

void hmp_savevm(Monitor *mon, const QDict *qdict)
{
    const char *name = qdict_get_try_str(qdict, "name");
    QEMUMessageCallback cb = make_monitor_message_cb(mon);
    qemu_savevm(name, &cb);
}

int qemu_savevm(const char* name, const QEMUMessageCallback* messages)
{
#if SNAPSHOT_PROFILE
    int64_t beginTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#endif
    BlockDriverState *bs, *bs1;
    QEMUSnapshotInfo sn1, *sn = &sn1, old_sn1, *old_sn = &old_sn1;
    int ret = -1;
    QEMUFile *f;
    int saved_vm_running;
    uint64_t vm_state_size;
    qemu_timeval tv;
    struct tm tm;
    Error *local_err = NULL;
    AioContext *aio_context;

    if (!replay_can_snapshot()) {
        error_report("Record/replay does not allow making snapshot "
                     "right now. Try once more later.");
        return ret;
    }

    if (!bdrv_all_can_snapshot(&bs)) {
        if (s_snapshot_callbacks.savevm.on_quick_fail) {
            s_snapshot_callbacks.savevm.on_quick_fail(name, -ENOTSUP);
        }
        messages->out(messages->opaque, "Device '%s' is writable but does not "
                       "support snapshots.\n", bdrv_get_device_name(bs));
        return ret;
    }

    /* Delete old snapshots of the same name */
    if (name && bdrv_all_delete_snapshot(name, &bs1, &local_err) < 0) {
        if (s_snapshot_callbacks.savevm.on_quick_fail) {
            s_snapshot_callbacks.savevm.on_quick_fail(name, -EFAULT);
        }
        messages->err(messages->opaque, local_err,
                     "Error while deleting snapshot on device '%s': ",
                     bdrv_get_device_name(bs1));
        return -1;
    }

    bs = bdrv_all_find_vmstate_bs();
    if (bs == NULL) {
        if (s_snapshot_callbacks.savevm.on_quick_fail) {
            s_snapshot_callbacks.savevm.on_quick_fail(name, -ENOTSUP);
        }
        messages->out(messages->opaque, "No block device can accept snapshots\n");
        return ret;
    }
    aio_context = bdrv_get_aio_context(bs);

    saved_vm_running = runstate_is_running();

    ret = global_state_store();
    if (ret) {
        if (s_snapshot_callbacks.savevm.on_quick_fail) {
            s_snapshot_callbacks.savevm.on_quick_fail(name, -EFAULT);
        }
        messages->out(messages->opaque, "Error saving global state\n");
        return ret;
    }
    vm_stop(RUN_STATE_SAVE_VM);

    bdrv_drain_all_begin();

    aio_context_acquire(aio_context);

    memset(sn, 0, sizeof(*sn));

    /* fill auxiliary fields */
    qemu_gettimeofday(&tv);
    sn->date_sec = tv.tv_sec;
    sn->date_nsec = tv.tv_usec * 1000;
    sn->vm_clock_nsec = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    if (name) {
        ret = bdrv_snapshot_find(bs, old_sn, name);
        if (ret >= 0) {
            pstrcpy(sn->name, sizeof(sn->name), old_sn->name);
            pstrcpy(sn->id_str, sizeof(sn->id_str), old_sn->id_str);
        } else {
            pstrcpy(sn->name, sizeof(sn->name), name);
        }
    } else {
        /* cast below needed for OpenBSD where tv_sec is still 'long' */
        localtime_r((const time_t *)&tv.tv_sec, &tm);
        strftime(sn->name, sizeof(sn->name), "vm-%Y%m%d%H%M%S", &tm);
    }

    if (s_snapshot_callbacks.savevm.on_start) {
        ret = s_snapshot_callbacks.savevm.on_start(name);
        if (ret) {
            messages->out(messages->opaque, "Error %d from the snapshot callback\n", ret);
            return ret;
        }
    }

    /* save the VM state */
    f = qemu_fopen_bdrv(bs, 1);
    if (!f) {
        ret = -1;
        messages->out(messages->opaque, "Could not open VM state file\n");
        goto the_end;
    }
    qemu_file_set_hooks(f, sSaveFileHooks);

    qemu_file_set_pb(f, s_protobuf);

#if SNAPSHOT_PROFILE > 1
    int64_t beginSaveStateTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("preparation time %.03f ms\n",
           (beginSaveStateTime - beginTime)/1000000.0);
#endif

    ret = qemu_savevm_state(f, &local_err);
    vm_state_size = qemu_ftell(f);
    qemu_fclose(f);
    if (ret < 0) {
        messages->err(messages->opaque, local_err, NULL);
        goto the_end;
    }

#if SNAPSHOT_PROFILE > 1
    int64_t endSaveStateBeginSnapTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("qemu_savevm_state time %.03f ms\n",
            (endSaveStateBeginSnapTime - beginSaveStateTime)/1000000.0);
#endif
    /* The bdrv_all_create_snapshot() call that follows acquires the AioContext
     * for itself.  BDRV_POLL_WHILE() does not support nested locking because
     * it only releases the lock once.  Therefore synchronous I/O will deadlock
     * unless we release the AioContext before bdrv_all_create_snapshot().
     */
    aio_context_release(aio_context);
    aio_context = NULL;

    ret = bdrv_all_create_snapshot(sn, bs, vm_state_size, &bs);
    if (ret < 0) {
        messages->out(messages->opaque, "Error while creating snapshot on '%s'\n",
                       bdrv_get_device_name(bs));
        goto the_end;
    }

    ret = 0;

 the_end:
    if (aio_context) {
        aio_context_release(aio_context);
    }

    if (s_snapshot_callbacks.savevm.on_end) {
        s_snapshot_callbacks.savevm.on_end(name, ret);
    }

#if SNAPSHOT_PROFILE
    int64_t endTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#if SNAPSHOT_PROFILE > 1
    printf("snapshotting block devices time %.03f ms\n",
           (endTime - endSaveStateBeginSnapTime)/1000000.0);
#endif // > 1
    printf("savevm time %.03f ms\n",
           (endTime - beginTime)/1000000.0);
#endif // > 0

    bdrv_drain_all_end();

    if (saved_vm_running) {
        vm_start();
    }
    return ret;
}

void qmp_xen_save_devices_state(const char *filename, bool has_live, bool live,
                                Error **errp)
{
    QEMUFile *f;
    QIOChannelFile *ioc;
    int saved_vm_running;
    int ret;

    if (!has_live) {
        /* live default to true so old version of Xen tool stack can have a
         * successfull live migration */
        live = true;
    }

    saved_vm_running = runstate_is_running();
    vm_stop(RUN_STATE_SAVE_VM);
    global_state_store_running();

    ioc = qio_channel_file_new_path(filename, O_WRONLY | O_CREAT, 0660, errp);
    if (!ioc) {
        goto the_end;
    }
    qio_channel_set_name(QIO_CHANNEL(ioc), "migration-xen-save-state");
    f = qemu_fopen_channel_output(QIO_CHANNEL(ioc));
    object_unref(OBJECT(ioc));
    ret = qemu_save_device_state(f);
    if (ret < 0 || qemu_fclose(f) < 0) {
        error_setg(errp, QERR_IO_ERROR);
    } else {
        /* libxl calls the QMP command "stop" before calling
         * "xen-save-devices-state" and in case of migration failure, libxl
         * would call "cont".
         * So call bdrv_inactivate_all (release locks) here to let the other
         * side of the migration take controle of the images.
         */
        if (live && !saved_vm_running) {
            ret = bdrv_inactivate_all();
            if (ret) {
                error_setg(errp, "%s: bdrv_inactivate_all() failed (%d)",
                           __func__, ret);
            }
        }
    }

 the_end:
    if (saved_vm_running) {
        vm_start();
    }
}

void qmp_xen_load_devices_state(const char *filename, Error **errp)
{
    QEMUFile *f;
    QIOChannelFile *ioc;
    int ret;

    /* Guest must be paused before loading the device state; the RAM state
     * will already have been loaded by xc
     */
    if (runstate_is_running()) {
        error_setg(errp, "Cannot update device state while vm is running");
        return;
    }
    vm_stop(RUN_STATE_RESTORE_VM);

    ioc = qio_channel_file_new_path(filename, O_RDONLY | O_BINARY, 0, errp);
    if (!ioc) {
        return;
    }
    qio_channel_set_name(QIO_CHANNEL(ioc), "migration-xen-load-state");
    f = qemu_fopen_channel_input(QIO_CHANNEL(ioc));
    object_unref(OBJECT(ioc));

    ret = qemu_loadvm_state(f);
    qemu_fclose(f);
    if (ret < 0) {
        error_setg(errp, QERR_IO_ERROR);
    }
    migration_incoming_state_destroy();
}

/* Loadvm wants to print messages directly to the console */
static void printf_wrap_cb(void* dummy, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

int load_snapshot(const char *name, Error **errp)
{
    QEMUMessageCallback cb = {
        .opaque = NULL,
        .out = printf_wrap_cb,
        .err = report_error_err_cb,
    };
    return qemu_loadvm(name, &cb);
}

int qemu_loadvm(const char* name, const QEMUMessageCallback* messages)
{
#if SNAPSHOT_PROFILE
    int64_t beginTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#endif
    BlockDriverState *bs, *bs_vm_state;
    QEMUSnapshotInfo sn;
    QEMUFile *f;
    Error *local_err = NULL;
    int ret;
    AioContext *aio_context;
    MigrationIncomingState *mis = migration_incoming_get_current();

    if (!replay_can_snapshot()) {
        error_report("Record/replay does not allow loading snapshot "
                     "right now. Try once more later.");
        return -EINVAL;
    }

    if (!bdrv_all_can_snapshot(&bs)) {
        if (s_snapshot_callbacks.loadvm.on_quick_fail) {
            s_snapshot_callbacks.loadvm.on_quick_fail(name, -ENOTSUP);
        }
        messages->err(messages->opaque, NULL,
                     "Device '%s' is writable but does not support snapshots.",
                     bdrv_get_device_name(bs));
        return -ENOTSUP;
    }
    ret = bdrv_all_find_snapshot(name, &bs);
    if (ret < 0) {
        if (s_snapshot_callbacks.loadvm.on_quick_fail) {
            s_snapshot_callbacks.loadvm.on_quick_fail(name, -ENOENT);
        }
        messages->err(messages->opaque, NULL,
                     "Device '%s' does not have the requested snapshot '%s'",
                     bdrv_get_device_name(bs), name);
        return ret;
    }

    bs_vm_state = bdrv_all_find_vmstate_bs();
    if (!bs_vm_state) {
        if (s_snapshot_callbacks.loadvm.on_quick_fail) {
            s_snapshot_callbacks.loadvm.on_quick_fail(name, -ENOTSUP);
        }
        messages->err(messages->opaque, NULL,
                     "No block device supports snapshots");
        return -ENOTSUP;
    }
    aio_context = bdrv_get_aio_context(bs_vm_state);

    /* Don't even try to load empty VM states */
    aio_context_acquire(aio_context);
    ret = bdrv_snapshot_find(bs_vm_state, &sn, name);
    aio_context_release(aio_context);
    if (ret < 0) {
        if (s_snapshot_callbacks.loadvm.on_quick_fail) {
            s_snapshot_callbacks.loadvm.on_quick_fail(name, ret);
        }
        return ret;
    } else if (sn.vm_state_size == 0) {
        if (s_snapshot_callbacks.loadvm.on_quick_fail) {
            s_snapshot_callbacks.loadvm.on_quick_fail(name, -EINVAL);
        }
        messages->err(messages->opaque, NULL,
                     "This is a disk-only snapshot. Revert to it offline "
                     "using qemu-img.");
        return -EINVAL;
    }

    if (s_snapshot_callbacks.loadvm.on_start) {
        ret = s_snapshot_callbacks.loadvm.on_start(name);
        if (ret) {
            messages->err(messages->opaque, NULL,
                         "Error %d from the snapshot callback", ret);
            return -EINVAL;
        }
    }

    /* Flush all IO requests so they don't interfere with the new state.  */
    bdrv_drain_all_begin();

#if SNAPSHOT_PROFILE > 1
    int64_t gotoTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("preparation time %.03f ms\n", (gotoTime - beginTime)/1000000.0);
#endif

    ret = bdrv_all_goto_snapshot(name, &bs, &local_err);
    if (ret < 0) {
        if (s_snapshot_callbacks.loadvm.on_end) {
            s_snapshot_callbacks.loadvm.on_end(name, ret);
        }
        messages->err(messages->opaque, NULL,
                     "Error %d while activating snapshot '%s' on '%s'",
                     ret, name, bdrv_get_device_name(bs));
        goto err_drain;
    }

#if SNAPSHOT_PROFILE > 1
    int64_t gotoEndTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("goto snapshot time %.03f ms\n", (gotoEndTime - gotoTime)/1000000.0);
#endif

    /* restore the VM state */
    f = qemu_fopen_bdrv(bs_vm_state, 0);
    if (!f) {
        if (s_snapshot_callbacks.loadvm.on_end) {
            s_snapshot_callbacks.loadvm.on_end(name, -EINVAL);
        }
        messages->err(messages->opaque, NULL, "Could not open VM state file");
        ret = -EINVAL;
        goto err_drain;
    }
    qemu_file_set_hooks(f, sLoadFileHooks);

    qemu_file_set_pb(f, s_protobuf);

    qemu_system_reset(SHUTDOWN_CAUSE_NONE);
    mis->from_src_file = f;

#if SNAPSHOT_PROFILE > 1
    int64_t resetEndTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
    printf("system reset time %.03f ms\n",
           (resetEndTime - gotoEndTime)/1000000.0);
#endif

    aio_context_acquire(aio_context);
    ret = qemu_loadvm_state(f);
    migration_incoming_state_destroy();
    aio_context_release(aio_context);

    migration_incoming_state_destroy();

    if (s_snapshot_callbacks.loadvm.on_end) {
        s_snapshot_callbacks.loadvm.on_end(name, ret);
    }

err_drain:
    bdrv_drain_all_end();

    if (ret < 0) {
        messages->err(messages->opaque, NULL,
                     "Error %d while loading VM state", ret);
        return ret;
    }

#if SNAPSHOT_PROFILE
    int64_t endTime = qemu_clock_get_ns(QEMU_CLOCK_REALTIME);
#if SNAPSHOT_PROFILE > 1
    printf("loadvm_state() time %.03f ms\n", (endTime - resetEndTime)/1000000.0);
#endif // > 1
    printf("loadvm time %.03f ms\n", (endTime - beginTime)/1000000.0);
#endif // > 0

    return 0;
}

void hmp_delvm(Monitor *mon, const QDict *qdict)
{
    const char *name = qdict_get_try_str(qdict, "name");
    QEMUMessageCallback cb = make_monitor_message_cb(mon);
    qemu_delvm(name, &cb);
}

int qemu_delvm(const char* name, const QEMUMessageCallback* messages)
{
    BlockDriverState *bs;
    Error *err = NULL;
    int ret;

    if (s_snapshot_callbacks.delvm.on_start) {
        ret = s_snapshot_callbacks.delvm.on_start(name);
        if (ret) {
            messages->err(messages->opaque, err,
                          "Error %d from the snapshot callback", ret);
            return -1;
        }
    }

    ret = bdrv_all_delete_snapshot(name, &bs, &err);

    if (s_snapshot_callbacks.delvm.on_end) {
        s_snapshot_callbacks.delvm.on_end(name, ret);
    }

    if (ret < 0) {
        messages->err(messages->opaque, err,
                      "Error while deleting snapshot on device '%s': ",
                      bdrv_get_device_name(bs));
    }

    return ret;
}

void hmp_info_snapshots(Monitor *mon, const QDict *qdict)
{
    QEMUMessageCallback cb = make_monitor_message_cb(mon);
    qemu_listvms(NULL, NULL, &cb);
}

int qemu_listvms(char(*names)[256], int* names_count, const QEMUMessageCallback* messages)
{
    BlockDriverState *bs, *bs1;
    BdrvNextIterator it1;
    QEMUSnapshotInfo *sn_tab, *sn;
    bool no_snapshot = true;
    int nb_sns, i;
    int total;
    int *global_snapshots;
    AioContext *aio_context;

    typedef struct SnapshotEntry {
        QEMUSnapshotInfo sn;
        QTAILQ_ENTRY(SnapshotEntry) next;
    } SnapshotEntry;

    typedef struct ImageEntry {
        const char *imagename;
        QTAILQ_ENTRY(ImageEntry) next;
        QTAILQ_HEAD(, SnapshotEntry) snapshots;
    } ImageEntry;

    QTAILQ_HEAD(, ImageEntry) image_list =
        QTAILQ_HEAD_INITIALIZER(image_list);

    ImageEntry *image_entry, *next_ie;
    SnapshotEntry *snapshot_entry;

    bs = bdrv_all_find_vmstate_bs();
    if (!bs) {
        messages->out(messages->opaque, "No available block device supports snapshots\n");
        if (names_count) {
            *names_count = 0;
        }
        return 0;
    }
    aio_context = bdrv_get_aio_context(bs);

    aio_context_acquire(aio_context);
    nb_sns = bdrv_snapshot_list(bs, &sn_tab);
    aio_context_release(aio_context);

    if (nb_sns < 0) {
        messages->out(messages->opaque, "bdrv_snapshot_list: error %d\n", nb_sns);
        return nb_sns;
    }

    for (bs1 = bdrv_first(&it1); bs1; bs1 = bdrv_next(&it1)) {
        int bs1_nb_sns = 0;
        ImageEntry *ie;
        SnapshotEntry *se;
        AioContext *ctx = bdrv_get_aio_context(bs1);

        aio_context_acquire(ctx);
        if (bdrv_can_snapshot(bs1)) {
            sn = NULL;
            bs1_nb_sns = bdrv_snapshot_list(bs1, &sn);
            if (bs1_nb_sns > 0) {
                no_snapshot = false;
                ie = g_new0(ImageEntry, 1);
                ie->imagename = bdrv_get_device_name(bs1);
                QTAILQ_INIT(&ie->snapshots);
                QTAILQ_INSERT_TAIL(&image_list, ie, next);
                for (i = 0; i < bs1_nb_sns; i++) {
                    se = g_new0(SnapshotEntry, 1);
                    se->sn = sn[i];
                    QTAILQ_INSERT_TAIL(&ie->snapshots, se, next);
                }
            }
            g_free(sn);
        }
        aio_context_release(ctx);
    }

    if (no_snapshot) {
        messages->out(messages->opaque, "There is no snapshot available.\n");
        if (names_count) {
            *names_count = 0;
        }
        return 0;
    }

    global_snapshots = g_new0(int, nb_sns);
    total = 0;
    for (i = 0; i < nb_sns; i++) {
        SnapshotEntry *next_sn;
        if (bdrv_all_find_snapshot(sn_tab[i].name, &bs1) == 0) {
            global_snapshots[total] = i;
            if (names && names_count && total < *names_count) {
                strcpy(names[total], sn_tab[i].name);
            }
            total++;
            QTAILQ_FOREACH(image_entry, &image_list, next) {
                QTAILQ_FOREACH_SAFE(snapshot_entry, &image_entry->snapshots,
                                    next, next_sn) {
                    if (!strcmp(sn_tab[i].name, snapshot_entry->sn.name)) {
                        QTAILQ_REMOVE(&image_entry->snapshots, snapshot_entry,
                                      next);
                        g_free(snapshot_entry);
                    }
                }
            }
        }
    }

    messages->out(messages->opaque, "List of snapshots present on all disks:\n");

    if (total > 0) {
        bdrv_snapshot_dump((fprintf_function)messages->out, messages->opaque, NULL);
        messages->out(messages->opaque, "\n");
        for (i = 0; i < total; i++) {
            sn = &sn_tab[global_snapshots[i]];
            /* The ID is not guaranteed to be the same on all images, so
             * overwrite it.
             */
            pstrcpy(sn->id_str, sizeof(sn->id_str), "--");
            bdrv_snapshot_dump((fprintf_function)messages->out, messages->opaque, sn);
            messages->out(messages->opaque, "\n");
        }
    } else {
        messages->out(messages->opaque, "None\n");
    }

    QTAILQ_FOREACH(image_entry, &image_list, next) {
        if (QTAILQ_EMPTY(&image_entry->snapshots)) {
            continue;
        }
        messages->out(messages->opaque,
                       "\nList of partial (non-loadable) snapshots on '%s':\n",
                       image_entry->imagename);
        bdrv_snapshot_dump((fprintf_function)messages->out, messages->opaque, NULL);
        messages->out(messages->opaque, "\n");
        QTAILQ_FOREACH(snapshot_entry, &image_entry->snapshots, next) {
            bdrv_snapshot_dump((fprintf_function)messages->out, messages->opaque,
                               &snapshot_entry->sn);
            messages->out(messages->opaque, "\n");
        }
    }

    QTAILQ_FOREACH_SAFE(image_entry, &image_list, next, next_ie) {
        SnapshotEntry *next_sn;
        QTAILQ_FOREACH_SAFE(snapshot_entry, &image_entry->snapshots, next,
                            next_sn) {
            g_free(snapshot_entry);
        }
        g_free(image_entry);
    }
    g_free(sn_tab);
    g_free(global_snapshots);

    if (names_count) {
        *names_count = total;
    }
    return 0;
}

void vmstate_register_ram(MemoryRegion *mr, DeviceState *dev)
{
    qemu_ram_set_migrate(mr->ram_block, true);
    qemu_ram_set_idstr(mr->ram_block,
                       memory_region_name(mr), dev);
}

void vmstate_unregister_ram(MemoryRegion *mr, DeviceState *dev)
{
    qemu_ram_set_migrate(mr->ram_block, false);
    qemu_ram_unset_idstr(mr->ram_block);
}

void vmstate_register_ram_global(MemoryRegion *mr)
{
    vmstate_register_ram(mr, NULL);
}

bool vmstate_check_only_migratable(const VMStateDescription *vmsd)
{
    /* check needed if --only-migratable is specified */
    if (!migrate_get_current()->only_migratable) {
        return true;
    }

    return !(vmsd && vmsd->unmigratable);
}
