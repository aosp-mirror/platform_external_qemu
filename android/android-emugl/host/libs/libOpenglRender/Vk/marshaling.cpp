
#include "marshaling.h"

#include "vkUtils_custom.h"

static vk_reassign_funcs sReassigns = {};

void registerReassignmentFuncs(vk_reassign_funcs* funcs) {
    memcpy(&sReassigns, funcs, sizeof(vk_reassign_funcs));
}



size_t allocSz_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkMappedMemoryRange);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkMappedMemoryRange* initFromBuffer_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, unsigned char* buf) {
    VkMappedMemoryRange* res;
    
    res = (VkMappedMemoryRange*)buf; buf += overall_count * sizeof(VkMappedMemoryRange);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkMappedMemoryRange));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkMappedMemoryRange));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkDeviceMemory:memory
        remap_VkDeviceMemory(sReassigns.reassign_VkDeviceMemory, 1, (&(in[i0].memory)), (VkDeviceMemory*)(&(out[i0].memory)));
    }
}

void deepCopyWithRemap_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, VkMappedMemoryRange* out) {
    deepCopyAssign_VkMappedMemoryRange(overall_count, in, out);
    deepCopyReassign_VkMappedMemoryRange(overall_count, in, out);
}

VkMappedMemoryRange* remap_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in, unsigned char* buf) {
    VkMappedMemoryRange* res = initFromBuffer_VkMappedMemoryRange(overall_count, in, buf);
    deepCopyAssign_VkMappedMemoryRange(overall_count, in, res);
    deepCopyReassign_VkMappedMemoryRange(overall_count, in, res);
    return res;
}



size_t allocSz_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkSubmitInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            totalSz += (in + i0)->waitSemaphoreCount * sizeof(VkSemaphore);
        }
        if (in[i0].pCommandBuffers) {
            totalSz += (in + i0)->commandBufferCount * sizeof(VkCommandBuffer);
        }
        if (in[i0].pSignalSemaphores) {
            totalSz += (in + i0)->signalSemaphoreCount * sizeof(VkSemaphore);
        }
    }
    return totalSz;
}

VkSubmitInfo* initFromBuffer_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, unsigned char* buf) {
    VkSubmitInfo* res;
    
    res = (VkSubmitInfo*)buf; buf += overall_count * sizeof(VkSubmitInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            memcpy((void*)(&res[i0].pWaitSemaphores), ((VkSemaphore**)&buf), sizeof(void*));
            buf += (in + i0)->waitSemaphoreCount * sizeof(VkSemaphore);
        }
        if (in[i0].pCommandBuffers) {
            memcpy((void*)(&res[i0].pCommandBuffers), ((VkCommandBuffer**)&buf), sizeof(void*));
            buf += (in + i0)->commandBufferCount * sizeof(VkCommandBuffer);
        }
        if (in[i0].pSignalSemaphores) {
            memcpy((void*)(&res[i0].pSignalSemaphores), ((VkSemaphore**)&buf), sizeof(void*));
            buf += (in + i0)->signalSemaphoreCount * sizeof(VkSemaphore);
        }
    }
    return res;
}

void deepCopyAssign_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSubmitInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            memcpy((void*)out[i0].pWaitSemaphores, in[i0].pWaitSemaphores, (in + i0)->waitSemaphoreCount * sizeof(VkSemaphore));
        }
        if (in[i0].pCommandBuffers) {
            memcpy((void*)out[i0].pCommandBuffers, in[i0].pCommandBuffers, (in + i0)->commandBufferCount * sizeof(VkCommandBuffer));
        }
        if (in[i0].pSignalSemaphores) {
            memcpy((void*)out[i0].pSignalSemaphores, in[i0].pSignalSemaphores, (in + i0)->signalSemaphoreCount * sizeof(VkSemaphore));
        }
    }
}

void deepCopyReassign_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSubmitInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            // TODO: reassign ptr handle field VkSemaphore:pWaitSemaphores
            remap_VkSemaphore(sReassigns.reassign_VkSemaphore, (in + i0)->waitSemaphoreCount, in[i0].pWaitSemaphores, (VkSemaphore*)out[i0].pWaitSemaphores);
        }
        if (in[i0].pCommandBuffers) {
            // TODO: reassign ptr handle field VkCommandBuffer:pCommandBuffers
            remap_VkCommandBuffer(sReassigns.reassign_VkCommandBuffer, (in + i0)->commandBufferCount, in[i0].pCommandBuffers, (VkCommandBuffer*)out[i0].pCommandBuffers);
        }
        if (in[i0].pSignalSemaphores) {
            // TODO: reassign ptr handle field VkSemaphore:pSignalSemaphores
            remap_VkSemaphore(sReassigns.reassign_VkSemaphore, (in + i0)->signalSemaphoreCount, in[i0].pSignalSemaphores, (VkSemaphore*)out[i0].pSignalSemaphores);
        }
    }
}

void deepCopyWithRemap_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, VkSubmitInfo* out) {
    deepCopyAssign_VkSubmitInfo(overall_count, in, out);
    deepCopyReassign_VkSubmitInfo(overall_count, in, out);
}

VkSubmitInfo* remap_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in, unsigned char* buf) {
    VkSubmitInfo* res = initFromBuffer_VkSubmitInfo(overall_count, in, buf);
    deepCopyAssign_VkSubmitInfo(overall_count, in, res);
    deepCopyReassign_VkSubmitInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkSparseMemoryBind);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkSparseMemoryBind* initFromBuffer_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, unsigned char* buf) {
    VkSparseMemoryBind* res;
    
    res = (VkSparseMemoryBind*)buf; buf += overall_count * sizeof(VkSparseMemoryBind);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseMemoryBind));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseMemoryBind));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkDeviceMemory:memory
        remap_VkDeviceMemory(sReassigns.reassign_VkDeviceMemory, 1, (&(in[i0].memory)), (VkDeviceMemory*)(&(out[i0].memory)));
    }
}

void deepCopyWithRemap_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, VkSparseMemoryBind* out) {
    deepCopyAssign_VkSparseMemoryBind(overall_count, in, out);
    deepCopyReassign_VkSparseMemoryBind(overall_count, in, out);
}

VkSparseMemoryBind* remap_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in, unsigned char* buf) {
    VkSparseMemoryBind* res = initFromBuffer_VkSparseMemoryBind(overall_count, in, buf);
    deepCopyAssign_VkSparseMemoryBind(overall_count, in, res);
    deepCopyReassign_VkSparseMemoryBind(overall_count, in, res);
    return res;
}



size_t allocSz_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkSparseBufferMemoryBindInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBinds) {
            totalSz += (in + i0)->bindCount * sizeof(VkSparseMemoryBind);
        }
    }
    return totalSz;
}

VkSparseBufferMemoryBindInfo* initFromBuffer_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, unsigned char* buf) {
    VkSparseBufferMemoryBindInfo* res;
    
    res = (VkSparseBufferMemoryBindInfo*)buf; buf += overall_count * sizeof(VkSparseBufferMemoryBindInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBinds) {
            memcpy((void*)(&res[i0].pBinds), ((VkSparseMemoryBind**)&buf), sizeof(void*));
            buf += (in + i0)->bindCount * sizeof(VkSparseMemoryBind);
        }
    }
    return res;
}

void deepCopyAssign_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseBufferMemoryBindInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBinds) {
            memcpy((void*)out[i0].pBinds, in[i0].pBinds, (in + i0)->bindCount * sizeof(VkSparseMemoryBind));
        }
    }
}

void deepCopyReassign_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseBufferMemoryBindInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkBuffer:buffer
        remap_VkBuffer(sReassigns.reassign_VkBuffer, 1, (&(in[i0].buffer)), (VkBuffer*)(&(out[i0].buffer)));
        if (in[i0].pBinds) {
            memcpy((void*)out[i0].pBinds, in[i0].pBinds, (in + i0)->bindCount /* substruct */ * sizeof(VkSparseMemoryBind));
            for (uint32_t i2 = 0; i2 < (in + i0)->bindCount /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkDeviceMemory:memory
                remap_VkDeviceMemory(sReassigns.reassign_VkDeviceMemory, 1, (&(in[i0].pBinds[i2].memory)), (VkDeviceMemory*)(&(out[i0].pBinds[i2].memory)));
            }
        }
    }
}

