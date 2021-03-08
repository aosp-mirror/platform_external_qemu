#ifndef EXEC_MEMORY_REMAH_H_INCLUDED
#define EXEC_MEMORY_REMAH_H_INCLUDED

#include <stdint.h>

struct RAMBlock;

/*
 * TODO(lfy@google.com):
 * General function to remap a RAM block's backing store, if any.
 * It is assumed the size of the RAMBlock will be the same before and after,
 * otherwise, there will be an abort.
 *
 * If mem_path is null, then we remap to an anonymous mapping.
 * If mem_path is not null, then the RAMBlock will be remapped to the
 * given mem_path and will be mapped privately unless |shared| is set.
 */
void ram_block_remap_backing(RAMBlock* block, const char* mem_path, int shared);

/* Remaps all RAMBlocks with file backing, preserving all other settings
 * except for |shared|: whether the file is mapped shared or private.*/
void ram_blocks_remap_shared(int shared);

/*
 * Calls region_del and region_add for registered memory listeners.
 * Purpose: to refresh hypervisor memory mappings, primarily.
 */
void memory_listeners_refresh_topology();

/*
 * API for adding and removing mappings of guest RAM and host addrs.
 * Implementation depends on the hypervisor.
 */
#define USER_BACKED_RAM_FLAGS_NONE 0x0
#define USER_BACKED_RAM_FLAGS_READ 0x1
#define USER_BACKED_RAM_FLAGS_WRITE 0x2
#define USER_BACKED_RAM_FLAGS_EXEC 0x4
void qemu_user_backed_ram_map(uint64_t gpa, void* hva, uint64_t size, int flags);
void qemu_user_backed_ram_unmap(uint64_t gpa, uint64_t size);

#endif  /* EXEC_MEMORY_REMAH_H_INCLUDED */
