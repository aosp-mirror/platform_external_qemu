#include "qemu/abort.h"
#include "qemu/osdep.h"
#include "hw/hw.h"
#include "hw/acpi/goldfish_defs.h"
#include "hw/sysbus.h"
#include "hw/pci/pci.h"
#include "hw/pci/goldfish_address_space.h"
#include "sysemu/sysemu.h"

static int null_address_space_memory_state_load(QEMUFile *file) {
    (void)file;
    return 0;
}

static int null_address_space_memory_state_save(QEMUFile *file) {
    (void)file;
    return 0;
}

static const GoldfishAddressSpaceOps goldfish_address_space_null_ops = {
    .load = null_address_space_memory_state_load,
    .save = null_address_space_memory_state_save,
};

static const GoldfishAddressSpaceOps *s_goldfish_address_space_ops =
    &goldfish_address_space_null_ops;

void goldfish_address_space_set_service_ops(const GoldfishAddressSpaceOps *ops) {
    s_goldfish_address_space_ops = ops ? ops : &goldfish_address_space_null_ops;
}

#define ANDROID_EMU_ADDRESS_SPACE_ASSERT_FUNC g_assert
#define ANDROID_EMU_ADDRESS_SPACE_MALLOC0_FUNC g_malloc0
#define ANDROID_EMU_ADDRESS_SPACE_REALLOC_FUNC g_realloc
#define ANDROID_EMU_ADDRESS_SPACE_FREE_FUNC g_free

#define AS_DEBUG 0

#if AS_DEBUG
#define AS_DPRINT(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define AS_DPRINT(fmt,...)
#endif


#ifdef CONFIG_ANDROID

#include "android/base/address_space.h"

#else

/* Represents a continuous range of addresses and a flag if this block is
 * available
 */
struct address_block {
    uint64_t offset;
    union {
        uint64_t size_available;  /* VMSTATE_x does not support bit fields */
        struct {
            uint64_t size : 63;
            uint64_t available : 1;
        };
    };
};

/* A dynamic array of address blocks, with the following invariant:
 * blocks[i].size > 0
 * blocks[i+1].offset = blocks[i].offset + blocks[i].size
 */
struct address_space_allocator {
    struct address_block *blocks;
    int32_t size;
    int32_t capacity;
    uint64_t total_bytes;
};

#define ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET (~(uint64_t)0)
#define ANDROID_EMU_ADDRESS_SPACE_DEFAULT_PAGE_SIZE 4096

/* The assert function to abort if something goes wrong. */
static void address_space_assert(bool condition) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_ASSERT_FUNC
    ANDROID_EMU_ADDRESS_SPACE_ASSERT_FUNC(condition);
#else
    assert(condition);
#endif
}

void* address_space_malloc0(size_t size) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_MALLOC0_FUNC
    return ANDROID_EMU_ADDRESS_SPACE_MALLOC0_FUNC(size);
#else
    void* res = malloc(size);
    memset(res, 0, size);
    return res;
#endif
}

void* address_space_realloc(void* ptr, size_t size) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_REALLOC_FUNC
    return ANDROID_EMU_ADDRESS_SPACE_REALLOC_FUNC(ptr, size);
#else
    void* res = realloc(ptr, size);
    return res;
#endif
}

void address_space_free(void* ptr) {
#ifdef ANDROID_EMU_ADDRESS_SPACE_FREE_FUNC
    return ANDROID_EMU_ADDRESS_SPACE_FREE_FUNC(ptr);
#else
    free(ptr);
#endif
}

/* Looks for the smallest (to reduce fragmentation) available block with size to
 * fit the requested amount and returns its index or -1 if none is available.
 */
static int address_space_allocator_find_available_block(
    struct address_block *block,
    int n_blocks,
    uint64_t size_at_least)
{
    int index = -1;
    uint64_t size_at_index = 0;
    int i;

    address_space_assert(n_blocks >= 1);

    for (i = 0; i < n_blocks; ++i, ++block) {
        uint64_t this_size = block->size;
        address_space_assert(this_size > 0);

        if (this_size >= size_at_least && block->available &&
            (index < 0 || this_size < size_at_index)) {
            index = i;
            size_at_index = this_size;
        }
    }

    return index;
}