void deepCopyWithRemap_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, VkSparseBufferMemoryBindInfo* out) {
    deepCopyAssign_VkSparseBufferMemoryBindInfo(overall_count, in, out);
    deepCopyReassign_VkSparseBufferMemoryBindInfo(overall_count, in, out);
}

VkSparseBufferMemoryBindInfo* remap_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in, unsigned char* buf) {
    VkSparseBufferMemoryBindInfo* res = initFromBuffer_VkSparseBufferMemoryBindInfo(overall_count, in, buf);
    deepCopyAssign_VkSparseBufferMemoryBindInfo(overall_count, in, res);
    deepCopyReassign_VkSparseBufferMemoryBindInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkSparseImageOpaqueMemoryBindInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBinds) {
            totalSz += (in + i0)->bindCount * sizeof(VkSparseMemoryBind);
        }
    }
    return totalSz;
}

VkSparseImageOpaqueMemoryBindInfo* initFromBuffer_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, unsigned char* buf) {
    VkSparseImageOpaqueMemoryBindInfo* res;
    
    res = (VkSparseImageOpaqueMemoryBindInfo*)buf; buf += overall_count * sizeof(VkSparseImageOpaqueMemoryBindInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBinds) {
            memcpy((void*)(&res[i0].pBinds), ((VkSparseMemoryBind**)&buf), sizeof(void*));
            buf += (in + i0)->bindCount * sizeof(VkSparseMemoryBind);
        }
    }
    return res;
}

void deepCopyAssign_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseImageOpaqueMemoryBindInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBinds) {
            memcpy((void*)out[i0].pBinds, in[i0].pBinds, (in + i0)->bindCount * sizeof(VkSparseMemoryBind));
        }
    }
}

void deepCopyReassign_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseImageOpaqueMemoryBindInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkImage:image
        remap_VkImage(sReassigns.reassign_VkImage, 1, (&(in[i0].image)), (VkImage*)(&(out[i0].image)));
        if (in[i0].pBinds) {
            memcpy((void*)out[i0].pBinds, in[i0].pBinds, (in + i0)->bindCount /* substruct */ * sizeof(VkSparseMemoryBind));
            for (uint32_t i2 = 0; i2 < (in + i0)->bindCount /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkDeviceMemory:memory
                remap_VkDeviceMemory(sReassigns.reassign_VkDeviceMemory, 1, (&(in[i0].pBinds[i2].memory)), (VkDeviceMemory*)(&(out[i0].pBinds[i2].memory)));
            }
        }
    }
}

void deepCopyWithRemap_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, VkSparseImageOpaqueMemoryBindInfo* out) {
    deepCopyAssign_VkSparseImageOpaqueMemoryBindInfo(overall_count, in, out);
    deepCopyReassign_VkSparseImageOpaqueMemoryBindInfo(overall_count, in, out);
}

VkSparseImageOpaqueMemoryBindInfo* remap_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in, unsigned char* buf) {
    VkSparseImageOpaqueMemoryBindInfo* res = initFromBuffer_VkSparseImageOpaqueMemoryBindInfo(overall_count, in, buf);
    deepCopyAssign_VkSparseImageOpaqueMemoryBindInfo(overall_count, in, res);
    deepCopyReassign_VkSparseImageOpaqueMemoryBindInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkSparseImageMemoryBindInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkSparseImageMemoryBindInfo* initFromBuffer_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, unsigned char* buf) {
    VkSparseImageMemoryBindInfo* res;
    
    res = (VkSparseImageMemoryBindInfo*)buf; buf += overall_count * sizeof(VkSparseImageMemoryBindInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseImageMemoryBindInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkSparseImageMemoryBindInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkImage:image
        remap_VkImage(sReassigns.reassign_VkImage, 1, (&(in[i0].image)), (VkImage*)(&(out[i0].image)));
    }
}

void deepCopyWithRemap_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, VkSparseImageMemoryBindInfo* out) {
    deepCopyAssign_VkSparseImageMemoryBindInfo(overall_count, in, out);
    deepCopyReassign_VkSparseImageMemoryBindInfo(overall_count, in, out);
}

VkSparseImageMemoryBindInfo* remap_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in, unsigned char* buf) {
    VkSparseImageMemoryBindInfo* res = initFromBuffer_VkSparseImageMemoryBindInfo(overall_count, in, buf);
    deepCopyAssign_VkSparseImageMemoryBindInfo(overall_count, in, res);
    deepCopyReassign_VkSparseImageMemoryBindInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkBindSparseInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            totalSz += (in + i0)->waitSemaphoreCount * sizeof(VkSemaphore);
        }
        if (in[i0].pBufferBinds) {
            totalSz += (in + i0)->bufferBindCount /* substruct */ * sizeof(VkSparseBufferMemoryBindInfo);
            for (uint32_t i2 = 0; i2 < (in + i0)->bufferBindCount /* substruct */; i2++) {
                if (in[i0].pBufferBinds[i2].pBinds) {
                    totalSz += (in[i0].pBufferBinds + i2)->bindCount * sizeof(VkSparseMemoryBind);
                }
            }
        }
        if (in[i0].pImageOpaqueBinds) {
            totalSz += (in + i0)->imageOpaqueBindCount /* substruct */ * sizeof(VkSparseImageOpaqueMemoryBindInfo);
            for (uint32_t i2 = 0; i2 < (in + i0)->imageOpaqueBindCount /* substruct */; i2++) {
                if (in[i0].pImageOpaqueBinds[i2].pBinds) {
                    totalSz += (in[i0].pImageOpaqueBinds + i2)->bindCount * sizeof(VkSparseMemoryBind);
                }
            }
        }
        if (in[i0].pImageBinds) {
            totalSz += (in + i0)->imageBindCount * sizeof(VkSparseImageMemoryBindInfo);
        }
        if (in[i0].pSignalSemaphores) {
            totalSz += (in + i0)->signalSemaphoreCount * sizeof(VkSemaphore);
        }
    }
    return totalSz;
}

VkBindSparseInfo* initFromBuffer_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, unsigned char* buf) {
    VkBindSparseInfo* res;
    
    res = (VkBindSparseInfo*)buf; buf += overall_count * sizeof(VkBindSparseInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            memcpy((void*)(&res[i0].pWaitSemaphores), ((VkSemaphore**)&buf), sizeof(void*));
            buf += (in + i0)->waitSemaphoreCount * sizeof(VkSemaphore);
        }
        if (in[i0].pBufferBinds) {
            res[i0].pBufferBinds = (VkSparseBufferMemoryBindInfo*)buf; buf += (in + i0)->bufferBindCount /* substruct */ * sizeof(VkSparseBufferMemoryBindInfo);
            for (uint32_t i2 = 0; i2 < (in + i0)->bufferBindCount /* substruct */; i2++) {
                if (in[i0].pBufferBinds[i2].pBinds) {
                    memcpy((void*)(&res[i0].pBufferBinds[i2].pBinds), ((VkSparseMemoryBind**)&buf), sizeof(void*));
                    buf += (in[i0].pBufferBinds + i2)->bindCount * sizeof(VkSparseMemoryBind);
                }
            }
        }
        if (in[i0].pImageOpaqueBinds) {
            res[i0].pImageOpaqueBinds = (VkSparseImageOpaqueMemoryBindInfo*)buf; buf += (in + i0)->imageOpaqueBindCount /* substruct */ * sizeof(VkSparseImageOpaqueMemoryBindInfo);
            for (uint32_t i2 = 0; i2 < (in + i0)->imageOpaqueBindCount /* substruct */; i2++) {
                if (in[i0].pImageOpaqueBinds[i2].pBinds) {
                    memcpy((void*)(&res[i0].pImageOpaqueBinds[i2].pBinds), ((VkSparseMemoryBind**)&buf), sizeof(void*));
                    buf += (in[i0].pImageOpaqueBinds + i2)->bindCount * sizeof(VkSparseMemoryBind);
                }
            }
        }
        if (in[i0].pImageBinds) {
            memcpy((void*)(&res[i0].pImageBinds), ((VkSparseImageMemoryBindInfo**)&buf), sizeof(void*));
            buf += (in + i0)->imageBindCount * sizeof(VkSparseImageMemoryBindInfo);
        }
        if (in[i0].pSignalSemaphores) {
            memcpy((void*)(&res[i0].pSignalSemaphores), ((VkSemaphore**)&buf), sizeof(void*));
            buf += (in + i0)->signalSemaphoreCount * sizeof(VkSemaphore);
        }
    }
    return res;
}

