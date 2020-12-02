/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "qemu/main-loop.h"
#include "trace.h"
#include "hw/misc/goldfish_sync.h"
#include "migration/register.h"

#include <assert.h>

// TODO(digit): Plug better logging for Android emulator.
#define DPRINT(...)

// Goldfish sync commands=======================================================

// Batch command formats:
// Host->guest:
struct goldfish_sync_batch_cmd {
    // sorted by size for alignment
    uint64_t handle;
    uint64_t hostcmd_handle;
    uint32_t cmd;
    uint32_t time_arg;
};

// guest->host:
struct goldfish_sync_batch_guestcmd {
    uint64_t host_command; // uint64_t for alignment
    uint64_t glsync_handle;
    uint64_t thread_handle;
    uint64_t guest_timeline_handle;
};

// This is a suffix of batch cmd.
// The purpose of the "pending_cmd" variant
// is so we can queue up several batch commands
// in the same IRQ level.
struct goldfish_sync_pending_cmd {
    uint64_t handle;
    uint64_t hostcmd_handle;
    uint32_t cmd;
    uint32_t time_arg;
    struct goldfish_sync_pending_cmd* next;
};

// Command numbers that can only be sent from the guest to the device.
#define SYNC_GUEST_CMD_READY 0
#define SYNC_GUEST_CMD_TRIGGER_HOST_WAIT 5

// The register layout is:

#define SYNC_REG_BATCH_COMMAND                0x00 // host->guest batch commands
#define SYNC_REG_BATCH_GUESTCOMMAND           0x04 // guest->host batch commands
#define SYNC_REG_BATCH_COMMAND_ADDR           0x08 // communicate physical address of host->guest batch commands
#define SYNC_REG_BATCH_COMMAND_ADDR_HIGH      0x0c // 64-bit part
#define SYNC_REG_BATCH_GUESTCOMMAND_ADDR      0x10 // communicate physical address of guest->host commands
#define SYNC_REG_BATCH_GUESTCOMMAND_ADDR_HIGH 0x14 // 64-bit part
#define SYNC_REG_INIT                         0x18 // to signal that the device has been detected by the kernel

// The goldfish sync device is represented by the goldfish_sync_state struct.
// Besides the machinery necessary to do I/O with the guest:
// |pending|: a linked list of |goldfish_sync_pending_cmd|'s
// that need to be processed by the kernel.
// |first_pending_cmd|: maintains a pointer to the earliest pending command
// that needs to be sent. We plan to process pending commands in the same
// order in which they were received.
// |current|: Intermediate value that represents a popped-off pending
// command.
// |batch_cmd_addr|: Physical memory address where we write the details
// of each command that comes in from the host. We also read replies
// from the same address (if applicable)
// |batch_guestcmd_addr|: Physical memory address where we read the
// details of each command that comes in from the guest.
// |ops|: Callbacks to other areas of the emulator that need to be
// triggered upon reading a guest->host command.
struct goldfish_sync_state {
    SysBusDevice parent;
    MemoryRegion iomem;
    qemu_irq irq;

    struct goldfish_sync_pending_cmd* pending;
    struct goldfish_sync_pending_cmd* first_pending_cmd;
    struct goldfish_sync_pending_cmd* current;

    uint64_t batch_cmd_addr;
    uint64_t batch_guestcmd_addr;

    struct goldfish_sync_ops* ops;
};

static struct goldfish_sync_state* s_goldfish_sync_dev;

static const GoldfishSyncServiceOps* service_ops = NULL;

void goldfish_sync_set_service_ops(const GoldfishSyncServiceOps *ops) {
    service_ops = ops;
}

// Command list operations======================================================

static struct goldfish_sync_pending_cmd*
goldfish_sync_new_cmd(void) {
    struct goldfish_sync_pending_cmd* res;
    res = g_malloc0(sizeof(struct goldfish_sync_pending_cmd));
    return res;
}

