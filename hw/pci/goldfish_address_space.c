#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/acpi/goldfish_defs.h"
#include "hw/sysbus.h"
#include "hw/pci/pci.h"

#define ANDROID_EMU_ADDRESS_SPACE_ASSERT_FUNC g_assert
#define ANDROID_EMU_ADDRESS_SPACE_MALLOC0_FUNC g_malloc0
#define ANDROID_EMU_ADDRESS_SPACE_REALLOC_FUNC g_realloc
#define ANDROID_EMU_ADDRESS_SPACE_FREE_FUNC g_free

#include "android/base/address_space.h"

enum address_space_register_id {
	ADDRESS_SPACE_REGISTER_COMMAND = 0,
	ADDRESS_SPACE_REGISTER_STATUS = 4,
	ADDRESS_SPACE_REGISTER_GUEST_PAGE_SIZE = 8,
	ADDRESS_SPACE_REGISTER_BLOCK_SIZE_LOW = 12,
	ADDRESS_SPACE_REGISTER_BLOCK_SIZE_HIGH = 16,
	ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_LOW = 20,
	ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_HIGH = 24,
};

enum address_space_command_id {
	ADDRESS_SPACE_COMMAND_ALLOCATE_BLOCK = 1,
	ADDRESS_SPACE_COMMAND_DEALLOCATE_BLOCK = 2,
};

struct address_space_registers {
    /* the command register is not persistent */
    uint32_t status;
    uint32_t guest_page_size;
    uint32_t size_low;
    uint32_t size_high;
    uint32_t offset_low;
    uint32_t offset_high;
};

/* We need to cover 'size' bytes given at any offset with aligned pages. */
static uint64_t address_space_align_size(uint64_t page_size, uint64_t size)
{
    if (size == 0) {
        return 0;
    } else {
        return ((size + page_size * 2 - 2) / page_size) * page_size;
    }
}

struct address_space_state {
    PCIDevice parent_obj;
    SysBusDevice parent;
    MemoryRegion control_io;
    MemoryRegion area_mem;
    qemu_irq irq;

    struct address_space_registers registers;
    struct address_space_allocator allocator;
};

#define GOLDFISH_ADDRESS_SPACE(obj) \
    OBJECT_CHECK(struct address_space_state, (obj), \
                 GOLDFISH_ADDRESS_SPACE_NAME)

static uint64_t merge_u64(uint64_t low, uint64_t high)
{
    return low | (high << 32);
}

static uint32_t lower_32_bits(uint64_t x)
{
    return (uint32_t)x;
}

static uint32_t upper_32_bits(uint64_t x)
{
    return x >> 32;
}

static uint32_t address_space_allocate_block(struct address_space_state *state)
{
    struct address_space_registers *regs = &state->registers;
    uint64_t size = merge_u64(regs->size_low, regs->size_high);
    uint64_t aligned_size = address_space_align_size(regs->guest_page_size, size);
    if (!aligned_size) {
        return EINVAL;
    }

    uint64_t offset = address_space_allocator_allocate(&state->allocator,
                                                       aligned_size);

    regs->offset_low = lower_32_bits(offset);
    regs->offset_high = upper_32_bits(offset);
    regs->size_low = lower_32_bits(aligned_size);
    regs->size_high = upper_32_bits(aligned_size);

    if (offset == ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET) {
        return ENOMEM;
    } else {
        return 0;
    }
}

static uint32_t address_space_deallocate_block(struct address_space_state *state)
{
    struct address_space_registers *regs = &state->registers;
    uint64_t offset = merge_u64(regs->offset_low, regs->offset_high);

    return address_space_allocator_deallocate(&state->allocator, offset);
}

static uint32_t address_space_run_command(struct address_space_state *state,
                                          enum address_space_command_id cmd)
{
    switch (cmd) {
    case ADDRESS_SPACE_COMMAND_ALLOCATE_BLOCK:
        return address_space_allocate_block(state);

    case ADDRESS_SPACE_COMMAND_DEALLOCATE_BLOCK:
        return address_space_deallocate_block(state);
    }

    return ENOSYS;
}

static uint64_t address_space_control_read(void *opaque,
                                           hwaddr offset,
                                           unsigned size) {
    struct address_space_state *state = opaque;

    if (size != 4)
        goto bad_access;

    switch ((enum address_space_register_id)offset) {
    case ADDRESS_SPACE_REGISTER_COMMAND: /* write only */
        break;

    case ADDRESS_SPACE_REGISTER_STATUS:
        return state->registers.status;

    case ADDRESS_SPACE_REGISTER_GUEST_PAGE_SIZE:
        return state->registers.guest_page_size;

    case ADDRESS_SPACE_REGISTER_BLOCK_SIZE_LOW:
        return state->registers.size_low;

    case ADDRESS_SPACE_REGISTER_BLOCK_SIZE_HIGH:
        return state->registers.size_high;

    case ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_LOW:
        return state->registers.offset_low;

    case ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_HIGH:
        return state->registers.offset_high;
    }

bad_access:
    return 0;
}