void deepCopyAssign_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkBindSparseInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            memcpy((void*)out[i0].pWaitSemaphores, in[i0].pWaitSemaphores, (in + i0)->waitSemaphoreCount * sizeof(VkSemaphore));
        }
        if (in[i0].pBufferBinds) {
            memcpy((void*)out[i0].pBufferBinds, in[i0].pBufferBinds, (in + i0)->bufferBindCount /* substruct */ * sizeof(VkSparseBufferMemoryBindInfo));
            for (uint32_t i2 = 0; i2 < (in + i0)->bufferBindCount /* substruct */; i2++) {
                if (in[i0].pBufferBinds[i2].pBinds) {
                    memcpy((void*)out[i0].pBufferBinds[i2].pBinds, in[i0].pBufferBinds[i2].pBinds, (in[i0].pBufferBinds + i2)->bindCount * sizeof(VkSparseMemoryBind));
                }
            }
        }
        if (in[i0].pImageOpaqueBinds) {
            memcpy((void*)out[i0].pImageOpaqueBinds, in[i0].pImageOpaqueBinds, (in + i0)->imageOpaqueBindCount /* substruct */ * sizeof(VkSparseImageOpaqueMemoryBindInfo));
            for (uint32_t i2 = 0; i2 < (in + i0)->imageOpaqueBindCount /* substruct */; i2++) {
                if (in[i0].pImageOpaqueBinds[i2].pBinds) {
                    memcpy((void*)out[i0].pImageOpaqueBinds[i2].pBinds, in[i0].pImageOpaqueBinds[i2].pBinds, (in[i0].pImageOpaqueBinds + i2)->bindCount * sizeof(VkSparseMemoryBind));
                }
            }
        }
        if (in[i0].pImageBinds) {
            memcpy((void*)out[i0].pImageBinds, in[i0].pImageBinds, (in + i0)->imageBindCount * sizeof(VkSparseImageMemoryBindInfo));
        }
        if (in[i0].pSignalSemaphores) {
            memcpy((void*)out[i0].pSignalSemaphores, in[i0].pSignalSemaphores, (in + i0)->signalSemaphoreCount * sizeof(VkSemaphore));
        }
    }
}

void deepCopyReassign_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkBindSparseInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pWaitSemaphores) {
            // TODO: reassign ptr handle field VkSemaphore:pWaitSemaphores
            remap_VkSemaphore(sReassigns.reassign_VkSemaphore, (in + i0)->waitSemaphoreCount, in[i0].pWaitSemaphores, (VkSemaphore*)out[i0].pWaitSemaphores);
        }
        if (in[i0].pBufferBinds) {
            memcpy((void*)out[i0].pBufferBinds, in[i0].pBufferBinds, (in + i0)->bufferBindCount /* substruct */ * sizeof(VkSparseBufferMemoryBindInfo));
            for (uint32_t i2 = 0; i2 < (in + i0)->bufferBindCount /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkBuffer:buffer
                remap_VkBuffer(sReassigns.reassign_VkBuffer, 1, (&(in[i0].pBufferBinds[i2].buffer)), (VkBuffer*)(&(out[i0].pBufferBinds[i2].buffer)));
                if (in[i0].pBufferBinds[i2].pBinds) {
                    memcpy((void*)out[i0].pBufferBinds[i2].pBinds, in[i0].pBufferBinds[i2].pBinds, (in[i0].pBufferBinds + i2)->bindCount /* substruct */ * sizeof(VkSparseMemoryBind));
                    for (uint32_t i4 = 0; i4 < (in[i0].pBufferBinds + i2)->bindCount /* substruct */; i4++) {
                        // TODO: reassign non-ptr handle field VkDeviceMemory:memory
                        remap_VkDeviceMemory(sReassigns.reassign_VkDeviceMemory, 1, (&(in[i0].pBufferBinds[i2].pBinds[i4].memory)), (VkDeviceMemory*)(&(out[i0].pBufferBinds[i2].pBinds[i4].memory)));
                    }
                }
            }
        }
        if (in[i0].pImageOpaqueBinds) {
            memcpy((void*)out[i0].pImageOpaqueBinds, in[i0].pImageOpaqueBinds, (in + i0)->imageOpaqueBindCount /* substruct */ * sizeof(VkSparseImageOpaqueMemoryBindInfo));
            for (uint32_t i2 = 0; i2 < (in + i0)->imageOpaqueBindCount /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkImage:image
                remap_VkImage(sReassigns.reassign_VkImage, 1, (&(in[i0].pImageOpaqueBinds[i2].image)), (VkImage*)(&(out[i0].pImageOpaqueBinds[i2].image)));
                if (in[i0].pImageOpaqueBinds[i2].pBinds) {
                    memcpy((void*)out[i0].pImageOpaqueBinds[i2].pBinds, in[i0].pImageOpaqueBinds[i2].pBinds, (in[i0].pImageOpaqueBinds + i2)->bindCount /* substruct */ * sizeof(VkSparseMemoryBind));
                    for (uint32_t i4 = 0; i4 < (in[i0].pImageOpaqueBinds + i2)->bindCount /* substruct */; i4++) {
                        // TODO: reassign non-ptr handle field VkDeviceMemory:memory
                        remap_VkDeviceMemory(sReassigns.reassign_VkDeviceMemory, 1, (&(in[i0].pImageOpaqueBinds[i2].pBinds[i4].memory)), (VkDeviceMemory*)(&(out[i0].pImageOpaqueBinds[i2].pBinds[i4].memory)));
                    }
                }
            }
        }
        if (in[i0].pImageBinds) {
            memcpy((void*)out[i0].pImageBinds, in[i0].pImageBinds, (in + i0)->imageBindCount /* substruct */ * sizeof(VkSparseImageMemoryBindInfo));
            for (uint32_t i2 = 0; i2 < (in + i0)->imageBindCount /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkImage:image
                remap_VkImage(sReassigns.reassign_VkImage, 1, (&(in[i0].pImageBinds[i2].image)), (VkImage*)(&(out[i0].pImageBinds[i2].image)));
            }
        }
        if (in[i0].pSignalSemaphores) {
            // TODO: reassign ptr handle field VkSemaphore:pSignalSemaphores
            remap_VkSemaphore(sReassigns.reassign_VkSemaphore, (in + i0)->signalSemaphoreCount, in[i0].pSignalSemaphores, (VkSemaphore*)out[i0].pSignalSemaphores);
        }
    }
}

void deepCopyWithRemap_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, VkBindSparseInfo* out) {
    deepCopyAssign_VkBindSparseInfo(overall_count, in, out);
    deepCopyReassign_VkBindSparseInfo(overall_count, in, out);
}

VkBindSparseInfo* remap_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in, unsigned char* buf) {
    VkBindSparseInfo* res = initFromBuffer_VkBindSparseInfo(overall_count, in, buf);
    deepCopyAssign_VkBindSparseInfo(overall_count, in, res);
    deepCopyReassign_VkBindSparseInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkBufferViewCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkBufferViewCreateInfo* initFromBuffer_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, unsigned char* buf) {
    VkBufferViewCreateInfo* res;
    
    res = (VkBufferViewCreateInfo*)buf; buf += overall_count * sizeof(VkBufferViewCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkBufferViewCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkBufferViewCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkBuffer:buffer
        remap_VkBuffer(sReassigns.reassign_VkBuffer, 1, (&(in[i0].buffer)), (VkBuffer*)(&(out[i0].buffer)));
    }
}

