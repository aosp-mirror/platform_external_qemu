
#include "marshaling.h"

#include "vkUtils_custom.h"

static vk_reassign_funcs sReassigns = {};

void registerReassignmentFuncs(vk_reassign_funcs* funcs) {
    memcpy(&sReassigns, funcs, sizeof(vk_reassign_funcs));
}



size_t tmpAllocSz_VkMappedMemoryRange(uint32_t overall_count, const VkMappedMemoryRange* in) {
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



size_t tmpAllocSz_VkSubmitInfo(uint32_t overall_count, const VkSubmitInfo* in) {
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



size_t tmpAllocSz_VkSparseMemoryBind(uint32_t overall_count, const VkSparseMemoryBind* in) {
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



size_t tmpAllocSz_VkSparseBufferMemoryBindInfo(uint32_t overall_count, const VkSparseBufferMemoryBindInfo* in) {
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



size_t tmpAllocSz_VkSparseImageOpaqueMemoryBindInfo(uint32_t overall_count, const VkSparseImageOpaqueMemoryBindInfo* in) {
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



size_t tmpAllocSz_VkSparseImageMemoryBindInfo(uint32_t overall_count, const VkSparseImageMemoryBindInfo* in) {
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



size_t tmpAllocSz_VkBindSparseInfo(uint32_t overall_count, const VkBindSparseInfo* in) {
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



size_t tmpAllocSz_VkBufferViewCreateInfo(uint32_t overall_count, const VkBufferViewCreateInfo* in) {
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



size_t tmpAllocSz_VkImageViewCreateInfo(uint32_t overall_count, const VkImageViewCreateInfo* in) {
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



size_t tmpAllocSz_VkPipelineShaderStageCreateInfo(uint32_t overall_count, const VkPipelineShaderStageCreateInfo* in) {
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



size_t tmpAllocSz_VkGraphicsPipelineCreateInfo(uint32_t overall_count, const VkGraphicsPipelineCreateInfo* in) {
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



size_t tmpAllocSz_VkComputePipelineCreateInfo(uint32_t overall_count, const VkComputePipelineCreateInfo* in) {
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



size_t tmpAllocSz_VkPipelineLayoutCreateInfo(uint32_t overall_count, const VkPipelineLayoutCreateInfo* in) {
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



size_t tmpAllocSz_VkDescriptorSetLayoutBinding(uint32_t overall_count, const VkDescriptorSetLayoutBinding* in) {
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



size_t tmpAllocSz_VkDescriptorSetLayoutCreateInfo(uint32_t overall_count, const VkDescriptorSetLayoutCreateInfo* in) {
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



size_t tmpAllocSz_VkDescriptorSetAllocateInfo(uint32_t overall_count, const VkDescriptorSetAllocateInfo* in) {
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



size_t tmpAllocSz_VkDescriptorImageInfo(uint32_t overall_count, const VkDescriptorImageInfo* in) {
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



size_t tmpAllocSz_VkDescriptorBufferInfo(uint32_t overall_count, const VkDescriptorBufferInfo* in) {
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



size_t tmpAllocSz_VkWriteDescriptorSet(uint32_t overall_count, const VkWriteDescriptorSet* in) {
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



size_t tmpAllocSz_VkCopyDescriptorSet(uint32_t overall_count, const VkCopyDescriptorSet* in) {
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



size_t tmpAllocSz_VkFramebufferCreateInfo(uint32_t overall_count, const VkFramebufferCreateInfo* in) {
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



size_t tmpAllocSz_VkCommandBufferInheritanceInfo(uint32_t overall_count, const VkCommandBufferInheritanceInfo* in) {
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



size_t tmpAllocSz_VkCommandBufferAllocateInfo(uint32_t overall_count, const VkCommandBufferAllocateInfo* in) {
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



size_t tmpAllocSz_VkCommandBufferBeginInfo(uint32_t overall_count, const VkCommandBufferBeginInfo* in) {
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



size_t tmpAllocSz_VkBufferMemoryBarrier(uint32_t overall_count, const VkBufferMemoryBarrier* in) {
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



size_t tmpAllocSz_VkImageMemoryBarrier(uint32_t overall_count, const VkImageMemoryBarrier* in) {
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



size_t tmpAllocSz_VkRenderPassBeginInfo(uint32_t overall_count, const VkRenderPassBeginInfo* in) {
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


