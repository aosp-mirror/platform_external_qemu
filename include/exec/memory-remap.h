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

/* Remaps a portion of guest physical addresses. Allocates and
 * returns a new MemoryRegion* describing the mapping. */
MemoryRegion* memory_region_system_memory_redirect_add(const char* name, hwaddr gpa_start, size_t len, void* host);

/* Undoes |memory_region_redirect_guest_ram| */
void memory_region_system_memory_redirect_remove(MemoryRegion* mr);
