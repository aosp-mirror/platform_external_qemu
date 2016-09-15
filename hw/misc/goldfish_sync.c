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

#include "android/emulation/goldfish_sync.h"
#include "android/utils/debug.h"

#include "hw/hw.h"
#include "hw/sysbus.h"
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "qemu/main-loop.h"
#include "trace.h"

#ifndef max
#define max(x, y) ((x) < (y) ? y : x)
#endif

#ifndef min
#define min(x, y) ((x) > (y) ? y : x)
#endif

#define DEBUG_LOG_ALL_DEVICE_OPS 0

#if DEBUG_LOG_ALL_DEVICE_OPS
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(goldfishsync)) VERBOSE_ENABLE(goldfishsync); \
    VERBOSE_TID_FUNCTION_DPRINT(goldfishsync, __VA_ARGS__); } while(0)
#else
#define DPRINT(...)
#endif

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

// goldfish_sync_ops contains all the host-side operations that we want
// triggered along with certain goldfish sync device commands.
// For example, trigger_wait will run a eglClientWaitSyncKHR
// on the host in a particular sync thread.
struct goldfish_sync_ops {
    // Wait for the specified glsync object glsync_ptr
    // in thread thread_ptr.
    void (*trigger_wait)(uint64_t glsync_ptr,
                         uint64_t thread_ptr,
                         uint64_t timeline);
};

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

// Command list operations======================================================

static struct goldfish_sync_pending_cmd*
goldfish_sync_new_cmd() {
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

// goldfish_sync_host_signal does the actual work of setting up
// the sync device registers, causing IRQ blip,
// and waiting for + reading back results from the guest
// (if appplicable)
static void goldfish_sync_host_signal(
        uint32_t cmd,
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

// This sets |trigger_fn| as the function to call
// when guest asks the host to trigger a wait.
void goldfish_sync_register_trigger_wait_impl(
        trigger_wait_fn_t trigger_fn) {

    DPRINT("Enter");

    struct goldfish_sync_state* s = s_goldfish_sync_dev;

    if (!s->ops) {
        s->ops =
            (struct goldfish_sync_ops*)
            (malloc(sizeof(struct goldfish_sync_ops)));
    }

    s->ops->trigger_wait = trigger_fn;

    DPRINT("Exit");
}

static GoldfishSyncDeviceInterface qemu2_goldfish_sync_hw_funcs = {
    .doHostCommand = goldfish_sync_host_signal,
    .registerTriggerWait = goldfish_sync_register_trigger_wait_impl,
};

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
                   "cmd=%u handle=0x%llx time_arg=%u hostcmd_handle=0x%llx",
                   curr_batch_cmd->cmd,
                   curr_batch_cmd->handle,
                   curr_batch_cmd->time_arg,
                   curr_batch_cmd->hostcmd_handle);
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
        goldfish_sync_receive_hostcmd_result(
               incoming.cmd,
               incoming.handle,
               incoming.time_arg,
               incoming.hostcmd_handle);
        break;
    // SYNC_REG_BATCH_GUESTCOMMAND write: Used to send
    // guest->host commands. Currently, the only guest->host command
    // that matters is CMD_TRIGGER_HOST_WAIT, which is used
    // to cause a OpenGL client wait on the host GPU/CPU.
    case SYNC_REG_BATCH_GUESTCOMMAND:
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
        DPRINT("got: cmd=%llu glsync=0x%llx thread=0x%llx timeline=0x%llx",
                guest_incoming.host_command,
                guest_incoming.glsync_handle,
                guest_incoming.thread_handle,
                guest_incoming.guest_timeline_handle);
        switch (guest_incoming.host_command) {
        case CMD_TRIGGER_HOST_WAIT:
            DPRINT("exec CMD_TRIGGER_HOST_WAIT");
            s->ops->trigger_wait(guest_incoming.glsync_handle,
                                 guest_incoming.thread_handle,
                                 guest_incoming.guest_timeline_handle);
            break;
        case CMD_SYNC_READY:
            DPRINT("exec CMD_SYNC_READY");
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

// goldfish_sync_realize is called on startup if the sync device is enabled.
// It does the work of setting up the I/O, command queue, and function pointers.
static void goldfish_sync_realize(DeviceState *dev, Error **errp) {
    DPRINT("enter");
    SysBusDevice* sbdev = SYS_BUS_DEVICE(dev);
    struct goldfish_sync_state* s = GOLDFISH_SYNC(dev);

    s_goldfish_sync_dev = s;

    s->pending = NULL;
    s->current = NULL;

    memory_region_init_io(&s->iomem, OBJECT(s),
                          &goldfish_sync_iomem_ops,
                          s,
                          "goldfish_sync",
                          0x2000);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    goldfish_sync_set_hw_funcs(&qemu2_goldfish_sync_hw_funcs);
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