void deepCopyWithRemap_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, VkBufferViewCreateInfo* out) {
    deepCopyAssign_VkBufferViewCreateInfo(overall_count, in, out);
    deepCopyReassign_VkBufferViewCreateInfo(overall_count, in, out);
}

VkBufferViewCreateInfo* remap_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in, unsigned char* buf) {
    VkBufferViewCreateInfo* res = initFromBuffer_VkBufferViewCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkBufferViewCreateInfo(overall_count, in, res);
    deepCopyReassign_VkBufferViewCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkImageViewCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkImageViewCreateInfo* initFromBuffer_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, unsigned char* buf) {
    VkImageViewCreateInfo* res;
    
    res = (VkImageViewCreateInfo*)buf; buf += overall_count * sizeof(VkImageViewCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkImageViewCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkImageViewCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkImage:image
        remap_VkImage(sReassigns.reassign_VkImage, 1, (&(in[i0].image)), (VkImage*)(&(out[i0].image)));
    }
}

void deepCopyWithRemap_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, VkImageViewCreateInfo* out) {
    deepCopyAssign_VkImageViewCreateInfo(overall_count, in, out);
    deepCopyReassign_VkImageViewCreateInfo(overall_count, in, out);
}

VkImageViewCreateInfo* remap_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in, unsigned char* buf) {
    VkImageViewCreateInfo* res = initFromBuffer_VkImageViewCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkImageViewCreateInfo(overall_count, in, res);
    deepCopyReassign_VkImageViewCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkPipelineShaderStageCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkPipelineShaderStageCreateInfo* initFromBuffer_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, unsigned char* buf) {
    VkPipelineShaderStageCreateInfo* res;
    
    res = (VkPipelineShaderStageCreateInfo*)buf; buf += overall_count * sizeof(VkPipelineShaderStageCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkPipelineShaderStageCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkPipelineShaderStageCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkShaderModule:module
        remap_VkShaderModule(sReassigns.reassign_VkShaderModule, 1, (&(in[i0].module)), (VkShaderModule*)(&(out[i0].module)));
    }
}

void deepCopyWithRemap_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, VkPipelineShaderStageCreateInfo* out) {
    deepCopyAssign_VkPipelineShaderStageCreateInfo(overall_count, in, out);
    deepCopyReassign_VkPipelineShaderStageCreateInfo(overall_count, in, out);
}

VkPipelineShaderStageCreateInfo* remap_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in, unsigned char* buf) {
    VkPipelineShaderStageCreateInfo* res = initFromBuffer_VkPipelineShaderStageCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkPipelineShaderStageCreateInfo(overall_count, in, res);
    deepCopyReassign_VkPipelineShaderStageCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkGraphicsPipelineCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pStages) {
            totalSz += (in + i0)->stageCount * sizeof(VkPipelineShaderStageCreateInfo);
        }
    }
    return totalSz;
}

VkGraphicsPipelineCreateInfo* initFromBuffer_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, unsigned char* buf) {
    VkGraphicsPipelineCreateInfo* res;
    
    res = (VkGraphicsPipelineCreateInfo*)buf; buf += overall_count * sizeof(VkGraphicsPipelineCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pStages) {
            memcpy((void*)(&res[i0].pStages), ((VkPipelineShaderStageCreateInfo**)&buf), sizeof(void*));
            buf += (in + i0)->stageCount * sizeof(VkPipelineShaderStageCreateInfo);
        }
    }
    return res;
}

void deepCopyAssign_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkGraphicsPipelineCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pStages) {
            memcpy((void*)out[i0].pStages, in[i0].pStages, (in + i0)->stageCount * sizeof(VkPipelineShaderStageCreateInfo));
        }
    }
}

void deepCopyReassign_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkGraphicsPipelineCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pStages) {
            memcpy((void*)out[i0].pStages, in[i0].pStages, (in + i0)->stageCount /* substruct */ * sizeof(VkPipelineShaderStageCreateInfo));
            for (uint32_t i2 = 0; i2 < (in + i0)->stageCount /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkShaderModule:module
                remap_VkShaderModule(sReassigns.reassign_VkShaderModule, 1, (&(in[i0].pStages[i2].module)), (VkShaderModule*)(&(out[i0].pStages[i2].module)));
            }
        }
        // TODO: reassign non-ptr handle field VkPipelineLayout:layout
        remap_VkPipelineLayout(sReassigns.reassign_VkPipelineLayout, 1, (&(in[i0].layout)), (VkPipelineLayout*)(&(out[i0].layout)));
        // TODO: reassign non-ptr handle field VkRenderPass:renderPass
        remap_VkRenderPass(sReassigns.reassign_VkRenderPass, 1, (&(in[i0].renderPass)), (VkRenderPass*)(&(out[i0].renderPass)));
        // TODO: reassign non-ptr handle field VkPipeline:basePipelineHandle
        remap_VkPipeline(sReassigns.reassign_VkPipeline, 1, (&(in[i0].basePipelineHandle)), (VkPipeline*)(&(out[i0].basePipelineHandle)));
    }
}

void deepCopyWithRemap_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, VkGraphicsPipelineCreateInfo* out) {
    deepCopyAssign_VkGraphicsPipelineCreateInfo(overall_count, in, out);
    deepCopyReassign_VkGraphicsPipelineCreateInfo(overall_count, in, out);
}

VkGraphicsPipelineCreateInfo* remap_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in, unsigned char* buf) {
    VkGraphicsPipelineCreateInfo* res = initFromBuffer_VkGraphicsPipelineCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkGraphicsPipelineCreateInfo(overall_count, in, res);
    deepCopyReassign_VkGraphicsPipelineCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkComputePipelineCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkComputePipelineCreateInfo* initFromBuffer_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, unsigned char* buf) {
    VkComputePipelineCreateInfo* res;
    
    res = (VkComputePipelineCreateInfo*)buf; buf += overall_count * sizeof(VkComputePipelineCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkComputePipelineCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
            memcpy((void*)&out[i0].stage, &in[i0].stage, 1 /* substruct */ * sizeof(VkPipelineShaderStageCreateInfo));
            for (uint32_t i2 = 0; i2 < 1 /* substruct */; i2++) {
            }
    }
}

void deepCopyReassign_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkComputePipelineCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign in substruct non-ptr
            memcpy((void*)(&(out[i0].stage)), (&(in[i0].stage)), 1 /* substruct */ * sizeof(VkPipelineShaderStageCreateInfo));
            for (uint32_t i2 = 0; i2 < 1 /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkShaderModule:module
                remap_VkShaderModule(sReassigns.reassign_VkShaderModule, 1, (&((&(in[i0].stage))[i2].module)), (VkShaderModule*)(&((&(out[i0].stage))[i2].module)));
            }
        // TODO: reassign non-ptr handle field VkPipelineLayout:layout
        remap_VkPipelineLayout(sReassigns.reassign_VkPipelineLayout, 1, (&(in[i0].layout)), (VkPipelineLayout*)(&(out[i0].layout)));
        // TODO: reassign non-ptr handle field VkPipeline:basePipelineHandle
        remap_VkPipeline(sReassigns.reassign_VkPipeline, 1, (&(in[i0].basePipelineHandle)), (VkPipeline*)(&(out[i0].basePipelineHandle)));
    }
}

void deepCopyWithRemap_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, VkComputePipelineCreateInfo* out) {
    deepCopyAssign_VkComputePipelineCreateInfo(overall_count, in, out);
    deepCopyReassign_VkComputePipelineCreateInfo(overall_count, in, out);
}