static int
address_space_allocator_grow_capacity(int old_capacity) {
    address_space_assert(old_capacity >= 1);

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
static struct address_block*
address_space_allocator_split_block(
    struct address_space_allocator *allocator,
    int i,
    uint64_t size)
{
    address_space_assert(allocator->capacity >= 1);
    address_space_assert(allocator->size >= 1);
    address_space_assert(allocator->size <= allocator->capacity);
    address_space_assert(i >= 0);
    address_space_assert(i < allocator->size);
    address_space_assert(size < allocator->blocks[i].size);

    if (allocator->size == allocator->capacity) {
        int new_capacity = address_space_allocator_grow_capacity(allocator->capacity);
        allocator->blocks =
            (struct address_block*)
            address_space_realloc(
                allocator->blocks,
                sizeof(struct address_block) * new_capacity);
        address_space_assert(allocator->blocks);
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
static void address_space_allocator_release_block(
    struct address_space_allocator *allocator,
    int i)
{
    struct address_block *blocks = allocator->blocks;
    int before = i - 1;
    int after = i + 1;
    int size = allocator->size;

    address_space_assert(i >= 0);
    address_space_assert(i < size);

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
static uint64_t address_space_allocator_allocate(
    struct address_space_allocator *allocator,
    uint64_t size)
{
    int i = address_space_allocator_find_available_block(allocator->blocks,
                                                         allocator->size,
                                                         size);
    if (i < 0) {
        return ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET;
    } else {
        address_space_assert(i < allocator->size);

        struct address_block *block = &allocator->blocks[i];
        address_space_assert(block->size >= size);

        if (block->size > size) {
            block = address_space_allocator_split_block(allocator, i, size);
        }

        address_space_assert(block->size == size);
        block->available = 0;

        return block->offset;
    }
}

/* Takes an offset returned from address_space_allocator_allocate ealier
 * (see above) and marks this block as available for further allocation.
 */
static uint32_t address_space_allocator_deallocate(
    struct address_space_allocator *allocator,
    uint64_t offset)
{
    struct address_block *block = allocator->blocks;
    int size = allocator->size;
    int i;

    address_space_assert(size >= 1);

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
static void address_space_allocator_init(
    struct address_space_allocator *allocator,
    uint64_t size,
    int initial_capacity)
{
    address_space_assert(initial_capacity >= 1);

    allocator->blocks =
        (struct address_block*)
        address_space_malloc0(sizeof(struct address_block) * initial_capacity);
    address_space_assert(allocator->blocks);

    struct address_block *block = allocator->blocks;

    block->offset = 0;
    block->size = size;
    block->available = 1;

    allocator->size = 1;
    allocator->capacity = initial_capacity;
    allocator->total_bytes = size;
}

/* At this point there should be no used blocks and all available blocks must
 * have been merged into one block.
 */
static void address_space_allocator_destroy(
    struct address_space_allocator *allocator)
{
    address_space_assert(allocator->size == 1);
    address_space_assert(allocator->capacity >= allocator->size);
    address_space_assert(allocator->blocks[0].available);
    address_space_free(allocator->blocks);
}

/* Resets the state of the allocator to the initial state without
 * performing any dynamic memory management. */
static void address_space_allocator_reset(
    struct address_space_allocator *allocator)
{
    address_space_assert(allocator->size >= 1);

    allocator->size = 1;

    struct address_block* block = allocator->blocks;
    block->offset = 0;
    block->size = allocator->total_bytes;
    block->available = 1;
}

#endif

struct goldfish_address_space_ping {
	uint64_t offset;
	uint64_t size;
	uint64_t metadata;
	uint32_t version;
	uint32_t wait_fd;
    uint32_t wait_flags;
    uint32_t direction;
};

enum address_space_register_id {
	ADDRESS_SPACE_REGISTER_COMMAND = 0,
	ADDRESS_SPACE_REGISTER_STATUS = 4,
	ADDRESS_SPACE_REGISTER_GUEST_PAGE_SIZE = 8,
	ADDRESS_SPACE_REGISTER_BLOCK_SIZE_LOW = 12,
	ADDRESS_SPACE_REGISTER_BLOCK_SIZE_HIGH = 16,
	ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_LOW = 20,
	ADDRESS_SPACE_REGISTER_BLOCK_OFFSET_HIGH = 24,
    ADDRESS_SPACE_REGISTER_PING = 28,
    ADDRESS_SPACE_REGISTER_PING_INFO_ADDR_LOW = 32,
    ADDRESS_SPACE_REGISTER_PING_INFO_ADDR_HIGH = 36,
    ADDRESS_SPACE_REGISTER_HANDLE = 40,
    ADDRESS_SPACE_REGISTER_PHYS_START_LOW = 44,
    ADDRESS_SPACE_REGISTER_PHYS_START_HIGH = 48,
};

enum address_space_command_id {
	ADDRESS_SPACE_COMMAND_ALLOCATE_BLOCK = 1,
	ADDRESS_SPACE_COMMAND_DEALLOCATE_BLOCK = 2,
    ADDRESS_SPACE_COMMAND_GEN_HANDLE = 3,
    ADDRESS_SPACE_COMMAND_DESTROY_HANDLE = 4,
    ADDRESS_SPACE_COMMAND_TELL_PING_INFO_ADDR = 5,
};

struct address_space_registers {
    /* the command register is not persistent */
    uint32_t status;
    uint32_t guest_page_size;
    uint32_t size_low;
    uint32_t size_high;
    uint32_t offset_low;
    uint32_t offset_high;
    uint32_t ping; // unused
    uint32_t ping_info_addr_low;
    uint32_t ping_info_addr_high;
    uint32_t handle;
    uint32_t phys_start_low;
    uint32_t phys_start_high;
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
    QemuMutex mutex;

    struct address_space_registers registers;
    struct address_space_allocator allocator;

    // Starting physical address, determined by the kernel
    uint64_t phys_addr_start;
};

static struct address_space_state* s_current_state = 0;

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

    uint64_t physStart = goldfish_address_space_get_phys_addr_start_locked();

    if (physStart) {
        qemu_get_address_space_device_control_ops()->run_deallocation_callbacks(
                physStart + offset);
    }
    return address_space_allocator_deallocate(&state->allocator, offset);
}

static uint32_t address_space_run_command(struct address_space_state *state,
                                          enum address_space_command_id cmd)
{
    switch (cmd) {
    case ADDRESS_SPACE_COMMAND_ALLOCATE_BLOCK:
        AS_DPRINT("allocate block");
        return address_space_allocate_block(state);
    case ADDRESS_SPACE_COMMAND_DEALLOCATE_BLOCK:
        AS_DPRINT("deallocate block");
        return address_space_deallocate_block(state);
    case ADDRESS_SPACE_COMMAND_GEN_HANDLE:
        AS_DPRINT("gen handle");
        state->registers.handle =
            qemu_get_address_space_device_control_ops()->gen_handle();
        break;
    case ADDRESS_SPACE_COMMAND_DESTROY_HANDLE:
        AS_DPRINT("destroy handle");
        qemu_get_address_space_device_control_ops()->destroy_handle(
            state->registers.handle);
        break;
    case ADDRESS_SPACE_COMMAND_TELL_PING_INFO_ADDR:
        AS_DPRINT("tell ping info addr handle %u high 0x%x low 0x%x",
                  state->registers.handle,
                  state->registers.ping_info_addr_high,
                  state->registers.ping_info_addr_low);
        {
            uint64_t high =
                ((uint64_t)state->registers.ping_info_addr_high) << 32ULL;
            uint64_t low =
                ((uint64_t)state->registers.ping_info_addr_low);
            uint64_t phys = high | low;
            AS_DPRINT("tell ping info: ping info phys: 0x%llxn", (unsigned long long)phys);
            qemu_get_address_space_device_control_ops()->tell_ping_info(
                state->registers.handle, phys);
        }
        break;
    }

    return ENOSYS;
}

static uint64_t address_space_control_read_locked(struct address_space_state* state,
                                                  hwaddr offset,
                                                  unsigned size) {
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

    case ADDRESS_SPACE_REGISTER_PING:
        fprintf(stderr, "%s: warning: kernel tried to read PING register!\n", __func__);
        return state->registers.ping;

    case ADDRESS_SPACE_REGISTER_PING_INFO_ADDR_LOW:
        AS_DPRINT("read ping info addr low 0x%x", state->registers.ping_info_addr_low);
        return state->registers.ping_info_addr_low;
    case ADDRESS_SPACE_REGISTER_PING_INFO_ADDR_HIGH:
        AS_DPRINT("read ping info addr high 0x%x", state->registers.ping_info_addr_high);
        return state->registers.ping_info_addr_high;
    case ADDRESS_SPACE_REGISTER_HANDLE:
        AS_DPRINT("read handle 0x%x", state->registers.handle);
        return state->registers.handle;
    case ADDRESS_SPACE_REGISTER_PHYS_START_LOW:
        AS_DPRINT("read low bits of phys start: 0x%x", state->registers.phys_start_low);
        return state->registers.phys_start_low;
    case ADDRESS_SPACE_REGISTER_PHYS_START_HIGH:
        AS_DPRINT("read high bits of phys start: 0x%x", state->registers.phys_start_high);
        return state->registers.phys_start_high;
    }

    /* Should not happen */
    fprintf(stderr, "%s:%d warning: unknown state!\n", __func__, __LINE__);
    assert(false);
    return -1;
}

static void address_space_control_write_locked(void *opaque,
                                               hwaddr offset,
                                               uint64_t val,
                                               unsigned size) {
    struct address_space_state *state = opaque;

    if (size != 4) {
        return;
    }

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

    case ADDRESS_SPACE_REGISTER_PING:
        AS_DPRINT("ping for handle: %u", (uint32_t)val);
        qemu_get_address_space_device_control_ops()->ping(val);
        break;

    case ADDRESS_SPACE_REGISTER_PING_INFO_ADDR_LOW:
        AS_DPRINT("write ping info addr low 0x%x", (uint32_t)val);
        state->registers.ping_info_addr_low = (uint32_t)val;
        break;
    case ADDRESS_SPACE_REGISTER_PING_INFO_ADDR_HIGH:
        AS_DPRINT("write ping info addr high 0x%x", (uint32_t)val);
        state->registers.ping_info_addr_high = (uint32_t)val;
        break;
    case ADDRESS_SPACE_REGISTER_HANDLE:
        AS_DPRINT("write handle 0x%x", (uint32_t)val);
        state->registers.handle = (uint32_t)val;
        break;
    case ADDRESS_SPACE_REGISTER_PHYS_START_LOW:
        AS_DPRINT("write phys start low 0x%x", (uint32_t)val);
        state->registers.phys_start_low = (uint32_t)val;
        break;
    case ADDRESS_SPACE_REGISTER_PHYS_START_HIGH:
        AS_DPRINT("write phys start high 0x%x", (uint32_t)val);
        state->registers.phys_start_high = (uint32_t)val;
        break;

    }
}

static uint64_t address_space_control_read(void *opaque,
                                           hwaddr offset,
                                           unsigned size) {
    struct address_space_state *state = opaque;
    uint64_t res;

    if (size != 4) {
        goto bad_access;
    }

    qemu_mutex_lock(&state->mutex);
    res = address_space_control_read_locked(state, offset, size);
    qemu_mutex_unlock(&state->mutex);

    return res;

bad_access:
    return 0;
}

static void address_space_control_write(void *opaque,
                                        hwaddr offset,
                                        uint64_t val,
                                        unsigned size) {
    struct address_space_state *state = opaque;

    if (size != 4) {
        return;
    }

    qemu_mutex_lock(&state->mutex);
    address_space_control_write_locked(state, offset, val, size);
    qemu_mutex_unlock(&state->mutex);
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
    AS_DPRINT("reset device");
    struct address_space_state* address_space = GOLDFISH_ADDRESS_SPACE(dev);
}

static void address_space_pci_set_config(PCIDevice *dev)
{
    AS_DPRINT("set pci config");
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
    AS_DPRINT("realize");

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

    qemu_mutex_init(&state->mutex);

    state->registers.guest_page_size = DEFAULT_GUEST_PAGE_SIZE;

    address_space_allocator_init(&state->allocator,
                                 GOLDFISH_ADDRESS_SPACE_AREA_SIZE,
                                 32);

    if (s_current_state) {
        qemu_abort("FATAL: Two address space devices created\n");
    }

    s_current_state = state;
}

static void address_space_pci_unrealize(PCIDevice* dev) {
    struct address_space_state* address_space = GOLDFISH_ADDRESS_SPACE(dev);
    uint64_t available;
    uint64_t allocated;

    s_current_state = 0;
    address_space_allocator_destroy(&address_space->allocator);
    qemu_mutex_destroy(&address_space->mutex);
}

static Property address_space_pci_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static int
vmstate_address_space_memory_state_load(QEMUFile *f,
                                        void *opaque,
                                        size_t size,
                                        VMStateField *field) {
    return s_goldfish_address_space_ops->load(f);
}

static int
vmstate_address_space_memory_state_save(QEMUFile *f,
                                        void *opaque,
                                        size_t size,
                                        VMStateField *field,
                                        QJSON *vmdesc) {
    return s_goldfish_address_space_ops->save(f);
}

#define VMSTATE_USER(_name, _version, _info) { \
    .name         = _name,                                            \
    .version_id   = (_version),                                       \
    .size         = 0,                                                \
    .info         = &(_info),                                         \
    .flags        = VMS_SINGLE,                                       \
    .offset       = 0,                                                \
}

const VMStateInfo vmstate_info_address_space_memory_state = {
    .name = "vmstate_info_address_space_memory_state",
    .get = &vmstate_address_space_memory_state_load,
    .put = &vmstate_address_space_memory_state_save,
};

static const VMStateDescription vmstate_address_space_registers = {
    .name = "address_space_registers",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(status,              struct address_space_registers),
        VMSTATE_UINT32(guest_page_size,     struct address_space_registers),
        VMSTATE_UINT32(size_low,            struct address_space_registers),
        VMSTATE_UINT32(size_high,           struct address_space_registers),
        VMSTATE_UINT32(offset_low,          struct address_space_registers),
        VMSTATE_UINT32(offset_high,         struct address_space_registers),
        VMSTATE_UINT32(ping,                struct address_space_registers),
        VMSTATE_UINT32(ping_info_addr_low,  struct address_space_registers),
        VMSTATE_UINT32(ping_info_addr_high, struct address_space_registers),
        VMSTATE_UINT32(handle,              struct address_space_registers),
        VMSTATE_UINT32(phys_start_low,      struct address_space_registers),
        VMSTATE_UINT32(phys_start_high,     struct address_space_registers),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_address_block = {
    .name = "address_block",
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(offset,         struct address_block),
        VMSTATE_UINT64(size_available, struct address_block),
        VMSTATE_END_OF_LIST()
    }
};

static int vmstate_address_space_allocator_pre_load(void *opaque) {
    struct address_space_allocator *allocator = opaque;

    /* VMS_ALLOC will replace this field */
    ANDROID_EMU_ADDRESS_SPACE_FREE_FUNC(allocator->blocks);
    return 0;
}

static const VMStateDescription vmstate_address_space_allocator = {
    .name = "address_space_allocator",
    .version_id = 0,
    .minimum_version_id = 0,
    .pre_load = &vmstate_address_space_allocator_pre_load,
    .fields = (VMStateField[]) {
        VMSTATE_INT32(size,         struct address_space_allocator),
        VMSTATE_INT32(capacity,     struct address_space_allocator),
        VMSTATE_UINT64(total_bytes, struct address_space_allocator),
        VMSTATE_STRUCT_VARRAY_ALLOC(blocks,
                                    struct address_space_allocator,
                                    capacity,
                                    0,
                                    vmstate_address_block,
                                    struct address_block),
        VMSTATE_END_OF_LIST()
    }
};

static const VMStateDescription vmstate_address_space_pci = {
    .name = GOLDFISH_ADDRESS_SPACE_NAME,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_PCI_DEVICE(parent_obj, struct address_space_state),
        VMSTATE_STRUCT(registers, struct address_space_state, 0,
                       vmstate_address_space_registers,
                       struct address_space_registers),
        VMSTATE_STRUCT(allocator, struct address_space_state, 0,
                       vmstate_address_space_allocator,
                       struct address_space_allocator),
        VMSTATE_USER("memory_state", 0,
                     vmstate_info_address_space_memory_state),
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

/* Host interface */
int goldfish_address_space_alloc_shared_host_region(
    uint64_t page_aligned_size, uint64_t* offset)
{
    uint64_t offset_out = ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET;
    int res;

    if (!s_current_state)
    {
        fprintf(stderr, "%s: ERROR: no allocator present!\n", __func__);
        return -EINVAL;
    }

    qemu_mutex_lock(&s_current_state->mutex);

    res = goldfish_address_space_alloc_shared_host_region_locked(
        page_aligned_size, offset);

    qemu_mutex_unlock(&s_current_state->mutex);

    return res;
}

int goldfish_address_space_free_shared_host_region(uint64_t offset)
{
    int res;

    if (!s_current_state)
    {
        fprintf(stderr, "%s: ERROR: no allocator present!\n", __func__);
        return -EINVAL;
    }

    qemu_mutex_lock(&s_current_state->mutex);

    res = goldfish_address_space_free_shared_host_region_locked(offset);

    qemu_mutex_unlock(&s_current_state->mutex);

    return res;
}

int goldfish_address_space_alloc_shared_host_region_locked(
    uint64_t page_aligned_size, uint64_t* offset)
{
    uint64_t offset_out = ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET;

    if (!offset)
    {
        fprintf(stderr, "%s: ERROR: |offset| output is null!\n", __func__);
        return -EINVAL;
    }

    offset_out = address_space_allocator_allocate(
        &s_current_state->allocator, page_aligned_size);

    if (offset_out == ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET)
    {
        return -ENOMEM;
    }
    else
    {
        *offset = offset_out;
        return 0;
    }
}

int goldfish_address_space_free_shared_host_region_locked(uint64_t offset)
{
    uint32_t res = address_space_allocator_deallocate(
        &s_current_state->allocator, offset);

    if (!res)
    {
        return 0;
    }
    else
    {
        return -EINVAL;
    }
}

uint64_t goldfish_address_space_get_phys_addr_start(void)
{
    uint64_t res;

    if (!s_current_state)
    {
        fprintf(stderr, "%s: ERROR: no allocator present!\n", __func__);
        return ANDROID_EMU_ADDRESS_SPACE_BAD_OFFSET;
    }

    qemu_mutex_lock(&s_current_state->mutex);

    res = goldfish_address_space_get_phys_addr_start_locked();

    qemu_mutex_unlock(&s_current_state->mutex);

    return res;
}

uint64_t goldfish_address_space_get_phys_addr_start_locked(void)
{
    uint64_t res;

    if (!s_current_state) return 0;

    res = (uint64_t)s_current_state->registers.phys_start_low;
    res |= ((uint64_t)s_current_state->registers.phys_start_high) << 32;

    // BUG: 162435702, 162264185
    // It's not fatal; just treat 0 as a invalid phys address start.
    // Q system images might not have this.
    // This depends on the guest kernel.
    //
    // if (!res)
    // {
    //     qemu_abort("FATAL: Tried to ask for phys addr start without it being initialized!\n");
    // }

    return res;
}
