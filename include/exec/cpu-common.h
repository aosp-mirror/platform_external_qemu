#ifndef CPU_COMMON_H
#define CPU_COMMON_H 1

/* CPU interfaces that are target indpendent.  */

#if defined(__arm__) || defined(__sparc__) || defined(__mips__) || defined(__hppa__) || defined(__ia64__)
#define WORDS_ALIGNED
#endif

#ifdef TARGET_PHYS_ADDR_BITS
#include "exec/hwaddr.h"
#endif

#ifndef NEED_CPU_H
#include "exec/poison.h"
#endif

#include "qemu/bswap.h"
#include "qemu/queue.h"

#if !defined(CONFIG_USER_ONLY)

/* address in the RAM (different from a physical address) */
typedef unsigned long ram_addr_t;

/* memory API */

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

void cpu_physical_memory_rw(hwaddr addr, uint8_t *buf,
                            int len, int is_write);
static inline void cpu_physical_memory_read(hwaddr addr,
                                            uint8_t *buf, int len)
{
    cpu_physical_memory_rw(addr, buf, len, 0);
}
static inline void cpu_physical_memory_write(hwaddr addr,
                                             const uint8_t *buf, int len)
{
    cpu_physical_memory_rw(addr, (uint8_t *)buf, len, 1);
}
void *cpu_physical_memory_map(hwaddr addr,
                              hwaddr *plen,
                              int is_write);
void cpu_physical_memory_unmap(void *buffer, hwaddr len,
                               int is_write, hwaddr access_len);
void *cpu_register_map_client(void *opaque, void (*callback)(void *opaque));
void cpu_unregister_map_client(void *cookie);

uint32_t ldub_phys(hwaddr addr);
uint32_t lduw_phys(hwaddr addr);
uint32_t ldl_phys(hwaddr addr);
uint64_t ldq_phys(hwaddr addr);
void stl_phys_notdirty(hwaddr addr, uint32_t val);
void stq_phys_notdirty(hwaddr addr, uint64_t val);
void stb_phys(hwaddr addr, uint32_t val);
void stw_phys(hwaddr addr, uint32_t val);
void stl_phys(hwaddr addr, uint32_t val);
void stq_phys(hwaddr addr, uint64_t val);

void cpu_physical_memory_write_rom(hwaddr addr,
                                   const uint8_t *buf, int len);

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