VkComputePipelineCreateInfo* remap_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in, unsigned char* buf) {
    VkComputePipelineCreateInfo* res = initFromBuffer_VkComputePipelineCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkComputePipelineCreateInfo(overall_count, in, res);
    deepCopyReassign_VkComputePipelineCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkPipelineLayoutCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pSetLayouts) {
            totalSz += (in + i0)->setLayoutCount * sizeof(VkDescriptorSetLayout);
        }
    }
    return totalSz;
}

VkPipelineLayoutCreateInfo* initFromBuffer_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, unsigned char* buf) {
    VkPipelineLayoutCreateInfo* res;
    
    res = (VkPipelineLayoutCreateInfo*)buf; buf += overall_count * sizeof(VkPipelineLayoutCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pSetLayouts) {
            memcpy((void*)(&res[i0].pSetLayouts), ((VkDescriptorSetLayout**)&buf), sizeof(void*));
            buf += (in + i0)->setLayoutCount * sizeof(VkDescriptorSetLayout);
        }
    }
    return res;
}

void deepCopyAssign_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkPipelineLayoutCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pSetLayouts) {
            memcpy((void*)out[i0].pSetLayouts, in[i0].pSetLayouts, (in + i0)->setLayoutCount * sizeof(VkDescriptorSetLayout));
        }
    }
}

void deepCopyReassign_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkPipelineLayoutCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pSetLayouts) {
            // TODO: reassign ptr handle field VkDescriptorSetLayout:pSetLayouts
            remap_VkDescriptorSetLayout(sReassigns.reassign_VkDescriptorSetLayout, (in + i0)->setLayoutCount, in[i0].pSetLayouts, (VkDescriptorSetLayout*)out[i0].pSetLayouts);
        }
    }
}

void deepCopyWithRemap_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, VkPipelineLayoutCreateInfo* out) {
    deepCopyAssign_VkPipelineLayoutCreateInfo(overall_count, in, out);
    deepCopyReassign_VkPipelineLayoutCreateInfo(overall_count, in, out);
}

VkPipelineLayoutCreateInfo* remap_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in, unsigned char* buf) {
    VkPipelineLayoutCreateInfo* res = initFromBuffer_VkPipelineLayoutCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkPipelineLayoutCreateInfo(overall_count, in, res);
    deepCopyReassign_VkPipelineLayoutCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkDescriptorSetLayoutBinding);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pImmutableSamplers) {
            totalSz += (in + i0)->descriptorCount * sizeof(VkSampler);
        }
    }
    return totalSz;
}

VkDescriptorSetLayoutBinding* initFromBuffer_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, unsigned char* buf) {
    VkDescriptorSetLayoutBinding* res;
    
    res = (VkDescriptorSetLayoutBinding*)buf; buf += overall_count * sizeof(VkDescriptorSetLayoutBinding);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pImmutableSamplers) {
            memcpy((void*)(&res[i0].pImmutableSamplers), ((VkSampler**)&buf), sizeof(void*));
            buf += (in + i0)->descriptorCount * sizeof(VkSampler);
        }
    }
    return res;
}

void deepCopyAssign_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorSetLayoutBinding));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pImmutableSamplers) {
            memcpy((void*)out[i0].pImmutableSamplers, in[i0].pImmutableSamplers, (in + i0)->descriptorCount * sizeof(VkSampler));
        }
    }
}

void deepCopyReassign_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorSetLayoutBinding));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pImmutableSamplers) {
            // TODO: reassign ptr handle field VkSampler:pImmutableSamplers
            remap_VkSampler(sReassigns.reassign_VkSampler, (in + i0)->descriptorCount, in[i0].pImmutableSamplers, (VkSampler*)out[i0].pImmutableSamplers);
        }
    }
}

void deepCopyWithRemap_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, VkDescriptorSetLayoutBinding* out) {
    deepCopyAssign_VkDescriptorSetLayoutBinding(overall_count, in, out);
    deepCopyReassign_VkDescriptorSetLayoutBinding(overall_count, in, out);
}

VkDescriptorSetLayoutBinding* remap_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in, unsigned char* buf) {
    VkDescriptorSetLayoutBinding* res = initFromBuffer_VkDescriptorSetLayoutBinding(overall_count, in, buf);
    deepCopyAssign_VkDescriptorSetLayoutBinding(overall_count, in, res);
    deepCopyReassign_VkDescriptorSetLayoutBinding(overall_count, in, res);
    return res;
}



size_t allocSz_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkDescriptorSetLayoutCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBindings) {
            totalSz += (in + i0)->bindingCount /* substruct */ * sizeof(VkDescriptorSetLayoutBinding);
            for (uint32_t i2 = 0; i2 < (in + i0)->bindingCount /* substruct */; i2++) {
                if (in[i0].pBindings[i2].pImmutableSamplers) {
                    totalSz += (in[i0].pBindings + i2)->descriptorCount * sizeof(VkSampler);
                }
            }
        }
    }
    return totalSz;
}

VkDescriptorSetLayoutCreateInfo* initFromBuffer_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, unsigned char* buf) {
    VkDescriptorSetLayoutCreateInfo* res;
    
    res = (VkDescriptorSetLayoutCreateInfo*)buf; buf += overall_count * sizeof(VkDescriptorSetLayoutCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBindings) {
            res[i0].pBindings = (VkDescriptorSetLayoutBinding*)buf; buf += (in + i0)->bindingCount /* substruct */ * sizeof(VkDescriptorSetLayoutBinding);
            for (uint32_t i2 = 0; i2 < (in + i0)->bindingCount /* substruct */; i2++) {
                if (in[i0].pBindings[i2].pImmutableSamplers) {
                    memcpy((void*)(&res[i0].pBindings[i2].pImmutableSamplers), ((VkSampler**)&buf), sizeof(void*));
                    buf += (in[i0].pBindings + i2)->descriptorCount * sizeof(VkSampler);
                }
            }
        }
    }
    return res;
}

void deepCopyAssign_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorSetLayoutCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBindings) {
            memcpy((void*)out[i0].pBindings, in[i0].pBindings, (in + i0)->bindingCount /* substruct */ * sizeof(VkDescriptorSetLayoutBinding));
            for (uint32_t i2 = 0; i2 < (in + i0)->bindingCount /* substruct */; i2++) {
                if (in[i0].pBindings[i2].pImmutableSamplers) {
                    memcpy((void*)out[i0].pBindings[i2].pImmutableSamplers, in[i0].pBindings[i2].pImmutableSamplers, (in[i0].pBindings + i2)->descriptorCount * sizeof(VkSampler));
                }
            }
        }
    }
}

void deepCopyReassign_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorSetLayoutCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pBindings) {
            memcpy((void*)out[i0].pBindings, in[i0].pBindings, (in + i0)->bindingCount /* substruct */ * sizeof(VkDescriptorSetLayoutBinding));
            for (uint32_t i2 = 0; i2 < (in + i0)->bindingCount /* substruct */; i2++) {
                if (in[i0].pBindings[i2].pImmutableSamplers) {
                    // TODO: reassign ptr handle field VkSampler:pImmutableSamplers
                    remap_VkSampler(sReassigns.reassign_VkSampler, (in[i0].pBindings + i2)->descriptorCount, in[i0].pBindings[i2].pImmutableSamplers, (VkSampler*)out[i0].pBindings[i2].pImmutableSamplers);
                }
            }
        }
    }
}

void deepCopyWithRemap_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, VkDescriptorSetLayoutCreateInfo* out) {
    deepCopyAssign_VkDescriptorSetLayoutCreateInfo(overall_count, in, out);
    deepCopyReassign_VkDescriptorSetLayoutCreateInfo(overall_count, in, out);
}

VkDescriptorSetLayoutCreateInfo* remap_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in, unsigned char* buf) {
    VkDescriptorSetLayoutCreateInfo* res = initFromBuffer_VkDescriptorSetLayoutCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkDescriptorSetLayoutCreateInfo(overall_count, in, res);
    deepCopyReassign_VkDescriptorSetLayoutCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkDescriptorSetAllocateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pSetLayouts) {
            totalSz += (in + i0)->descriptorSetCount * sizeof(VkDescriptorSetLayout);
        }
    }
    return totalSz;
}

