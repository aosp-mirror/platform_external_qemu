#ifndef CPU_COMMON_H
#define CPU_COMMON_H 1

#include "qemu-common.h"

/* CPU interfaces that are target independent.  */

#ifndef CONFIG_USER_ONLY
#include "exec/hwaddr.h"
#endif

#ifndef NEED_CPU_H
#include "exec/poison.h"
#endif

#include "qemu/bswap.h"
#include "qemu/queue.h"

/**
 * CPUListState:
 * @cpu_fprintf: Print function.
 * @file: File to print to using @cpu_fprint.
 *
 * State commonly used for iterating over CPU models.
 */
typedef struct CPUListState {
    fprintf_function cpu_fprintf;
    FILE *file;
} CPUListState;

#if !defined(CONFIG_USER_ONLY)

enum device_endian {
    DEVICE_NATIVE_ENDIAN,
    DEVICE_BIG_ENDIAN,
    DEVICE_LITTLE_ENDIAN,
};

/* address in the RAM (different from a physical address) */
#if defined(CONFIG_XEN_BACKEND)
typedef uint64_t ram_addr_t;
#  define RAM_ADDR_MAX UINT64_MAX
#  define RAM_ADDR_FMT "%" PRIx64
#else
typedef uintptr_t ram_addr_t;
#  define RAM_ADDR_MAX UINTPTR_MAX
#  define RAM_ADDR_FMT "%" PRIxPTR
#endif

/* memory API */

/* MMIO pages are identified by a combination of an IO device index and
   3 flags.  The ROMD code stores the page ram offset in iotlb entry,
   so only a limited number of ids are avaiable.  */

#define IO_MEM_NB_ENTRIES  (1 << (TARGET_PAGE_BITS  - IO_MEM_SHIFT))

typedef void CPUWriteMemoryFunc(void *opaque, hwaddr addr, uint32_t value);
typedef uint32_t CPUReadMemoryFunc(void *opaque, hwaddr addr);

void cpu_register_physical_memory_log(hwaddr start_addr,
                                      ram_addr_t size,
                                      ram_addr_t phys_offset,
                                      ram_addr_t region_offset,
                                      bool log_dirty);

static inline void cpu_register_physical_memory_offset(hwaddr start_addr,
                                                       ram_addr_t size,
                                                       ram_addr_t phys_offset,
                                                       ram_addr_t region_offset)
{
    cpu_register_physical_memory_log(start_addr, size, phys_offset,
                                     region_offset, false);
}

static inline void cpu_register_physical_memory(hwaddr start_addr,
                                                ram_addr_t size,
                                                ram_addr_t phys_offset)
{
    cpu_register_physical_memory_offset(start_addr, size, phys_offset, 0);
}

ram_addr_t cpu_get_physical_page_desc(hwaddr addr);
ram_addr_t qemu_ram_alloc_from_ptr(DeviceState *dev, const char *name,
                        ram_addr_t size, void *host);
ram_addr_t qemu_ram_alloc(DeviceState *dev, const char *name, ram_addr_t size);
void qemu_ram_free(ram_addr_t addr);
void qemu_ram_remap(ram_addr_t addr, ram_addr_t length);
/* This should only be used for ram local to a device.  */
void *qemu_get_ram_ptr(ram_addr_t addr);
/* Same but slower, to use for migration, where the order of
 * RAMBlocks must not change. */
void *qemu_safe_ram_ptr(ram_addr_t addr);
/* This should not be used by devices.  */
int qemu_ram_addr_from_host(void *ptr, ram_addr_t *ram_addr);
ram_addr_t qemu_ram_addr_from_host_nofail(void *ptr);

int cpu_register_io_memory(CPUReadMemoryFunc * const *mem_read,
                           CPUWriteMemoryFunc * const *mem_write,
                           void *opaque);
void cpu_unregister_io_memory(int table_address);

void cpu_physical_memory_rw(hwaddr addr, void *buf,
                            int len, int is_write);
static inline void cpu_physical_memory_read(hwaddr addr,
                                            void *buf, int len)
{
    cpu_physical_memory_rw(addr, buf, len, 0);
}
static inline void cpu_physical_memory_write(hwaddr addr,
                                             const void *buf, int len)
{
    cpu_physical_memory_rw(addr, (void*)buf, len, 1);
}
void *cpu_physical_memory_map(hwaddr addr,
                              hwaddr *plen,
                              int is_write);
void cpu_physical_memory_unmap(void *buffer, hwaddr len,
                               int is_write, hwaddr access_len);
void *cpu_register_map_client(void *opaque, void (*callback)(void *opaque));

uint32_t ldub_phys(hwaddr addr);
uint32_t lduw_le_phys(hwaddr addr);
uint32_t lduw_be_phys(hwaddr addr);
uint32_t ldl_le_phys(hwaddr addr);
uint32_t ldl_be_phys(hwaddr addr);
uint64_t ldq_le_phys(hwaddr addr);
uint64_t ldq_be_phys(hwaddr addr);
void stb_phys(hwaddr addr, uint32_t val);
void stw_le_phys(hwaddr addr, uint32_t val);
void stw_be_phys(hwaddr addr, uint32_t val);
void stl_le_phys(hwaddr addr, uint32_t val);
void stl_be_phys(hwaddr addr, uint32_t val);
void stq_le_phys(hwaddr addr, uint64_t val);
void stq_be_phys(hwaddr addr, uint64_t val);

#ifdef NEED_CPU_H
uint32_t lduw_phys(hwaddr addr);
uint32_t ldl_phys(hwaddr addr);
uint64_t ldq_phys(hwaddr addr);
void stl_phys_notdirty(hwaddr addr, uint32_t val);
void stq_phys_notdirty(hwaddr addr, uint64_t val);
void stw_phys(hwaddr addr, uint32_t val);
void stl_phys(hwaddr addr, uint32_t val);
void stq_phys(hwaddr addr, uint64_t val);
#endif

void cpu_physical_memory_write_rom(hwaddr addr,
                                   const void *buf, int len);

#define IO_MEM_SHIFT       3

#define IO_MEM_RAM         (0 << IO_MEM_SHIFT) /* hardcoded offset */
#define IO_MEM_ROM         (1 << IO_MEM_SHIFT) /* hardcoded offset */
#define IO_MEM_UNASSIGNED  (2 << IO_MEM_SHIFT)
#define IO_MEM_NOTDIRTY    (3 << IO_MEM_SHIFT)

/* Acts like a ROM when read and like a device when written.  */
#define IO_MEM_ROMD        (1)
#define IO_MEM_SUBPAGE     (2)
#define IO_MEM_SUBWIDTH    (4)

#endif

#endif /* !CPU_COMMON_H */