static void
goldfish_sync_free_cmd(struct goldfish_sync_pending_cmd* cmd) {
    g_free(cmd);
}

static void
goldfish_sync_push_cmd(struct goldfish_sync_state* state,
                       struct goldfish_sync_pending_cmd* cmd) {

    cmd->next = NULL;

    if (state->pending) {
        state->pending->next = cmd;
    }

    // We need to reset first_pending_cmd if
    // the list was empty to start with.
    if (!state->pending) {
        state->first_pending_cmd = cmd;
    }

    state->pending = cmd;
}

static struct goldfish_sync_pending_cmd*
goldfish_sync_pop_first_cmd(struct goldfish_sync_state* state) {

    struct goldfish_sync_pending_cmd* popped =
        state->first_pending_cmd;

    // Update list structure as follows:
    // 1. Don't do anything if its already empty.
    if (!popped) return NULL;

    // 2. Update first_pending_cmd and pending fields,
    // changing list structure to reflect the change.
    state->first_pending_cmd = popped->next;
    if (state->pending == popped) state->pending = NULL;

    return popped;
}

static void goldfish_sync_clear_pending(struct goldfish_sync_state* state) {
    struct goldfish_sync_pending_cmd* curr =
        state->pending;

    while (curr) {
        struct goldfish_sync_pending_cmd* tmp = curr;
        curr = curr->next;
        goldfish_sync_free_cmd(tmp);
    }

    state->pending = NULL;
    state->first_pending_cmd = NULL;
}

static void goldfish_sync_reset_device(struct goldfish_sync_state* state) {
    goldfish_sync_clear_pending(state);
    qemu_set_irq(state->irq, 0);
}

// Callbacks and hw_funcs struct================================================

// goldfish_sync_send_command does the actual work of setting up
// the sync device registers, causing IRQ blip,
// and waiting for + reading back results from the guest
// (if appplicable)
void goldfish_sync_send_command(uint32_t cmd,
                                uint64_t handle,
                                uint32_t time_arg,
                                uint64_t hostcmd_handle) {

    struct goldfish_sync_state* s;
    struct goldfish_sync_pending_cmd* to_send;
    s = s_goldfish_sync_dev;

    DPRINT("incoming cmd. "
           "cmd=%u "
           "handle=0x%llx "
           "time_arg=%u "
           "hostcmd_handle=0x%llx",
           cmd, handle, time_arg, hostcmd_handle);

    to_send = goldfish_sync_new_cmd();
    to_send->cmd = cmd;
    to_send->handle = handle;
    to_send->time_arg = time_arg;
    to_send->hostcmd_handle = hostcmd_handle;

    goldfish_sync_push_cmd(s, to_send);

    qemu_set_irq(s->irq, 1);

    DPRINT("Exit");
}

#define TYPE_GOLDFISH_SYNC "goldfish_sync"
#define GOLDFISH_SYNC(obj) \
    OBJECT_CHECK(struct goldfish_sync_state, (obj), TYPE_GOLDFISH_SYNC)

