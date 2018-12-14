#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/acpi/goldfish_defs.h"
#include "hw/sysbus.h"
#include "hw/pci/pci.h"

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

/* Represents a continuous range of addresses and a flag if this block is
 * available
 */
struct address_block {
    uint64_t offset;
    uint64_t size : 63;
    uint64_t available : 1;
};

/* A dynamic array of address blocks, with the following invariant:
 * blocks[i].size > 0
 * blocks[i+1].offset = blocks[i].offset + blocks[i].size
 */
struct address_space_allocator {
    struct address_block *blocks;
    int size;
    int capacity;
};

#define BAD_OFFSET (~(uint64_t)0)
#define DEFAULT_GUEST_PAGE_SIZE 4096

/* Looks for the smallest (to reduce fragmentation) available block with size to
 * fit the requested amount and returns its index or -1 if none is available.
 */
int address_space_allocator_find_available_block(struct address_block *block,
                                                 int n_blocks,
                                                 uint64_t size_at_least)
{
    int index = -1;
    uint64_t size_at_index = 0;
    int i;

    g_assert(n_blocks >= 1);

    for (i = 0; i < n_blocks; ++i, ++block) {
        uint64_t this_size = block->size;
        g_assert(this_size > 0);

        if (this_size >= size_at_least && block->available &&
            (index < 0 || this_size < size_at_index)) {
            index = i;
            size_at_index = this_size;
        }
    }

    return index;
}

int address_space_allocator_grow_capacity(int old_capacity)
{
    g_assert(old_capacity >= 1);

    return old_capacity + old_capacity;
}

/* Inserts one more address block right after i'th (by borrowing i'th size) and
 * adjusts sizes:
 * pre:
 *   size > blocks[i].size
 *
 * post:
 *   * might reallocate allocator->blocks if there is no capacity to insert one
 *   * blocks[i].size -= size;
 *   * blocks[i+1].size = size;
 */
struct address_block *address_space_allocator_split_block(struct address_space_allocator *allocator,
                                                          int i,
                                                          uint64_t size)
{
    g_assert(allocator->capacity >= 1);
    g_assert(allocator->size >= 1);
    g_assert(allocator->size <= allocator->capacity);
    g_assert(i >= 0);
    g_assert(i < allocator->size);
    g_assert(size < allocator->blocks[i].size);

    if (allocator->size == allocator->capacity) {
        int new_capacity = address_space_allocator_grow_capacity(allocator->capacity);
        allocator->blocks = g_realloc(allocator->blocks,
                                      sizeof(struct address_block) * new_capacity);
        g_assert(allocator->blocks);
        allocator->capacity = new_capacity;
    }

    struct address_block *blocks = allocator->blocks;

    /*   size = 5, i = 1
     *   [ 0 | 1 |  2  |  3  | 4 ]  =>  [ 0 | 1 | new |  2  | 3 | 4 ]
     *         i  (i+1) (i+2)                 i  (i+1) (i+2)
     */
    memmove(&blocks[i + 2], &blocks[i + 1],
            sizeof(struct address_block) * (allocator->size - i - 1));

    struct address_block *to_borrow_from = &blocks[i];
    struct address_block *new_block = to_borrow_from + 1;

    uint64_t new_size = to_borrow_from->size - size;

    to_borrow_from->size = new_size;

    new_block->offset = to_borrow_from->offset + new_size;
    new_block->size = size;
    new_block->available = 1;

    ++allocator->size;

    return new_block;
}

/* Marks i'th block as available. If adjacent ((i-1) and (i+1)) blocks are also
 * available, it merges i'th block with them.
 * pre:
 *   i < allocator->size
 * post:
 *   i'th block is merged with adjacent ones if they are available, blocks that
 *   were merged from are removed. allocator->size is updated if blocks were
 *   removed.
 */
void address_space_allocator_release_block(struct address_space_allocator *allocator,
                                           int i)
{
    struct address_block *blocks = allocator->blocks;
    int before = i - 1;
    int after = i + 1;
    int size = allocator->size;

    g_assert(i >= 0);
    g_assert(i < size);

    blocks[i].available = 1;

    if (before >= 0 && blocks[before].available) {
        if (after < size && blocks[after].available) {
            // merge (before, i, after) into before
            blocks[before].size += (blocks[i].size + blocks[after].size);

            size -= 2;
            memmove(&blocks[i], &blocks[i + 2],
                sizeof(struct address_block) * (size - i));
            allocator->size = size;
        } else {
            // merge (before, i) into before
            blocks[before].size += blocks[i].size;

            --size;
            memmove(&blocks[i], &blocks[i + 1],
                sizeof(struct address_block) * (size - i));
            allocator->size = size;
        }
    } else if (after < size && blocks[after].available) {
        // merge (i, after) into i
        blocks[i].size += blocks[after].size;

        --size;
        memmove(&blocks[after], &blocks[after + 1],
            sizeof(struct address_block) * (size - after));
        allocator->size = size;
    }

}

/* Takes a size to allocate an address block and returns an offset where this
 * block is allocated. This block will not be available for other callers unless
 * it is explicitly deallocated (see address_space_allocator_deallocate below).
 */
static uint64_t address_space_allocator_allocate(struct address_space_allocator *allocator,
                                                 uint64_t size)
{
    int i = address_space_allocator_find_available_block(allocator->blocks,
                                                         allocator->size,
                                                         size);
    if (i < 0) {
        return BAD_OFFSET;
    } else {
        g_assert(i < allocator->size);

        struct address_block *block = &allocator->blocks[i];
        g_assert(block->size >= size);

        if (block->size > size) {
            block = address_space_allocator_split_block(allocator, i, size);
        }

        g_assert(block->size == size);
        block->available = 0;

        return block->offset;
    }
}

/* Takes an offset returned from address_space_allocator_allocate ealier
 * (see above) and marks this block as available for further allocation.
 */
static uint32_t address_space_allocator_deallocate(struct address_space_allocator *allocator,
                                                   uint64_t offset)
{
    struct address_block *block = allocator->blocks;
    int size = allocator->size;
    int i;

    g_assert(size >= 1);

    for (i = 0; i < size; ++i, ++block) {
        if (block->offset == offset) {
            if (block->available) {
                return EINVAL;
            } else {
                address_space_allocator_release_block(allocator, i);
                return 0;
            }
        }
    }

    return EINVAL;
}

/* Creates a seed block. */
static void address_space_allocator_init(struct address_space_allocator *allocator,
                                         uint64_t size,
                                         int initial_capacity)
{
    g_assert(initial_capacity >= 1);

    allocator->blocks = g_malloc0(sizeof(struct address_block) * initial_capacity);
    g_assert(allocator->blocks);

    struct address_block *block = allocator->blocks;

    block->offset = 0;
    block->size = size;
    block->available = 1;

    allocator->size = 1;
    allocator->capacity = initial_capacity;
}

/* At this point there should be no used blocks and all available blocks must
 * have been merged into one block.
 */
static void address_space_allocator_destroy(struct address_space_allocator *allocator)
{
    g_assert(allocator->size == 1);
    g_assert(allocator->capacity >= allocator->size);
    g_assert(allocator->blocks[0].available);
    g_free(allocator->blocks);
}

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

    if (offset == BAD_OFFSET) {
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
                     PCI_BASE_ADDRESS_SPACE_MEMORY,
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