static void address_space_control_write(void *opaque,
                                        hwaddr offset,
                                        uint64_t val,
                                        unsigned size) {
    struct address_space_state *state = opaque;

    if (size != 4)
        return;

    switch ((enum address_space_register_id)offset) {
    case ADDRESS_SPACE_REGISTER_COMMAND:
        state->registers.status =
            address_space_run_command(state, (enum address_space_command_id)val);
        qemu_irq_pulse(state->irq);
        break;

    case ADDRESS_SPACE_REGISTER_STATUS: /* read only */
        break;

    case ADDRESS_SPACE_REGISTER_GUEST_PAGE_SIZE:
        state->registers.guest_page_size = val;
        break;

    case ADDRESS_SPACE_REGISTER_BLOCK_SIZE_LOW:
        state->registers.size_low = val;
        break;

    case ADDRESS_SPACE_REGISTER_BLOCK_SIZE_HIGH:
        state->registers.size_high = val;
        break;

    case ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_LOW:
        state->registers.offset_low = val;
        break;

    case ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_HIGH:
        state->registers.offset_high = val;
        break;
    }
}

static const MemoryRegionOps address_space_control_ops = {
    .read = &address_space_control_read,
    .write = &address_space_control_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4
};

static void address_space_pci_reset(DeviceState *dev)
{
    struct address_space_state* address_space = GOLDFISH_ADDRESS_SPACE(dev);
}

static void address_space_pci_set_config(PCIDevice *dev)
{
    pci_set_word(dev->config + PCI_VENDOR_ID,
                 GOLDFISH_ADDRESS_SPACE_PCI_VENDOR_ID);
    pci_set_word(dev->config + PCI_DEVICE_ID,
                 GOLDFISH_ADDRESS_SPACE_PCI_DEVICE_ID);
    pci_config_set_revision(dev->config, GOLDFISH_ADDRESS_SPACE_PCI_REVISION);

    pci_set_byte(dev->config + PCI_INTERRUPT_LINE,
                 GOLDFISH_ADDRESS_SPACE_PCI_INTERRUPT_LINE);
    pci_config_set_interrupt_pin(dev->config,
                                 GOLDFISH_ADDRESS_SPACE_PCI_INTERRUPT_PIN);

    /* offsets of PCI BARs: */
    pci_set_long(dev->config + PCI_BASE_ADDRESS_0, 0);
    pci_set_long(dev->config + PCI_BASE_ADDRESS_1,
                 GOLDFISH_ADDRESS_SPACE_CONTROL_SIZE);
}

#define DEFAULT_GUEST_PAGE_SIZE 4096

static void address_space_pci_realize(PCIDevice *dev, Error **errp) {
    struct address_space_state* state = GOLDFISH_ADDRESS_SPACE(dev);

    address_space_pci_set_config(dev);

    memory_region_init_io(&state->control_io,
                          OBJECT(state),
                          &address_space_control_ops,
                          state,
                          GOLDFISH_ADDRESS_SPACE_CONTROL_NAME,
                          GOLDFISH_ADDRESS_SPACE_CONTROL_SIZE);
    pci_register_bar(dev,
                     GOLDFISH_ADDRESS_SPACE_CONTROL_BAR,
                     PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &state->control_io);

    memory_region_init_ram_user_backed(&state->area_mem,
                                       OBJECT(state),
                                       GOLDFISH_ADDRESS_SPACE_AREA_NAME,
                                       GOLDFISH_ADDRESS_SPACE_AREA_SIZE);
    pci_register_bar(dev,
                     GOLDFISH_ADDRESS_SPACE_AREA_BAR,
                     PCI_BASE_ADDRESS_SPACE_MEMORY |
                     PCI_BASE_ADDRESS_MEM_TYPE_64,
                     &state->area_mem);

    state->irq = pci_allocate_irq(dev);

    state->registers.guest_page_size = DEFAULT_GUEST_PAGE_SIZE;

    address_space_allocator_init(&state->allocator,
                                 GOLDFISH_ADDRESS_SPACE_AREA_SIZE,
                                 32);
}

static void address_space_pci_unrealize(PCIDevice* dev) {
    struct address_space_state* address_space = GOLDFISH_ADDRESS_SPACE(dev);
    uint64_t available;
    uint64_t allocated;

    address_space_allocator_destroy(&address_space->allocator);
}

static Property address_space_pci_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static const VMStateDescription vmstate_address_space_pci = {
    .name = GOLDFISH_ADDRESS_SPACE_NAME,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void address_space_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->desc = GOLDFISH_ADDRESS_SPACE_NAME;
    dc->hotpluggable = false;
    dc->reset = &address_space_pci_reset;
    dc->props = address_space_pci_properties;
    dc->vmsd = &vmstate_address_space_pci;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    PCIDeviceClass* pci = PCI_DEVICE_CLASS(klass);

    pci->realize = &address_space_pci_realize;
    pci->exit = &address_space_pci_unrealize;
    pci->vendor_id = GOLDFISH_ADDRESS_SPACE_PCI_VENDOR_ID;
    pci->device_id = GOLDFISH_ADDRESS_SPACE_PCI_DEVICE_ID;
    pci->revision = 0x00;
    pci->class_id = PCI_CLASS_OTHERS;
}

static const TypeInfo address_space_pci_info = {
    .name          = GOLDFISH_ADDRESS_SPACE_NAME,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(struct address_space_state),
    .class_init    = address_space_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void address_space_register(void) {
    type_register_static(&address_space_pci_info);
}

type_init(address_space_register);