static uint64_t
goldfish_sync_read(void *opaque,
                   hwaddr offset,
                   unsigned size) {

    DPRINT("opaque=%p offset=0x%lx sz=0x%x", opaque, offset, size);

    struct goldfish_sync_state* s = opaque;
    struct goldfish_sync_batch_cmd* curr_batch_cmd;

    switch (offset) {
    // SYNC_REG_BATCH_COMMAND read:
    // This is triggered by the kernel whenever we are processing
    // any host->guest sync command, such as timeline increment.
    // The role of this part is to pass command info (in the struct
    // |goldfish_sync_batch_cmd|'s to the guest where the commands
    // are actually run.
    //
    // SYNC_REG_BATCH_COMMAND is the first thing
    // to happen on a IRQ level raise, and will continue re-issuing
    // throughout IRQ active high, until all pending commands on the
    // host (the linked list) are processed. At that point, IRQ
    // is lowered.
    case SYNC_REG_BATCH_COMMAND:
        if (!s->batch_cmd_addr) return 0;

        s->current = goldfish_sync_pop_first_cmd(s);
        if (!s->current) {
            DPRINT("Out of pending commands. Lower IRQ.");
            struct goldfish_sync_batch_cmd quit_cmd = {
                .cmd = 0, .handle = 0, .time_arg = 0, .hostcmd_handle = 0
            };
            cpu_physical_memory_write(s->batch_cmd_addr,
                                      (void*)&quit_cmd,
                                      sizeof(struct goldfish_sync_batch_cmd));
            qemu_set_irq(s->irq, 0);
            return 0;
        } else {
            // goldfish_sync_batch_cmd is a prefix of
            // goldfish_sync_pending_cmd, so we can avoid a copy.
            curr_batch_cmd = (struct goldfish_sync_batch_cmd*)(s->current);
            DPRINT("read SYNC_REG_BATCH_COMMAND. writing to batch addr: "
                   "cmd=%u handle=0x%llx time_arg=%u hostcmd_handle=0x%llx\n",
                   curr_batch_cmd->cmd,
                   (unsigned long long)curr_batch_cmd->handle,
                   curr_batch_cmd->time_arg,
                   (unsigned long long)curr_batch_cmd->hostcmd_handle);
            cpu_physical_memory_write(s->batch_cmd_addr,
                                      (void*)curr_batch_cmd,
                                      sizeof(struct goldfish_sync_batch_cmd));
            goldfish_sync_free_cmd(s->current);
            s->current = NULL;
        }
        DPRINT("done SYNC_REG_BATCH_COMMAND. returning result val?");
        return 0;
    // SYNC_REG_BATCH_(GUEST)COMMAND_ADDR(_HIGH) read:
    // Used solely to verify that the |s->batch_cmd_addr| on the host
    // is the same physical address as the corresponding one
    // on the guest.
    case SYNC_REG_BATCH_COMMAND_ADDR:
        return (uint64_t)(uint32_t)(s->batch_cmd_addr);
    case SYNC_REG_BATCH_COMMAND_ADDR_HIGH:
        return (uint64_t)(uint32_t)(s->batch_cmd_addr >> 32);
    case SYNC_REG_BATCH_GUESTCOMMAND_ADDR:
        return (uint64_t)(uint32_t)(s->batch_guestcmd_addr);
    case SYNC_REG_BATCH_GUESTCOMMAND_ADDR_HIGH:
        return (uint64_t)(uint32_t)(s->batch_guestcmd_addr >> 32);
    default:
        DPRINT("INVALID sync_read! reg=%u", offset);
        assert(false);
        return 0;
    }
    return 0;
}
// The guest kernel's goldfish_sync driver triggers goldfish_sync_read
// and goldfish_sync_write. The only purpose of those two functions is to
// pass the right value to and from the special |regs| field of the
// goldfish_sync_state.

