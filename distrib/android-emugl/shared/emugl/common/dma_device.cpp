
#include "android/emulation/GoldfishDma.h"
#include "emugl/common/dma_device.h"

static void* defaultDmaReader(uint32_t offset) {
    return nullptr;
}

static void defaultDmaAddBuffer(void* pipe, uint64_t guest_paddr, uint64_t size) { }
static void defaultDmaRemoveBuffer(uint64_t guest_paddr) { }
static uint64_t defaultDmaGetHostAddr(uint64_t guest_paddr) {
    return 0;
}
static void defaultDmaInvalidateHostMappings() { }

static void defaultDmaUnlock(uint64_t addr) { }

emugl_dma_read_t g_emugl_dma_read = defaultDmaReader;
emugl_dma_add_buffer_t g_emugl_dma_add_buffer = defaultDmaAddBuffer;
emugl_dma_remove_buffer_t g_emugl_dma_remove_buffer = defaultDmaRemoveBuffer;
emugl_dma_get_host_addr_t g_emugl_dma_get_host_addr = defaultDmaGetHostAddr;
emugl_dma_invalidate_host_mappings_t g_emugl_dma_invalidate_host_mappings = defaultDmaInvalidateHostMappings;
emugl_dma_unlock_t g_emugl_dma_unlock = defaultDmaUnlock;

void set_emugl_dma_read(emugl_dma_read_t f) {
    g_emugl_dma_read = f;
}

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