VkDescriptorSetAllocateInfo* initFromBuffer_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, unsigned char* buf) {
    VkDescriptorSetAllocateInfo* res;
    
    res = (VkDescriptorSetAllocateInfo*)buf; buf += overall_count * sizeof(VkDescriptorSetAllocateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pSetLayouts) {
            memcpy((void*)(&res[i0].pSetLayouts), ((VkDescriptorSetLayout**)&buf), sizeof(void*));
            buf += (in + i0)->descriptorSetCount * sizeof(VkDescriptorSetLayout);
        }
    }
    return res;
}

void deepCopyAssign_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorSetAllocateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pSetLayouts) {
            memcpy((void*)out[i0].pSetLayouts, in[i0].pSetLayouts, (in + i0)->descriptorSetCount * sizeof(VkDescriptorSetLayout));
        }
    }
}

void deepCopyReassign_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorSetAllocateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkDescriptorPool:descriptorPool
        remap_VkDescriptorPool(sReassigns.reassign_VkDescriptorPool, 1, (&(in[i0].descriptorPool)), (VkDescriptorPool*)(&(out[i0].descriptorPool)));
        if (in[i0].pSetLayouts) {
            // TODO: reassign ptr handle field VkDescriptorSetLayout:pSetLayouts
            remap_VkDescriptorSetLayout(sReassigns.reassign_VkDescriptorSetLayout, (in + i0)->descriptorSetCount, in[i0].pSetLayouts, (VkDescriptorSetLayout*)out[i0].pSetLayouts);
        }
    }
}

void deepCopyWithRemap_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, VkDescriptorSetAllocateInfo* out) {
    deepCopyAssign_VkDescriptorSetAllocateInfo(overall_count, in, out);
    deepCopyReassign_VkDescriptorSetAllocateInfo(overall_count, in, out);
}

VkDescriptorSetAllocateInfo* remap_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in, unsigned char* buf) {
    VkDescriptorSetAllocateInfo* res = initFromBuffer_VkDescriptorSetAllocateInfo(overall_count, in, buf);
    deepCopyAssign_VkDescriptorSetAllocateInfo(overall_count, in, res);
    deepCopyReassign_VkDescriptorSetAllocateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkDescriptorImageInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkDescriptorImageInfo* initFromBuffer_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, unsigned char* buf) {
    VkDescriptorImageInfo* res;
    
    res = (VkDescriptorImageInfo*)buf; buf += overall_count * sizeof(VkDescriptorImageInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorImageInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorImageInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkSampler:sampler
        remap_VkSampler(sReassigns.reassign_VkSampler, 1, (&(in[i0].sampler)), (VkSampler*)(&(out[i0].sampler)));
        // TODO: reassign non-ptr handle field VkImageView:imageView
        remap_VkImageView(sReassigns.reassign_VkImageView, 1, (&(in[i0].imageView)), (VkImageView*)(&(out[i0].imageView)));
    }
}

void deepCopyWithRemap_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, VkDescriptorImageInfo* out) {
    deepCopyAssign_VkDescriptorImageInfo(overall_count, in, out);
    deepCopyReassign_VkDescriptorImageInfo(overall_count, in, out);
}

VkDescriptorImageInfo* remap_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in, unsigned char* buf) {
    VkDescriptorImageInfo* res = initFromBuffer_VkDescriptorImageInfo(overall_count, in, buf);
    deepCopyAssign_VkDescriptorImageInfo(overall_count, in, res);
    deepCopyReassign_VkDescriptorImageInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkDescriptorBufferInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkDescriptorBufferInfo* initFromBuffer_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, unsigned char* buf) {
    VkDescriptorBufferInfo* res;
    
    res = (VkDescriptorBufferInfo*)buf; buf += overall_count * sizeof(VkDescriptorBufferInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorBufferInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkDescriptorBufferInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkBuffer:buffer
        remap_VkBuffer(sReassigns.reassign_VkBuffer, 1, (&(in[i0].buffer)), (VkBuffer*)(&(out[i0].buffer)));
    }
}

void deepCopyWithRemap_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, VkDescriptorBufferInfo* out) {
    deepCopyAssign_VkDescriptorBufferInfo(overall_count, in, out);
    deepCopyReassign_VkDescriptorBufferInfo(overall_count, in, out);
}

VkDescriptorBufferInfo* remap_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in, unsigned char* buf) {
    VkDescriptorBufferInfo* res = initFromBuffer_VkDescriptorBufferInfo(overall_count, in, buf);
    deepCopyAssign_VkDescriptorBufferInfo(overall_count, in, res);
    deepCopyReassign_VkDescriptorBufferInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkWriteDescriptorSet);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pImageInfo) {
            totalSz += customCount_VkWriteDescriptorSet_pImageInfo((in + i0)) * sizeof(VkDescriptorImageInfo);
        }
        if (in[i0].pBufferInfo) {
            totalSz += customCount_VkWriteDescriptorSet_pBufferInfo((in + i0)) * sizeof(VkDescriptorBufferInfo);
        }
        if (in[i0].pTexelBufferView) {
            totalSz += customCount_VkWriteDescriptorSet_pTexelBufferView((in + i0)) * sizeof(VkBufferView);
        }
    }
    return totalSz;
}

VkWriteDescriptorSet* initFromBuffer_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, unsigned char* buf) {
    VkWriteDescriptorSet* res;
    
    res = (VkWriteDescriptorSet*)buf; buf += overall_count * sizeof(VkWriteDescriptorSet);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pImageInfo) {
            memcpy((void*)(&res[i0].pImageInfo), ((VkDescriptorImageInfo**)&buf), sizeof(void*));
            buf += customCount_VkWriteDescriptorSet_pImageInfo((in + i0)) * sizeof(VkDescriptorImageInfo);
        }
        if (in[i0].pBufferInfo) {
            memcpy((void*)(&res[i0].pBufferInfo), ((VkDescriptorBufferInfo**)&buf), sizeof(void*));
            buf += customCount_VkWriteDescriptorSet_pBufferInfo((in + i0)) * sizeof(VkDescriptorBufferInfo);
        }
        if (in[i0].pTexelBufferView) {
            memcpy((void*)(&res[i0].pTexelBufferView), ((VkBufferView**)&buf), sizeof(void*));
            buf += customCount_VkWriteDescriptorSet_pTexelBufferView((in + i0)) * sizeof(VkBufferView);
        }
    }
    return res;
}

void deepCopyAssign_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkWriteDescriptorSet));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pImageInfo) {
            memcpy((void*)out[i0].pImageInfo, in[i0].pImageInfo, customCount_VkWriteDescriptorSet_pImageInfo((in + i0)) * sizeof(VkDescriptorImageInfo));
        }
        if (in[i0].pBufferInfo) {
            memcpy((void*)out[i0].pBufferInfo, in[i0].pBufferInfo, customCount_VkWriteDescriptorSet_pBufferInfo((in + i0)) * sizeof(VkDescriptorBufferInfo));
        }
        if (in[i0].pTexelBufferView) {
            memcpy((void*)out[i0].pTexelBufferView, in[i0].pTexelBufferView, customCount_VkWriteDescriptorSet_pTexelBufferView((in + i0)) * sizeof(VkBufferView));
        }
    }
}

