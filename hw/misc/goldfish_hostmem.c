/* Copyright (C) 2018 The Android Open Source Project
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
#include "hw/misc/trace.h"
#include "hw/misc/goldfish_hostmem.h"
#include "hw/pci/pci.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"

#include <assert.h>

// TODO(digit): Plug better logging for Android emulator.
#define DPRINT(fmt,...) fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

// Goldfish hostmem commands====================================================

// Batch command format
struct goldfish_hostmem_batch_cmd {
    uint64_t gpa;
    uint64_t id;
    uint64_t host_ptr;
    uint64_t size;
    uint64_t status_code;
    uint32_t cmd;
};

// This is a suffix of batch cmd.
// The purpose of the "pending_cmd" variant
// is so we can queue up several batch commands
// in the same IRQ level.
struct goldfish_hostmem_pending_cmd {
    uint64_t gpa;
    uint64_t id;
    uint64_t host_ptr;
    uint64_t size;
    uint64_t status_code;
    uint32_t cmd;
    struct goldfish_hostmem_pending_cmd* next;
};

// GOLDFISH HOSTMEM DEVICE
// The Goldfish hostmem driver is designed to map memory from the host
// into the guest.
// The purpose of the device/driver is to enable correct and lightweight
// mapped buffers in OpenGL / Vulkan.

// Each time the interrupt trips, the guest kernel is notified of changes
// in mapped host memory regions.

// The operations are:

#define CMD_HOSTMEM_READY            0
#define CMD_HOSTMEM_SET_PTR          1
#define CMD_HOSTMEM_UNSET_PTR        2

// The register layout is:

#define HOSTMEM_REG_BATCH_COMMAND                0x00 // host->guest batch commands
#define HOSTMEM_REG_BATCH_COMMAND_ADDR           0x08 // communicate physical address of host->guest batch commands
#define HOSTMEM_REG_BATCH_COMMAND_ADDR_HIGH      0x0c // 64-bit part
#define HOSTMEM_REG_INIT                         0x18 // to signal that the device has been detected by the kernel

typedef struct MSIVector {
    PCIDevice *pdev;
    int virq;
} MSIVector;

typedef struct goldfish_hostmem_state {
    PCIDevice parent_obj;

    SysBusDevice parent;
    MemoryRegion iomem;
    MemoryRegion iomem_port;
    qemu_irq irq;

    struct goldfish_hostmem_pending_cmd* pending;
    struct goldfish_hostmem_pending_cmd* first_pending_cmd;
    struct goldfish_hostmem_pending_cmd* current;

    uint64_t batch_cmd_addr;
    uint64_t batch_guestcmd_addr;

    hwaddr addr;
    uint64_t size;

    uint32_t vectors;
    MSIVector *msi_vectors;

    PCIDevice* pci_dev;

} goldfish_hostmem_state;

static struct goldfish_hostmem_state* s_goldfish_hostmem_dev;

static const GoldfishHostmemServiceOps* service_ops = NULL;

void goldfish_hostmem_set_service_ops(const GoldfishHostmemServiceOps *ops) {
    service_ops = ops;
}

// Command list operations======================================================

static struct goldfish_hostmem_pending_cmd*
goldfish_hostmem_new_cmd(void) {
    struct goldfish_hostmem_pending_cmd* res;
    res = g_malloc0(sizeof(struct goldfish_hostmem_pending_cmd));
    return res;
}

static void
goldfish_hostmem_free_cmd(struct goldfish_hostmem_pending_cmd* cmd) {
    g_free(cmd);
}

static void
goldfish_hostmem_push_cmd(struct goldfish_hostmem_state* state,
                       struct goldfish_hostmem_pending_cmd* cmd) {

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

static struct goldfish_hostmem_pending_cmd*
goldfish_hostmem_pop_first_cmd(struct goldfish_hostmem_state* state) {

    struct goldfish_hostmem_pending_cmd* popped =
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

static void goldfish_hostmem_clear_pending(struct goldfish_hostmem_state* state) {
    struct goldfish_hostmem_pending_cmd* curr =
        state->pending;

    while (curr) {
        struct goldfish_hostmem_pending_cmd* tmp = curr;
        curr = curr->next;
        goldfish_hostmem_free_cmd(tmp);
    }

    state->pending = NULL;
    state->first_pending_cmd = NULL;
}

static void goldfish_hostmem_reset_device(struct goldfish_hostmem_state* state) {
    goldfish_hostmem_clear_pending(state);
    qemu_set_irq(state->irq, 0);
}

// Callbacks and hw_funcs struct================================================

// goldfish_hostmem_send_command does the actual work of setting up
// the hostmem device registers, causing IRQ blip,
// and waiting for + reading back results from the guest
// (if appplicable)
void goldfish_hostmem_send_command(uint64_t gpa,
                                   uint64_t id,
                                   void* host_ptr,
                                   uint64_t size,
                                   uint64_t status_code,
                                   uint64_t cmd) {

    struct goldfish_hostmem_state* s;
    struct goldfish_hostmem_pending_cmd* to_send;

    s = s_goldfish_hostmem_dev;

    // DPRINT("incoming hostmem cmd. "
    //        "id=0x%llx "
    //        "host_ptr=%p "
    //        "size=%llu "
    //        "status_code=%llu "
    //        "cmd=%llu ",
    //        (unsigned long long)id,
    //        host_ptr,
    //        (unsigned long long)size,
    //        (unsigned long long)status_code,
    //        (unsigned long long)cmd);

    to_send = goldfish_hostmem_new_cmd();
    to_send->gpa = gpa;
    to_send->id = id;
    to_send->host_ptr = (uint64_t)(uintptr_t)host_ptr;
    to_send->size = size;
    to_send->status_code = status_code;
    to_send->cmd = cmd;

    goldfish_hostmem_push_cmd(s, to_send);

    msix_notify(s->pci_dev, 0);
    // qemu_set_irq(s->irq, 1);

    // DPRINT("Exit");
}

#define TYPE_GOLDFISH_HOSTMEM "goldfish_hostmem"
#define GOLDFISH_HOSTMEM(obj) \
    OBJECT_CHECK(goldfish_hostmem_state, (obj), TYPE_GOLDFISH_HOSTMEM)

static uint64_t
goldfish_hostmem_read(void *opaque,
                   hwaddr offset,
                   unsigned size) {
    DPRINT("opaque=%p offset=0x%llx sz=0x%x", opaque, (unsigned long long)offset, size);

    struct goldfish_hostmem_state* s = opaque;
    struct goldfish_hostmem_batch_cmd* curr_batch_cmd;

    switch (offset) {
    // HOSTMEM_REG_BATCH_COMMAND read:
    // This is triggered by the kernel whenever we are processing
    // any host->guest hostmem command, such as timeline increment.
    // The role of this part is to pass command info (in the struct
    // |goldfish_hostmem_batch_cmd|'s to the guest where the commands
    // are actually run.
    //
    // HOSTMEM_REG_BATCH_COMMAND is the first thing
    // to happen on a IRQ level raise, and will continue re-issuing
    // throughout IRQ active high, until all pending commands on the
    // host (the linked list) are processed. At that point, IRQ
    // is lowered.
    case HOSTMEM_REG_BATCH_COMMAND:
        s->current = goldfish_hostmem_pop_first_cmd(s);
        if (!s->current) {
            DPRINT("Out of pending commands. Lower IRQ.");
            struct goldfish_hostmem_batch_cmd quit_cmd = {
                .cmd = 0,
            };
            cpu_physical_memory_write(s->batch_cmd_addr,
                                      (void*)&quit_cmd,
                                      sizeof(struct goldfish_hostmem_batch_cmd));
            qemu_set_irq(s->irq, 0);
            return 0;
        } else {
            // goldfish_hostmem_batch_cmd is a prefix of
            // goldfish_hostmem_pending_cmd, so we can avoid a copy.
            curr_batch_cmd = (struct goldfish_hostmem_batch_cmd*)(s->current);
            cpu_physical_memory_write(s->batch_cmd_addr,
                                      (void*)curr_batch_cmd,
                                      sizeof(struct goldfish_hostmem_batch_cmd));
            goldfish_hostmem_free_cmd(s->current);
            s->current = NULL;
        }
        DPRINT("done HOSTMEM_REG_BATCH_COMMAND. returning result val?");
        return 0;
    // HOSTMEM_REG_BATCH_COMMAND_ADDR(_HIGH) read:
    // Used solely to verify that the |s->batch_cmd_addr| on the host
    // is the same physical address as the corresponding one
    // on the guest.
    case HOSTMEM_REG_BATCH_COMMAND_ADDR:
        return (uint64_t)(uint32_t)(s->batch_cmd_addr);
    case HOSTMEM_REG_BATCH_COMMAND_ADDR_HIGH:
        return (uint64_t)(uint32_t)(s->batch_cmd_addr >> 32);
    default:
        DPRINT("INVALID hostmem_read! reg=%llu", (unsigned long long)offset);
        assert(false);
        return 0;
    }
    return 0;
}

static void
goldfish_hostmem_write(void *opaque,
                       hwaddr offset,
                       uint64_t val,
                       unsigned size) {
    DPRINT("opaque=%p "
           "offset=0x%llx "
           "sz=0x%x "
           "val=%" PRIu64,
           opaque, (unsigned long long)offset, size, val);

    struct goldfish_hostmem_state* s = opaque;

    switch (offset) {
    // HOSTMEM_REG_BATCH_COMMAND write: Used to send acknowledgements
    // to host->guest commands. Some commands that can be issued from
    // the host, such as |goldfish_hostmem_create_timeline|, require a
    // return value of the timeline pointer itself. Not all commands
    // use this; for example, we never acknowledge any
    // |goldfish_hostmem_timeline_inc| commands because that would
    // decrease performance.
    case HOSTMEM_REG_BATCH_COMMAND:
        DPRINT("write HOSTMEM_REG_BATCH_COMMAND. obtaining batch cmd vals.");
        struct goldfish_hostmem_batch_cmd incoming = {
            .id = 0, .host_ptr = 0, .size = 0, .status_code = 0, .cmd = 0,
        };
        cpu_physical_memory_read(s->batch_cmd_addr,
                                 (void*)&incoming,
                                 sizeof(struct goldfish_hostmem_batch_cmd));
        break;
    case HOSTMEM_REG_BATCH_COMMAND_ADDR:
        s->batch_cmd_addr = val;
        break;
    case HOSTMEM_REG_BATCH_COMMAND_ADDR_HIGH:
        s->batch_cmd_addr |= (uint64_t)(val << 32);
        DPRINT("Got batch command address. addr=0x%llx",
               (unsigned long long)(s->batch_cmd_addr));
        break;
    case HOSTMEM_REG_INIT:
        goldfish_hostmem_reset_device(s);
        break;
    default:
        DPRINT("INVALID hostmem_write! reg=%llu", (unsigned long long)offset);
        assert(false);
    }
}

static const MemoryRegionOps goldfish_hostmem_iomem_ops = {
    .read = goldfish_hostmem_read,
    .write = goldfish_hostmem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static const MemoryRegionOps goldfish_hostmem_iomem_port_ops = {
    .read = goldfish_hostmem_read,
    .write = goldfish_hostmem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void
save_pending_cmds(QEMUFile* file,
                  struct goldfish_hostmem_pending_cmd* cmd) {
    struct goldfish_hostmem_pending_cmd* curr = cmd;

    qemu_put_byte(file, curr != 0);

    while (curr) {
        qemu_put_be64(file, curr->gpa);
        qemu_put_be64(file, curr->id);
        qemu_put_be64(file, (uint64_t)(uintptr_t)curr->host_ptr);
        qemu_put_be64(file, curr->size);
        qemu_put_be64(file, curr->status_code);
        qemu_put_be64(file, curr->cmd);
        curr = curr->next;
        qemu_put_byte(file, curr != 0);
    }
}

// Returns the head of the list.
static struct goldfish_hostmem_pending_cmd*
load_pending_cmds(QEMUFile* file) {
    struct goldfish_hostmem_pending_cmd* res = 0;
    struct goldfish_hostmem_pending_cmd* curr;

    bool exists = qemu_get_byte(file);

    if (!exists) {
        return res;
    }

    res = goldfish_hostmem_new_cmd();
    curr = res;

    while (exists) {
        curr->gpa = qemu_get_be64(file);
        curr->id = qemu_get_be64(file);
        curr->host_ptr = qemu_get_be64(file);
        curr->size = qemu_get_be64(file);
        curr->status_code = qemu_get_be64(file);
        curr->cmd = qemu_get_be64(file);
        exists = qemu_get_byte(file);
        if (exists) {
            curr->next = goldfish_hostmem_new_cmd();
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
static void goldfish_hostmem_save(QEMUFile* file, void* opaque) {
    struct goldfish_hostmem_state* s = opaque;

    qemu_put_be64(file, s->batch_cmd_addr);
    qemu_put_be64(file, s->batch_guestcmd_addr);

    save_pending_cmds(file, s->pending);

    service_ops->save(file);
}

static int goldfish_hostmem_load(QEMUFile* file, void* opaque, int version_id) {
    struct goldfish_hostmem_state* s = opaque;
    goldfish_hostmem_reset_device(s);

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

static void goldfish_hostmem_realize(PCIDevice *pci_dev, Error **errp) {
    fprintf(stderr, "%s: call\n", __func__);

    struct goldfish_hostmem_state* goldfish_state = GOLDFISH_HOSTMEM(pci_dev);
    uint8_t* pci_conf;
    char* name;
    int r;
    int i;

    pci_conf = pci_dev->config;
    pci_conf[PCI_INTERRUPT_LINE] = 0x0b;
    pci_conf[PCI_INTERRUPT_PIN] = 0x01;

    pci_dev->romfile = 0;

    memory_region_init_io(&goldfish_state->iomem,
                          OBJECT(goldfish_state),
                          &goldfish_hostmem_iomem_ops,
                          goldfish_state,
                          "goldfish_hostmem_mmio",
                          0x1000);

    pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &goldfish_state->iomem);

    goldfish_state->vectors = 4;

    if (msix_init_exclusive_bar(pci_dev, goldfish_state->vectors, 1, errp)) {
        DPRINT("could not initialize MSIX bar");
    }

    DPRINT("MSIX initialized with %d vectors", goldfish_state->vectors);

    goldfish_state->msi_vectors = g_malloc0(goldfish_state->vectors * sizeof(MSIVector));

    for (i = 0; i < goldfish_state->vectors; i++) {
        msix_vector_use(pci_dev, i);
    }

    s_goldfish_hostmem_dev = goldfish_state;

    goldfish_state->pending = NULL;
    goldfish_state->current = NULL;

    goldfish_state->pci_dev = pci_dev;

    // register_savevm(
    //     dev, "goldfish_hostmem", 0, 1,
    //     goldfish_hostmem_save, goldfish_hostmem_load, s);
}

static void goldfish_hostmem_unrealize(PCIDevice* dev) {
    fprintf(stderr, "%s: call\n", __func__);
    struct goldfish_hostmem_state* s = GOLDFISH_HOSTMEM(dev);

    int i;
}

static void goldfish_hostmem_class_init(ObjectClass *klass, void *data)
{
    fprintf(stderr, "%s: call\n", __func__);

    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass* k = PCI_DEVICE_CLASS(klass);

    k->realize = goldfish_hostmem_realize;
    k->exit = goldfish_hostmem_unrealize;
    k->vendor_id = 0x607d;
    k->device_id = 0xf153;
    k->revision = 0x00;
    k->class_id = PCI_CLASS_OTHERS;

    dc->desc = "goldfish_hostmem";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo goldfish_hostmem_info = {
    .name          = TYPE_GOLDFISH_HOSTMEM,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(struct goldfish_hostmem_state),
    .class_init    = goldfish_hostmem_class_init,
};

static void goldfish_hostmem_register(void)
{
    DPRINT("enter");
    type_register_static(&goldfish_hostmem_info);
}

type_init(goldfish_hostmem_register);
