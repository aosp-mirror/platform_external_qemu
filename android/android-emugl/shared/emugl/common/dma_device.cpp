
#include "android/emulation/GoldfishDma.h"
#include "emugl/common/dma_device.h"

static void defaultDmaAddBuffer(void* pipe, uint64_t guest_paddr, uint64_t size) { }
static void defaultDmaRemoveBuffer(uint64_t guest_paddr) { }
static void* defaultDmaGetHostAddr(uint64_t guest_paddr) {
    return nullptr;
}
static void defaultDmaInvalidateHostMappings() { }

static void defaultDmaUnlock(uint64_t addr) { }

static void defaultDmaHostmemSetPtr(uint64_t hostmem_id, void* host_ptr, uint64_t host_size, uint64_t status_code) {
    fprintf(stderr, "%s: asdf\n", __func__);
}
static void defaultDmaHostmemUnsetPtr(uint64_t hostmem_id, uint64_t status_code) { }

emugl_dma_add_buffer_t g_emugl_dma_add_buffer = defaultDmaAddBuffer;
emugl_dma_remove_buffer_t g_emugl_dma_remove_buffer = defaultDmaRemoveBuffer;
emugl_dma_get_host_addr_t g_emugl_dma_get_host_addr = defaultDmaGetHostAddr;
emugl_dma_invalidate_host_mappings_t g_emugl_dma_invalidate_host_mappings = defaultDmaInvalidateHostMappings;
emugl_dma_unlock_t g_emugl_dma_unlock = defaultDmaUnlock;
emugl_dma_hostmem_set_ptr_t g_emugl_dma_hostmem_set_ptr = defaultDmaHostmemSetPtr;
emugl_dma_hostmem_unset_ptr_t g_emugl_dma_hostmem_unset_ptr = defaultDmaHostmemUnsetPtr;

void set_emugl_dma_add_buffer(emugl_dma_add_buffer_t f) {
    g_emugl_dma_add_buffer = f;
}

void set_emugl_dma_remove_buffer(emugl_dma_remove_buffer_t f) {
    g_emugl_dma_remove_buffer = f;
}

void set_emugl_dma_get_host_addr(emugl_dma_get_host_addr_t f) {
    g_emugl_dma_get_host_addr = f;
}

void set_emugl_dma_invalidate_host_mappings(emugl_dma_invalidate_host_mappings_t f) {
    g_emugl_dma_invalidate_host_mappings = f;
}

void set_emugl_dma_unlock(emugl_dma_unlock_t f) {
    g_emugl_dma_unlock = f;
}

void set_emugl_dma_hostmem_set_ptr(emugl_dma_hostmem_set_ptr_t f) {
    g_emugl_dma_hostmem_set_ptr = f;
}

void set_emugl_dma_hostmem_unset_ptr(emugl_dma_hostmem_unset_ptr_t f) {
    g_emugl_dma_hostmem_unset_ptr = f;
}