void deepCopyReassign_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkWriteDescriptorSet));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkDescriptorSet:dstSet
        remap_VkDescriptorSet(sReassigns.reassign_VkDescriptorSet, 1, (&(in[i0].dstSet)), (VkDescriptorSet*)(&(out[i0].dstSet)));
        if (in[i0].pImageInfo) {
            memcpy((void*)out[i0].pImageInfo, in[i0].pImageInfo, customCount_VkWriteDescriptorSet_pImageInfo((in + i0)) /* substruct */ * sizeof(VkDescriptorImageInfo));
            for (uint32_t i2 = 0; i2 < customCount_VkWriteDescriptorSet_pImageInfo((in + i0)) /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkSampler:sampler
                remap_VkSampler(sReassigns.reassign_VkSampler, 1, (&(in[i0].pImageInfo[i2].sampler)), (VkSampler*)(&(out[i0].pImageInfo[i2].sampler)));
                // TODO: reassign non-ptr handle field VkImageView:imageView
                remap_VkImageView(sReassigns.reassign_VkImageView, 1, (&(in[i0].pImageInfo[i2].imageView)), (VkImageView*)(&(out[i0].pImageInfo[i2].imageView)));
            }
        }
        if (in[i0].pBufferInfo) {
            memcpy((void*)out[i0].pBufferInfo, in[i0].pBufferInfo, customCount_VkWriteDescriptorSet_pBufferInfo((in + i0)) /* substruct */ * sizeof(VkDescriptorBufferInfo));
            for (uint32_t i2 = 0; i2 < customCount_VkWriteDescriptorSet_pBufferInfo((in + i0)) /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkBuffer:buffer
                remap_VkBuffer(sReassigns.reassign_VkBuffer, 1, (&(in[i0].pBufferInfo[i2].buffer)), (VkBuffer*)(&(out[i0].pBufferInfo[i2].buffer)));
            }
        }
        if (in[i0].pTexelBufferView) {
            // TODO: reassign ptr handle field VkBufferView:pTexelBufferView
            remap_VkBufferView(sReassigns.reassign_VkBufferView, customCount_VkWriteDescriptorSet_pTexelBufferView((in + i0)), in[i0].pTexelBufferView, (VkBufferView*)out[i0].pTexelBufferView);
        }
    }
}

void deepCopyWithRemap_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, VkWriteDescriptorSet* out) {
    deepCopyAssign_VkWriteDescriptorSet(overall_count, in, out);
    deepCopyReassign_VkWriteDescriptorSet(overall_count, in, out);
}

VkWriteDescriptorSet* remap_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in, unsigned char* buf) {
    VkWriteDescriptorSet* res = initFromBuffer_VkWriteDescriptorSet(overall_count, in, buf);
    deepCopyAssign_VkWriteDescriptorSet(overall_count, in, res);
    deepCopyReassign_VkWriteDescriptorSet(overall_count, in, res);
    return res;
}



size_t allocSz_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkCopyDescriptorSet);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkCopyDescriptorSet* initFromBuffer_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, unsigned char* buf) {
    VkCopyDescriptorSet* res;
    
    res = (VkCopyDescriptorSet*)buf; buf += overall_count * sizeof(VkCopyDescriptorSet);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCopyDescriptorSet));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCopyDescriptorSet));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkDescriptorSet:srcSet
        remap_VkDescriptorSet(sReassigns.reassign_VkDescriptorSet, 1, (&(in[i0].srcSet)), (VkDescriptorSet*)(&(out[i0].srcSet)));
        // TODO: reassign non-ptr handle field VkDescriptorSet:dstSet
        remap_VkDescriptorSet(sReassigns.reassign_VkDescriptorSet, 1, (&(in[i0].dstSet)), (VkDescriptorSet*)(&(out[i0].dstSet)));
    }
}

void deepCopyWithRemap_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, VkCopyDescriptorSet* out) {
    deepCopyAssign_VkCopyDescriptorSet(overall_count, in, out);
    deepCopyReassign_VkCopyDescriptorSet(overall_count, in, out);
}

VkCopyDescriptorSet* remap_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in, unsigned char* buf) {
    VkCopyDescriptorSet* res = initFromBuffer_VkCopyDescriptorSet(overall_count, in, buf);
    deepCopyAssign_VkCopyDescriptorSet(overall_count, in, res);
    deepCopyReassign_VkCopyDescriptorSet(overall_count, in, res);
    return res;
}



size_t allocSz_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkFramebufferCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pAttachments) {
            totalSz += (in + i0)->attachmentCount * sizeof(VkImageView);
        }
    }
    return totalSz;
}

VkFramebufferCreateInfo* initFromBuffer_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, unsigned char* buf) {
    VkFramebufferCreateInfo* res;
    
    res = (VkFramebufferCreateInfo*)buf; buf += overall_count * sizeof(VkFramebufferCreateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pAttachments) {
            memcpy((void*)(&res[i0].pAttachments), ((VkImageView**)&buf), sizeof(void*));
            buf += (in + i0)->attachmentCount * sizeof(VkImageView);
        }
    }
    return res;
}

void deepCopyAssign_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkFramebufferCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pAttachments) {
            memcpy((void*)out[i0].pAttachments, in[i0].pAttachments, (in + i0)->attachmentCount * sizeof(VkImageView));
        }
    }
}

void deepCopyReassign_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkFramebufferCreateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkRenderPass:renderPass
        remap_VkRenderPass(sReassigns.reassign_VkRenderPass, 1, (&(in[i0].renderPass)), (VkRenderPass*)(&(out[i0].renderPass)));
        if (in[i0].pAttachments) {
            // TODO: reassign ptr handle field VkImageView:pAttachments
            remap_VkImageView(sReassigns.reassign_VkImageView, (in + i0)->attachmentCount, in[i0].pAttachments, (VkImageView*)out[i0].pAttachments);
        }
    }
}

void deepCopyWithRemap_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, VkFramebufferCreateInfo* out) {
    deepCopyAssign_VkFramebufferCreateInfo(overall_count, in, out);
    deepCopyReassign_VkFramebufferCreateInfo(overall_count, in, out);
}

VkFramebufferCreateInfo* remap_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in, unsigned char* buf) {
    VkFramebufferCreateInfo* res = initFromBuffer_VkFramebufferCreateInfo(overall_count, in, buf);
    deepCopyAssign_VkFramebufferCreateInfo(overall_count, in, res);
    deepCopyReassign_VkFramebufferCreateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkCommandBufferInheritanceInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkCommandBufferInheritanceInfo* initFromBuffer_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, unsigned char* buf) {
    VkCommandBufferInheritanceInfo* res;
    
    res = (VkCommandBufferInheritanceInfo*)buf; buf += overall_count * sizeof(VkCommandBufferInheritanceInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCommandBufferInheritanceInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCommandBufferInheritanceInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkRenderPass:renderPass
        remap_VkRenderPass(sReassigns.reassign_VkRenderPass, 1, (&(in[i0].renderPass)), (VkRenderPass*)(&(out[i0].renderPass)));
        // TODO: reassign non-ptr handle field VkFramebuffer:framebuffer
        remap_VkFramebuffer(sReassigns.reassign_VkFramebuffer, 1, (&(in[i0].framebuffer)), (VkFramebuffer*)(&(out[i0].framebuffer)));
    }
}

void deepCopyWithRemap_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, VkCommandBufferInheritanceInfo* out) {
    deepCopyAssign_VkCommandBufferInheritanceInfo(overall_count, in, out);
    deepCopyReassign_VkCommandBufferInheritanceInfo(overall_count, in, out);
}

VkCommandBufferInheritanceInfo* remap_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in, unsigned char* buf) {
    VkCommandBufferInheritanceInfo* res = initFromBuffer_VkCommandBufferInheritanceInfo(overall_count, in, buf);
    deepCopyAssign_VkCommandBufferInheritanceInfo(overall_count, in, res);
    deepCopyReassign_VkCommandBufferInheritanceInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkCommandBufferAllocateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkCommandBufferAllocateInfo* initFromBuffer_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, unsigned char* buf) {
    VkCommandBufferAllocateInfo* res;
    
    res = (VkCommandBufferAllocateInfo*)buf; buf += overall_count * sizeof(VkCommandBufferAllocateInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCommandBufferAllocateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCommandBufferAllocateInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkCommandPool:commandPool
        remap_VkCommandPool(sReassigns.reassign_VkCommandPool, 1, (&(in[i0].commandPool)), (VkCommandPool*)(&(out[i0].commandPool)));
    }
}