static void
goldfish_sync_write(void *opaque,
                    hwaddr offset,
                    uint64_t val,
                    unsigned size) {
    DPRINT("opaque=%p "
           "offset=0x%lx "
           "sz=0x%x "
           "val=%" PRIu64,
           opaque, offset, size, val);

    struct goldfish_sync_state* s = opaque;

    switch (offset) {
    // SYNC_REG_BATCH_COMMAND write: Used to send acknowledgements
    // to host->guest commands. Some commands that can be issued from
    // the host, such as |goldfish_sync_create_timeline|, require a
    // return value of the timeline pointer itself. Not all commands
    // use this; for example, we never acknowledge any
    // |goldfish_sync_timeline_inc| commands because that would
    // decrease performance.
    case SYNC_REG_BATCH_COMMAND:
        if (!s->batch_cmd_addr) break;

        DPRINT("write SYNC_REG_BATCH_COMMAND. obtaining batch cmd vals.");
        struct goldfish_sync_batch_cmd incoming = {
            .cmd = 0,
            .handle = 0,
            .time_arg = 0,
            .hostcmd_handle = 0,
        };
        cpu_physical_memory_read(s->batch_cmd_addr,
                                 (void*)&incoming,
                                 sizeof(struct goldfish_sync_batch_cmd));
        DPRINT("got: cmd=%u handle=0x%llx time_arg=%u hostcmd_handle=0x%llx",
               incoming.cmd,
               incoming.handle,
               incoming.time_arg,
               incoming.hostcmd_handle);
        service_ops->receive_hostcmd_result(
               incoming.cmd,
               incoming.handle,
               incoming.time_arg,
               incoming.hostcmd_handle);
        break;
    // SYNC_REG_BATCH_GUESTCOMMAND write: Used to send
    // guest->host commands. Currently, the only guest->host command
    // that matters is SYNC_GUEST_CMD_TRIGGER_HOST_WAIT, which is used
    // to cause a OpenGL client wait on the host GPU/CPU.
    case SYNC_REG_BATCH_GUESTCOMMAND:
        if (!s->batch_guestcmd_addr) break;

        DPRINT("write SYNC_REG_BATCH_GUESTCOMMAND. obtaining batch cmd vals.");
        struct goldfish_sync_batch_guestcmd guest_incoming = {
            .host_command = 0,
            .glsync_handle = 0,
            .thread_handle = 0,
            .guest_timeline_handle = 0,
        };
        cpu_physical_memory_read(s->batch_guestcmd_addr,
                                 (void*)&guest_incoming,
                                 sizeof(struct goldfish_sync_batch_guestcmd));
        DPRINT("got: batchaddr 0x%llx cmd=%llu glsync=0x%llx thread=0x%llx timeline=0x%llx\n",
               (unsigned long long)(s->batch_guestcmd_addr),
               (unsigned long long)(guest_incoming.host_command),
               (unsigned long long)(guest_incoming.glsync_handle),
               (unsigned long long)(guest_incoming.thread_handle),
               (unsigned long long)(guest_incoming.guest_timeline_handle));
        switch (guest_incoming.host_command) {
        case SYNC_GUEST_CMD_TRIGGER_HOST_WAIT:
            DPRINT("exec SYNC_GUEST_CMD_TRIGGER_HOST_WAIT");
            service_ops->trigger_host_wait(guest_incoming.glsync_handle,
                                           guest_incoming.thread_handle,
                                           guest_incoming.guest_timeline_handle);
            break;
        case SYNC_GUEST_CMD_READY:
            DPRINT("exec SYNC_GUEST_CMD_READY");
            break;
        }
        break;
    // SYNC_REG_BATCH_(GUEST)COMMAND_ADDR(_HIGH) write:
    // Used to communicate the physical address on the guest where
    // we write all the info for commands.
    case SYNC_REG_BATCH_COMMAND_ADDR:
        s->batch_cmd_addr = val;
        break;
    case SYNC_REG_BATCH_COMMAND_ADDR_HIGH:
        s->batch_cmd_addr |= (uint64_t)(val << 32);
        DPRINT("Got batch command address. addr=0x%llx",
               s->batch_cmd_addr);
        break;
    case SYNC_REG_BATCH_GUESTCOMMAND_ADDR:
        s->batch_guestcmd_addr = val;
        break;
    case SYNC_REG_BATCH_GUESTCOMMAND_ADDR_HIGH:
        s->batch_guestcmd_addr |= (uint64_t)(val << 32);
        DPRINT("Got batch guestcommand address. addr=0x%llx",
               s->batch_guestcmd_addr);
        break;
    case SYNC_REG_INIT:
        goldfish_sync_reset_device(s);
        break;
    default:
        DPRINT("INVALID sync_write! reg=%u", offset);
        assert(false);
    }
}