void deepCopyWithRemap_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, VkCommandBufferAllocateInfo* out) {
    deepCopyAssign_VkCommandBufferAllocateInfo(overall_count, in, out);
    deepCopyReassign_VkCommandBufferAllocateInfo(overall_count, in, out);
}

VkCommandBufferAllocateInfo* remap_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in, unsigned char* buf) {
    VkCommandBufferAllocateInfo* res = initFromBuffer_VkCommandBufferAllocateInfo(overall_count, in, buf);
    deepCopyAssign_VkCommandBufferAllocateInfo(overall_count, in, res);
    deepCopyReassign_VkCommandBufferAllocateInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkCommandBufferBeginInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pInheritanceInfo) {
            totalSz += customCount_VkCommandBufferBeginInfo_pInheritanceInfo((in + i0)) * sizeof(VkCommandBufferInheritanceInfo);
        }
    }
    return totalSz;
}

VkCommandBufferBeginInfo* initFromBuffer_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, unsigned char* buf) {
    VkCommandBufferBeginInfo* res;
    
    res = (VkCommandBufferBeginInfo*)buf; buf += overall_count * sizeof(VkCommandBufferBeginInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pInheritanceInfo) {
            memcpy((void*)(&res[i0].pInheritanceInfo), ((VkCommandBufferInheritanceInfo**)&buf), sizeof(void*));
            buf += customCount_VkCommandBufferBeginInfo_pInheritanceInfo((in + i0)) * sizeof(VkCommandBufferInheritanceInfo);
        }
    }
    return res;
}

void deepCopyAssign_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCommandBufferBeginInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pInheritanceInfo) {
            memcpy((void*)out[i0].pInheritanceInfo, in[i0].pInheritanceInfo, customCount_VkCommandBufferBeginInfo_pInheritanceInfo((in + i0)) * sizeof(VkCommandBufferInheritanceInfo));
        }
    }
}

void deepCopyReassign_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkCommandBufferBeginInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        if (in[i0].pInheritanceInfo) {
            memcpy((void*)out[i0].pInheritanceInfo, in[i0].pInheritanceInfo, customCount_VkCommandBufferBeginInfo_pInheritanceInfo((in + i0)) /* substruct */ * sizeof(VkCommandBufferInheritanceInfo));
            for (uint32_t i2 = 0; i2 < customCount_VkCommandBufferBeginInfo_pInheritanceInfo((in + i0)) /* substruct */; i2++) {
                // TODO: reassign non-ptr handle field VkRenderPass:renderPass
                remap_VkRenderPass(sReassigns.reassign_VkRenderPass, 1, (&(in[i0].pInheritanceInfo[i2].renderPass)), (VkRenderPass*)(&(out[i0].pInheritanceInfo[i2].renderPass)));
                // TODO: reassign non-ptr handle field VkFramebuffer:framebuffer
                remap_VkFramebuffer(sReassigns.reassign_VkFramebuffer, 1, (&(in[i0].pInheritanceInfo[i2].framebuffer)), (VkFramebuffer*)(&(out[i0].pInheritanceInfo[i2].framebuffer)));
            }
        }
    }
}

void deepCopyWithRemap_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, VkCommandBufferBeginInfo* out) {
    deepCopyAssign_VkCommandBufferBeginInfo(overall_count, in, out);
    deepCopyReassign_VkCommandBufferBeginInfo(overall_count, in, out);
}

VkCommandBufferBeginInfo* remap_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in, unsigned char* buf) {
    VkCommandBufferBeginInfo* res = initFromBuffer_VkCommandBufferBeginInfo(overall_count, in, buf);
    deepCopyAssign_VkCommandBufferBeginInfo(overall_count, in, res);
    deepCopyReassign_VkCommandBufferBeginInfo(overall_count, in, res);
    return res;
}



size_t allocSz_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkBufferMemoryBarrier);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkBufferMemoryBarrier* initFromBuffer_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, unsigned char* buf) {
    VkBufferMemoryBarrier* res;
    
    res = (VkBufferMemoryBarrier*)buf; buf += overall_count * sizeof(VkBufferMemoryBarrier);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkBufferMemoryBarrier));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkBufferMemoryBarrier));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkBuffer:buffer
        remap_VkBuffer(sReassigns.reassign_VkBuffer, 1, (&(in[i0].buffer)), (VkBuffer*)(&(out[i0].buffer)));
    }
}

void deepCopyWithRemap_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, VkBufferMemoryBarrier* out) {
    deepCopyAssign_VkBufferMemoryBarrier(overall_count, in, out);
    deepCopyReassign_VkBufferMemoryBarrier(overall_count, in, out);
}

VkBufferMemoryBarrier* remap_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in, unsigned char* buf) {
    VkBufferMemoryBarrier* res = initFromBuffer_VkBufferMemoryBarrier(overall_count, in, buf);
    deepCopyAssign_VkBufferMemoryBarrier(overall_count, in, res);
    deepCopyReassign_VkBufferMemoryBarrier(overall_count, in, res);
    return res;
}



size_t allocSz_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkImageMemoryBarrier);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkImageMemoryBarrier* initFromBuffer_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, unsigned char* buf) {
    VkImageMemoryBarrier* res;
    
    res = (VkImageMemoryBarrier*)buf; buf += overall_count * sizeof(VkImageMemoryBarrier);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkImageMemoryBarrier));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkImageMemoryBarrier));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkImage:image
        remap_VkImage(sReassigns.reassign_VkImage, 1, (&(in[i0].image)), (VkImage*)(&(out[i0].image)));
    }
}

void deepCopyWithRemap_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, VkImageMemoryBarrier* out) {
    deepCopyAssign_VkImageMemoryBarrier(overall_count, in, out);
    deepCopyReassign_VkImageMemoryBarrier(overall_count, in, out);
}

VkImageMemoryBarrier* remap_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in, unsigned char* buf) {
    VkImageMemoryBarrier* res = initFromBuffer_VkImageMemoryBarrier(overall_count, in, buf);
    deepCopyAssign_VkImageMemoryBarrier(overall_count, in, res);
    deepCopyReassign_VkImageMemoryBarrier(overall_count, in, res);
    return res;
}



size_t allocSz_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in) {
    size_t totalSz = 0;
    if (!in) return 0;
    
    totalSz += overall_count * sizeof(VkRenderPassBeginInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return totalSz;
}

VkRenderPassBeginInfo* initFromBuffer_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, unsigned char* buf) {
    VkRenderPassBeginInfo* res;
    
    res = (VkRenderPassBeginInfo*)buf; buf += overall_count * sizeof(VkRenderPassBeginInfo);
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
    return res;
}

void deepCopyAssign_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkRenderPassBeginInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
    }
}

void deepCopyReassign_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out) {
    if (!in) return;
    
    memcpy((void*)out, in, overall_count * sizeof(VkRenderPassBeginInfo));
    for (uint32_t i0 = 0; i0 < overall_count; i0++) {
        // TODO: reassign non-ptr handle field VkRenderPass:renderPass
        remap_VkRenderPass(sReassigns.reassign_VkRenderPass, 1, (&(in[i0].renderPass)), (VkRenderPass*)(&(out[i0].renderPass)));
        // TODO: reassign non-ptr handle field VkFramebuffer:framebuffer
        remap_VkFramebuffer(sReassigns.reassign_VkFramebuffer, 1, (&(in[i0].framebuffer)), (VkFramebuffer*)(&(out[i0].framebuffer)));
    }
}

void deepCopyWithRemap_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, VkRenderPassBeginInfo* out) {
    deepCopyAssign_VkRenderPassBeginInfo(overall_count, in, out);
    deepCopyReassign_VkRenderPassBeginInfo(overall_count, in, out);
}

VkRenderPassBeginInfo* remap_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in, unsigned char* buf) {
    VkRenderPassBeginInfo* res = initFromBuffer_VkRenderPassBeginInfo(overall_count, in, buf);
    deepCopyAssign_VkRenderPassBeginInfo(overall_count, in, res);
    deepCopyReassign_VkRenderPassBeginInfo(overall_count, in, res);
    return res;
}