static const MemoryRegionOps goldfish_sync_iomem_ops = {
    .read = goldfish_sync_read,
    .write = goldfish_sync_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void
save_pending_cmds(QEMUFile* file,
                  struct goldfish_sync_pending_cmd* cmd) {
    struct goldfish_sync_pending_cmd* curr = cmd;

    qemu_put_byte(file, curr != 0);

    while (curr) {
        qemu_put_be64(file, curr->handle);
        qemu_put_be64(file, curr->hostcmd_handle);
        qemu_put_be64(file, curr->cmd);
        qemu_put_be64(file, curr->time_arg);
        curr = curr->next;
        qemu_put_byte(file, curr != 0);
    }
}

// Returns the head of the list.
static struct goldfish_sync_pending_cmd*
load_pending_cmds(QEMUFile* file) {
    struct goldfish_sync_pending_cmd* res = 0;
    struct goldfish_sync_pending_cmd* curr;

    bool exists = qemu_get_byte(file);

    if (!exists) {
        return res;
    }

    res = goldfish_sync_new_cmd();
    curr = res;

    while (exists) {
        curr->handle = qemu_get_be64(file);
        curr->hostcmd_handle = qemu_get_be64(file);
        curr->cmd = qemu_get_be64(file);
        curr->time_arg = qemu_get_be64(file);
        exists = qemu_get_byte(file);
        if (exists) {
            curr->next = goldfish_sync_new_cmd();
            curr = curr->next;
        } else {
            curr->next = 0;
        }
    }

    return res;
}

// Save/load order:
// 1. Pipe
// 2. Sync
// (could be enforced by specification order in goldfish acpi dsdt, or
// in IRQ seq?)
static void goldfish_sync_save(QEMUFile* file, void* opaque) {
    struct goldfish_sync_state* s = opaque;

    qemu_put_be64(file, s->batch_cmd_addr);
    qemu_put_be64(file, s->batch_guestcmd_addr);

    save_pending_cmds(file, s->pending);

    service_ops->save(file);
}

static int goldfish_sync_load(QEMUFile* file, void* opaque, int version_id) {
    struct goldfish_sync_state* s = opaque;
    goldfish_sync_reset_device(s);

    s->batch_cmd_addr = qemu_get_be64(file);
    s->batch_guestcmd_addr = qemu_get_be64(file);

    s->pending = load_pending_cmds(file);
    s->first_pending_cmd = s->pending;

    // Do pending cmds after we restored the ones already
    // in the state struct.
    service_ops->load(file);

    if (s->pending) {
        qemu_set_irq(s->irq, 1);
    }

    return 0;
}

static SaveVMHandlers goldfish_sync_vmhandlers= {
    .save_state = goldfish_sync_save,
    .load_state = goldfish_sync_load
};

// goldfish_sync_realize is called on startup if the sync device is enabled.
// It does the work of setting up the I/O, command queue, and function pointers.
static void goldfish_sync_realize(DeviceState *dev, Error **errp) {
    DPRINT("enter");
    SysBusDevice* sbdev = SYS_BUS_DEVICE(dev);
    struct goldfish_sync_state* s = GOLDFISH_SYNC(dev);

    s_goldfish_sync_dev = s;

    s->batch_cmd_addr = 0;
    s->batch_guestcmd_addr = 0;
    s->pending = NULL;
    s->current = NULL;

    memory_region_init_io(&s->iomem, OBJECT(s),
                          &goldfish_sync_iomem_ops,
                          s,
                          "goldfish_sync",
                          0x2000);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    register_savevm_live(dev, "goldfish_sync", 0, 1, &goldfish_sync_vmhandlers,
                         s);
}

static Property goldfish_sync_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void goldfish_sync_class_init(ObjectClass *klass, void *data)
{
    DPRINT("enter");
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_sync_realize;
    dc->desc = "goldfish sync";
    dc->props = goldfish_sync_properties;
}

static const TypeInfo goldfish_sync_info = {
    .name          = TYPE_GOLDFISH_SYNC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct goldfish_sync_state),
    .class_init    = goldfish_sync_class_init,
};

static void goldfish_sync_register(void)
{
    DPRINT("enter");
    type_register_static(&goldfish_sync_info);
}

type_init(goldfish_sync_register);
