
#include "handle_reassignment.h"
#include "marshaling.h"

#include <vulkan/vulkan.h>

#include <vector>

#include <stdio.h>
#include <string.h>



VkMappedMemoryRange* genTestData_VkMappedMemoryRange() {
    uint32_t count = 4;
    VkMappedMemoryRange* res = nullptr;

    VkMappedMemoryRange* tmp_VkMappedMemoryRange = new VkMappedMemoryRange[count];
    memset(tmp_VkMappedMemoryRange, 0xabcddcba, count * sizeof(VkMappedMemoryRange));
    res = tmp_VkMappedMemoryRange;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkMappedMemoryRange(uint32_t count, const VkMappedMemoryRange* a, const VkMappedMemoryRange* b) {
    bool res = true;
    
    int cmpRes_VkMappedMemoryRange = memcmp(a, b, count * sizeof(VkMappedMemoryRange)); if (cmpRes_VkMappedMemoryRange) { fprintf(stderr, "ERROR compare VkMappedMemoryRange: %d\n", cmpRes_VkMappedMemoryRange); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkMappedMemoryRange(uint32_t count, const VkMappedMemoryRange* a, const VkMappedMemoryRange* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].memory))[i1] != 0x0) { fprintf(stderr, "ERROR VkDeviceMemory:memory handle not remapped to 0 %p\n", (&(b[i0].memory))[i1]); }
        }
    }
    return res;
}


VkSubmitInfo* genTestData_VkSubmitInfo() {
    uint32_t count = 4;
    VkSubmitInfo* res = nullptr;

    VkSubmitInfo* tmp_VkSubmitInfo = new VkSubmitInfo[count];
    memset(tmp_VkSubmitInfo, 0xabcddcba, count * sizeof(VkSubmitInfo));
    res = tmp_VkSubmitInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkSubmitInfo + i0)->waitSemaphoreCount = 4;
        if ((res + i0)->waitSemaphoreCount) {
            VkSemaphore* tmp_VkSemaphore = new VkSemaphore[(res + i0)->waitSemaphoreCount];
            memset(tmp_VkSemaphore, 0xabcddcba, (res + i0)->waitSemaphoreCount * sizeof(VkSemaphore));
            memcpy((void*)&res[i0].pWaitSemaphores, (void**)(&tmp_VkSemaphore), sizeof(void*));
        }
        (tmp_VkSubmitInfo + i0)->commandBufferCount = 4;
        if ((res + i0)->commandBufferCount) {
            VkCommandBuffer* tmp_VkCommandBuffer = new VkCommandBuffer[(res + i0)->commandBufferCount];
            memset(tmp_VkCommandBuffer, 0xabcddcba, (res + i0)->commandBufferCount * sizeof(VkCommandBuffer));
            memcpy((void*)&res[i0].pCommandBuffers, (void**)(&tmp_VkCommandBuffer), sizeof(void*));
        }
        (tmp_VkSubmitInfo + i0)->signalSemaphoreCount = 4;
        if ((res + i0)->signalSemaphoreCount) {
            VkSemaphore* tmp_VkSemaphore = new VkSemaphore[(res + i0)->signalSemaphoreCount];
            memset(tmp_VkSemaphore, 0xabcddcba, (res + i0)->signalSemaphoreCount * sizeof(VkSemaphore));
            memcpy((void*)&res[i0].pSignalSemaphores, (void**)(&tmp_VkSemaphore), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkSubmitInfo(uint32_t count, const VkSubmitInfo* a, const VkSubmitInfo* b) {
    bool res = true;
    
    int cmpRes_VkSubmitInfo = memcmp(a, b, count * sizeof(VkSubmitInfo)); if (cmpRes_VkSubmitInfo) { fprintf(stderr, "ERROR compare VkSubmitInfo: %d\n", cmpRes_VkSubmitInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->waitSemaphoreCount != (b + i0)->waitSemaphoreCount) { fprintf(stderr, "ERROR compare counts VkSubmitInfo:VkSemaphore %u %u\n", (a + i0)->waitSemaphoreCount, (b + i0)->waitSemaphoreCount); res = false; } 
        if ((a + i0)->commandBufferCount != (b + i0)->commandBufferCount) { fprintf(stderr, "ERROR compare counts VkSubmitInfo:VkCommandBuffer %u %u\n", (a + i0)->commandBufferCount, (b + i0)->commandBufferCount); res = false; } 
        if ((a + i0)->signalSemaphoreCount != (b + i0)->signalSemaphoreCount) { fprintf(stderr, "ERROR compare counts VkSubmitInfo:VkSemaphore %u %u\n", (a + i0)->signalSemaphoreCount, (b + i0)->signalSemaphoreCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkSubmitInfo(uint32_t count, const VkSubmitInfo* a, const VkSubmitInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < (a + i0)->waitSemaphoreCount; i1++) {
        if (b[i0].pWaitSemaphores[i1] != 0x0) { fprintf(stderr, "ERROR VkSemaphore:pWaitSemaphores handle not remapped to 0 %p\n", b[i0].pWaitSemaphores[i1]); }
        }
        for (uint32_t i1 = 0; i1 < (a + i0)->commandBufferCount; i1++) {
        if (b[i0].pCommandBuffers[i1] != 0x0) { fprintf(stderr, "ERROR VkCommandBuffer:pCommandBuffers handle not remapped to 0 %p\n", b[i0].pCommandBuffers[i1]); }
        }
        for (uint32_t i1 = 0; i1 < (a + i0)->signalSemaphoreCount; i1++) {
        if (b[i0].pSignalSemaphores[i1] != 0x0) { fprintf(stderr, "ERROR VkSemaphore:pSignalSemaphores handle not remapped to 0 %p\n", b[i0].pSignalSemaphores[i1]); }
        }
    }
    return res;
}


VkSparseMemoryBind* genTestData_VkSparseMemoryBind() {
    uint32_t count = 4;
    VkSparseMemoryBind* res = nullptr;

    VkSparseMemoryBind* tmp_VkSparseMemoryBind = new VkSparseMemoryBind[count];
    memset(tmp_VkSparseMemoryBind, 0xabcddcba, count * sizeof(VkSparseMemoryBind));
    res = tmp_VkSparseMemoryBind;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkSparseMemoryBind(uint32_t count, const VkSparseMemoryBind* a, const VkSparseMemoryBind* b) {
    bool res = true;
    
    int cmpRes_VkSparseMemoryBind = memcmp(a, b, count * sizeof(VkSparseMemoryBind)); if (cmpRes_VkSparseMemoryBind) { fprintf(stderr, "ERROR compare VkSparseMemoryBind: %d\n", cmpRes_VkSparseMemoryBind); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkSparseMemoryBind(uint32_t count, const VkSparseMemoryBind* a, const VkSparseMemoryBind* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].memory))[i1] != 0x0) { fprintf(stderr, "ERROR VkDeviceMemory:memory handle not remapped to 0 %p\n", (&(b[i0].memory))[i1]); }
        }
    }
    return res;
}


VkSparseBufferMemoryBindInfo* genTestData_VkSparseBufferMemoryBindInfo() {
    uint32_t count = 4;
    VkSparseBufferMemoryBindInfo* res = nullptr;

    VkSparseBufferMemoryBindInfo* tmp_VkSparseBufferMemoryBindInfo = new VkSparseBufferMemoryBindInfo[count];
    memset(tmp_VkSparseBufferMemoryBindInfo, 0xabcddcba, count * sizeof(VkSparseBufferMemoryBindInfo));
    res = tmp_VkSparseBufferMemoryBindInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkSparseBufferMemoryBindInfo + i0)->bindCount = 4;
        if ((res + i0)->bindCount) {
            VkSparseMemoryBind* tmp_VkSparseMemoryBind = new VkSparseMemoryBind[(res + i0)->bindCount];
            memset(tmp_VkSparseMemoryBind, 0xabcddcba, (res + i0)->bindCount * sizeof(VkSparseMemoryBind));
            memcpy((void*)&res[i0].pBinds, (void**)(&tmp_VkSparseMemoryBind), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkSparseBufferMemoryBindInfo(uint32_t count, const VkSparseBufferMemoryBindInfo* a, const VkSparseBufferMemoryBindInfo* b) {
    bool res = true;
    
    int cmpRes_VkSparseBufferMemoryBindInfo = memcmp(a, b, count * sizeof(VkSparseBufferMemoryBindInfo)); if (cmpRes_VkSparseBufferMemoryBindInfo) { fprintf(stderr, "ERROR compare VkSparseBufferMemoryBindInfo: %d\n", cmpRes_VkSparseBufferMemoryBindInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->bindCount != (b + i0)->bindCount) { fprintf(stderr, "ERROR compare counts VkSparseBufferMemoryBindInfo:VkSparseMemoryBind %u %u\n", (a + i0)->bindCount, (b + i0)->bindCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkSparseBufferMemoryBindInfo(uint32_t count, const VkSparseBufferMemoryBindInfo* a, const VkSparseBufferMemoryBindInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].buffer))[i1] != 0x0) { fprintf(stderr, "ERROR VkBuffer:buffer handle not remapped to 0 %p\n", (&(b[i0].buffer))[i1]); }
        }
            for (uint32_t i2 = 0; i2 < (a + i0)->bindCount /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < 1; i3++) {
                if ((&(b[i0].pBinds[i2].memory))[i3] != 0x0) { fprintf(stderr, "ERROR VkDeviceMemory:memory handle not remapped to 0 %p\n", (&(b[i0].pBinds[i2].memory))[i3]); }
                }
            }
    }
    return res;
}


VkSparseImageOpaqueMemoryBindInfo* genTestData_VkSparseImageOpaqueMemoryBindInfo() {
    uint32_t count = 4;
    VkSparseImageOpaqueMemoryBindInfo* res = nullptr;

    VkSparseImageOpaqueMemoryBindInfo* tmp_VkSparseImageOpaqueMemoryBindInfo = new VkSparseImageOpaqueMemoryBindInfo[count];
    memset(tmp_VkSparseImageOpaqueMemoryBindInfo, 0xabcddcba, count * sizeof(VkSparseImageOpaqueMemoryBindInfo));
    res = tmp_VkSparseImageOpaqueMemoryBindInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkSparseImageOpaqueMemoryBindInfo + i0)->bindCount = 4;
        if ((res + i0)->bindCount) {
            VkSparseMemoryBind* tmp_VkSparseMemoryBind = new VkSparseMemoryBind[(res + i0)->bindCount];
            memset(tmp_VkSparseMemoryBind, 0xabcddcba, (res + i0)->bindCount * sizeof(VkSparseMemoryBind));
            memcpy((void*)&res[i0].pBinds, (void**)(&tmp_VkSparseMemoryBind), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkSparseImageOpaqueMemoryBindInfo(uint32_t count, const VkSparseImageOpaqueMemoryBindInfo* a, const VkSparseImageOpaqueMemoryBindInfo* b) {
    bool res = true;
    
    int cmpRes_VkSparseImageOpaqueMemoryBindInfo = memcmp(a, b, count * sizeof(VkSparseImageOpaqueMemoryBindInfo)); if (cmpRes_VkSparseImageOpaqueMemoryBindInfo) { fprintf(stderr, "ERROR compare VkSparseImageOpaqueMemoryBindInfo: %d\n", cmpRes_VkSparseImageOpaqueMemoryBindInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->bindCount != (b + i0)->bindCount) { fprintf(stderr, "ERROR compare counts VkSparseImageOpaqueMemoryBindInfo:VkSparseMemoryBind %u %u\n", (a + i0)->bindCount, (b + i0)->bindCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkSparseImageOpaqueMemoryBindInfo(uint32_t count, const VkSparseImageOpaqueMemoryBindInfo* a, const VkSparseImageOpaqueMemoryBindInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].image))[i1] != 0x0) { fprintf(stderr, "ERROR VkImage:image handle not remapped to 0 %p\n", (&(b[i0].image))[i1]); }
        }
            for (uint32_t i2 = 0; i2 < (a + i0)->bindCount /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < 1; i3++) {
                if ((&(b[i0].pBinds[i2].memory))[i3] != 0x0) { fprintf(stderr, "ERROR VkDeviceMemory:memory handle not remapped to 0 %p\n", (&(b[i0].pBinds[i2].memory))[i3]); }
                }
            }
    }
    return res;
}


VkSparseImageMemoryBindInfo* genTestData_VkSparseImageMemoryBindInfo() {
    uint32_t count = 4;
    VkSparseImageMemoryBindInfo* res = nullptr;

    VkSparseImageMemoryBindInfo* tmp_VkSparseImageMemoryBindInfo = new VkSparseImageMemoryBindInfo[count];
    memset(tmp_VkSparseImageMemoryBindInfo, 0xabcddcba, count * sizeof(VkSparseImageMemoryBindInfo));
    res = tmp_VkSparseImageMemoryBindInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkSparseImageMemoryBindInfo(uint32_t count, const VkSparseImageMemoryBindInfo* a, const VkSparseImageMemoryBindInfo* b) {
    bool res = true;
    
    int cmpRes_VkSparseImageMemoryBindInfo = memcmp(a, b, count * sizeof(VkSparseImageMemoryBindInfo)); if (cmpRes_VkSparseImageMemoryBindInfo) { fprintf(stderr, "ERROR compare VkSparseImageMemoryBindInfo: %d\n", cmpRes_VkSparseImageMemoryBindInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkSparseImageMemoryBindInfo(uint32_t count, const VkSparseImageMemoryBindInfo* a, const VkSparseImageMemoryBindInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].image))[i1] != 0x0) { fprintf(stderr, "ERROR VkImage:image handle not remapped to 0 %p\n", (&(b[i0].image))[i1]); }
        }
    }
    return res;
}


VkBindSparseInfo* genTestData_VkBindSparseInfo() {
    uint32_t count = 4;
    VkBindSparseInfo* res = nullptr;

    VkBindSparseInfo* tmp_VkBindSparseInfo = new VkBindSparseInfo[count];
    memset(tmp_VkBindSparseInfo, 0xabcddcba, count * sizeof(VkBindSparseInfo));
    res = tmp_VkBindSparseInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkBindSparseInfo + i0)->waitSemaphoreCount = 4;
        if ((res + i0)->waitSemaphoreCount) {
            VkSemaphore* tmp_VkSemaphore = new VkSemaphore[(res + i0)->waitSemaphoreCount];
            memset(tmp_VkSemaphore, 0xabcddcba, (res + i0)->waitSemaphoreCount * sizeof(VkSemaphore));
            memcpy((void*)&res[i0].pWaitSemaphores, (void**)(&tmp_VkSemaphore), sizeof(void*));
        }
        (tmp_VkBindSparseInfo + i0)->bufferBindCount = 4;
        if ((res + i0)->bufferBindCount) {
            VkSparseBufferMemoryBindInfo* tmp_VkSparseBufferMemoryBindInfo = new VkSparseBufferMemoryBindInfo[(res + i0)->bufferBindCount /* substruct */];
            memset(tmp_VkSparseBufferMemoryBindInfo, 0xabcddcba, (res + i0)->bufferBindCount /* substruct */ * sizeof(VkSparseBufferMemoryBindInfo));
            res[i0].pBufferBinds = tmp_VkSparseBufferMemoryBindInfo;
            for (uint32_t i2 = 0; i2 < (res + i0)->bufferBindCount /* substruct */; i2++) {
                (tmp_VkSparseBufferMemoryBindInfo + i2)->bindCount = 4;
                if ((res[i0].pBufferBinds + i2)->bindCount) {
                    VkSparseMemoryBind* tmp_VkSparseMemoryBind = new VkSparseMemoryBind[(res[i0].pBufferBinds + i2)->bindCount];
                    memset(tmp_VkSparseMemoryBind, 0xabcddcba, (res[i0].pBufferBinds + i2)->bindCount * sizeof(VkSparseMemoryBind));
                    memcpy((void*)&res[i0].pBufferBinds[i2].pBinds, (void**)(&tmp_VkSparseMemoryBind), sizeof(void*));
                }
            }
        }
        (tmp_VkBindSparseInfo + i0)->imageOpaqueBindCount = 4;
        if ((res + i0)->imageOpaqueBindCount) {
            VkSparseImageOpaqueMemoryBindInfo* tmp_VkSparseImageOpaqueMemoryBindInfo = new VkSparseImageOpaqueMemoryBindInfo[(res + i0)->imageOpaqueBindCount /* substruct */];
            memset(tmp_VkSparseImageOpaqueMemoryBindInfo, 0xabcddcba, (res + i0)->imageOpaqueBindCount /* substruct */ * sizeof(VkSparseImageOpaqueMemoryBindInfo));
            res[i0].pImageOpaqueBinds = tmp_VkSparseImageOpaqueMemoryBindInfo;
            for (uint32_t i2 = 0; i2 < (res + i0)->imageOpaqueBindCount /* substruct */; i2++) {
                (tmp_VkSparseImageOpaqueMemoryBindInfo + i2)->bindCount = 4;
                if ((res[i0].pImageOpaqueBinds + i2)->bindCount) {
                    VkSparseMemoryBind* tmp_VkSparseMemoryBind = new VkSparseMemoryBind[(res[i0].pImageOpaqueBinds + i2)->bindCount];
                    memset(tmp_VkSparseMemoryBind, 0xabcddcba, (res[i0].pImageOpaqueBinds + i2)->bindCount * sizeof(VkSparseMemoryBind));
                    memcpy((void*)&res[i0].pImageOpaqueBinds[i2].pBinds, (void**)(&tmp_VkSparseMemoryBind), sizeof(void*));
                }
            }
        }
        (tmp_VkBindSparseInfo + i0)->imageBindCount = 4;
        if ((res + i0)->imageBindCount) {
            VkSparseImageMemoryBindInfo* tmp_VkSparseImageMemoryBindInfo = new VkSparseImageMemoryBindInfo[(res + i0)->imageBindCount];
            memset(tmp_VkSparseImageMemoryBindInfo, 0xabcddcba, (res + i0)->imageBindCount * sizeof(VkSparseImageMemoryBindInfo));
            memcpy((void*)&res[i0].pImageBinds, (void**)(&tmp_VkSparseImageMemoryBindInfo), sizeof(void*));
        }
        (tmp_VkBindSparseInfo + i0)->signalSemaphoreCount = 4;
        if ((res + i0)->signalSemaphoreCount) {
            VkSemaphore* tmp_VkSemaphore = new VkSemaphore[(res + i0)->signalSemaphoreCount];
            memset(tmp_VkSemaphore, 0xabcddcba, (res + i0)->signalSemaphoreCount * sizeof(VkSemaphore));
            memcpy((void*)&res[i0].pSignalSemaphores, (void**)(&tmp_VkSemaphore), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkBindSparseInfo(uint32_t count, const VkBindSparseInfo* a, const VkBindSparseInfo* b) {
    bool res = true;
    
    int cmpRes_VkBindSparseInfo = memcmp(a, b, count * sizeof(VkBindSparseInfo)); if (cmpRes_VkBindSparseInfo) { fprintf(stderr, "ERROR compare VkBindSparseInfo: %d\n", cmpRes_VkBindSparseInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->waitSemaphoreCount != (b + i0)->waitSemaphoreCount) { fprintf(stderr, "ERROR compare counts VkBindSparseInfo:VkSemaphore %u %u\n", (a + i0)->waitSemaphoreCount, (b + i0)->waitSemaphoreCount); res = false; } 
        if ((a + i0)->bufferBindCount != (b + i0)->bufferBindCount) { fprintf(stderr, "ERROR compare counts VkBindSparseInfo:VkSparseBufferMemoryBindInfo %u %u\n", (a + i0)->bufferBindCount, (b + i0)->bufferBindCount); res = false; } 
    int cmpRes_VkSparseBufferMemoryBindInfo = memcmp(a[i0].pBufferBinds, b[i0].pBufferBinds, (a + i0)->bufferBindCount /* substruct */ * sizeof(VkSparseBufferMemoryBindInfo)); if (cmpRes_VkSparseBufferMemoryBindInfo) { fprintf(stderr, "ERROR compare VkSparseBufferMemoryBindInfo: %d\n", cmpRes_VkSparseBufferMemoryBindInfo); }
            for (uint32_t i2 = 0; i2 < (a + i0)->bufferBindCount /* substruct */; i2++) {
                if ((a[i0].pBufferBinds + i2)->bindCount != (b[i0].pBufferBinds + i2)->bindCount) { fprintf(stderr, "ERROR compare counts VkSparseBufferMemoryBindInfo:VkSparseMemoryBind %u %u\n", (a[i0].pBufferBinds + i2)->bindCount, (b[i0].pBufferBinds + i2)->bindCount); res = false; } 
            }
        if ((a + i0)->imageOpaqueBindCount != (b + i0)->imageOpaqueBindCount) { fprintf(stderr, "ERROR compare counts VkBindSparseInfo:VkSparseImageOpaqueMemoryBindInfo %u %u\n", (a + i0)->imageOpaqueBindCount, (b + i0)->imageOpaqueBindCount); res = false; } 
    int cmpRes_VkSparseImageOpaqueMemoryBindInfo = memcmp(a[i0].pImageOpaqueBinds, b[i0].pImageOpaqueBinds, (a + i0)->imageOpaqueBindCount /* substruct */ * sizeof(VkSparseImageOpaqueMemoryBindInfo)); if (cmpRes_VkSparseImageOpaqueMemoryBindInfo) { fprintf(stderr, "ERROR compare VkSparseImageOpaqueMemoryBindInfo: %d\n", cmpRes_VkSparseImageOpaqueMemoryBindInfo); }
            for (uint32_t i2 = 0; i2 < (a + i0)->imageOpaqueBindCount /* substruct */; i2++) {
                if ((a[i0].pImageOpaqueBinds + i2)->bindCount != (b[i0].pImageOpaqueBinds + i2)->bindCount) { fprintf(stderr, "ERROR compare counts VkSparseImageOpaqueMemoryBindInfo:VkSparseMemoryBind %u %u\n", (a[i0].pImageOpaqueBinds + i2)->bindCount, (b[i0].pImageOpaqueBinds + i2)->bindCount); res = false; } 
            }
        if ((a + i0)->imageBindCount != (b + i0)->imageBindCount) { fprintf(stderr, "ERROR compare counts VkBindSparseInfo:VkSparseImageMemoryBindInfo %u %u\n", (a + i0)->imageBindCount, (b + i0)->imageBindCount); res = false; } 
        if ((a + i0)->signalSemaphoreCount != (b + i0)->signalSemaphoreCount) { fprintf(stderr, "ERROR compare counts VkBindSparseInfo:VkSemaphore %u %u\n", (a + i0)->signalSemaphoreCount, (b + i0)->signalSemaphoreCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkBindSparseInfo(uint32_t count, const VkBindSparseInfo* a, const VkBindSparseInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < (a + i0)->waitSemaphoreCount; i1++) {
        if (b[i0].pWaitSemaphores[i1] != 0x0) { fprintf(stderr, "ERROR VkSemaphore:pWaitSemaphores handle not remapped to 0 %p\n", b[i0].pWaitSemaphores[i1]); }
        }
            for (uint32_t i2 = 0; i2 < (a + i0)->bufferBindCount /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < 1; i3++) {
                if ((&(b[i0].pBufferBinds[i2].buffer))[i3] != 0x0) { fprintf(stderr, "ERROR VkBuffer:buffer handle not remapped to 0 %p\n", (&(b[i0].pBufferBinds[i2].buffer))[i3]); }
                }
                    for (uint32_t i4 = 0; i4 < (a[i0].pBufferBinds + i2)->bindCount /* substruct */; i4++) {
                        for (uint32_t i5 = 0; i5 < 1; i5++) {
                        if ((&(b[i0].pBufferBinds[i2].pBinds[i4].memory))[i5] != 0x0) { fprintf(stderr, "ERROR VkDeviceMemory:memory handle not remapped to 0 %p\n", (&(b[i0].pBufferBinds[i2].pBinds[i4].memory))[i5]); }
                        }
                    }
            }
            for (uint32_t i2 = 0; i2 < (a + i0)->imageOpaqueBindCount /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < 1; i3++) {
                if ((&(b[i0].pImageOpaqueBinds[i2].image))[i3] != 0x0) { fprintf(stderr, "ERROR VkImage:image handle not remapped to 0 %p\n", (&(b[i0].pImageOpaqueBinds[i2].image))[i3]); }
                }
                    for (uint32_t i4 = 0; i4 < (a[i0].pImageOpaqueBinds + i2)->bindCount /* substruct */; i4++) {
                        for (uint32_t i5 = 0; i5 < 1; i5++) {
                        if ((&(b[i0].pImageOpaqueBinds[i2].pBinds[i4].memory))[i5] != 0x0) { fprintf(stderr, "ERROR VkDeviceMemory:memory handle not remapped to 0 %p\n", (&(b[i0].pImageOpaqueBinds[i2].pBinds[i4].memory))[i5]); }
                        }
                    }
            }
            for (uint32_t i2 = 0; i2 < (a + i0)->imageBindCount /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < 1; i3++) {
                if ((&(b[i0].pImageBinds[i2].image))[i3] != 0x0) { fprintf(stderr, "ERROR VkImage:image handle not remapped to 0 %p\n", (&(b[i0].pImageBinds[i2].image))[i3]); }
                }
            }
        for (uint32_t i1 = 0; i1 < (a + i0)->signalSemaphoreCount; i1++) {
        if (b[i0].pSignalSemaphores[i1] != 0x0) { fprintf(stderr, "ERROR VkSemaphore:pSignalSemaphores handle not remapped to 0 %p\n", b[i0].pSignalSemaphores[i1]); }
        }
    }
    return res;
}


VkBufferViewCreateInfo* genTestData_VkBufferViewCreateInfo() {
    uint32_t count = 4;
    VkBufferViewCreateInfo* res = nullptr;

    VkBufferViewCreateInfo* tmp_VkBufferViewCreateInfo = new VkBufferViewCreateInfo[count];
    memset(tmp_VkBufferViewCreateInfo, 0xabcddcba, count * sizeof(VkBufferViewCreateInfo));
    res = tmp_VkBufferViewCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkBufferViewCreateInfo(uint32_t count, const VkBufferViewCreateInfo* a, const VkBufferViewCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkBufferViewCreateInfo = memcmp(a, b, count * sizeof(VkBufferViewCreateInfo)); if (cmpRes_VkBufferViewCreateInfo) { fprintf(stderr, "ERROR compare VkBufferViewCreateInfo: %d\n", cmpRes_VkBufferViewCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkBufferViewCreateInfo(uint32_t count, const VkBufferViewCreateInfo* a, const VkBufferViewCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].buffer))[i1] != 0x0) { fprintf(stderr, "ERROR VkBuffer:buffer handle not remapped to 0 %p\n", (&(b[i0].buffer))[i1]); }
        }
    }
    return res;
}


VkImageViewCreateInfo* genTestData_VkImageViewCreateInfo() {
    uint32_t count = 4;
    VkImageViewCreateInfo* res = nullptr;

    VkImageViewCreateInfo* tmp_VkImageViewCreateInfo = new VkImageViewCreateInfo[count];
    memset(tmp_VkImageViewCreateInfo, 0xabcddcba, count * sizeof(VkImageViewCreateInfo));
    res = tmp_VkImageViewCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkImageViewCreateInfo(uint32_t count, const VkImageViewCreateInfo* a, const VkImageViewCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkImageViewCreateInfo = memcmp(a, b, count * sizeof(VkImageViewCreateInfo)); if (cmpRes_VkImageViewCreateInfo) { fprintf(stderr, "ERROR compare VkImageViewCreateInfo: %d\n", cmpRes_VkImageViewCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkImageViewCreateInfo(uint32_t count, const VkImageViewCreateInfo* a, const VkImageViewCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].image))[i1] != 0x0) { fprintf(stderr, "ERROR VkImage:image handle not remapped to 0 %p\n", (&(b[i0].image))[i1]); }
        }
    }
    return res;
}


VkPipelineShaderStageCreateInfo* genTestData_VkPipelineShaderStageCreateInfo() {
    uint32_t count = 4;
    VkPipelineShaderStageCreateInfo* res = nullptr;

    VkPipelineShaderStageCreateInfo* tmp_VkPipelineShaderStageCreateInfo = new VkPipelineShaderStageCreateInfo[count];
    memset(tmp_VkPipelineShaderStageCreateInfo, 0xabcddcba, count * sizeof(VkPipelineShaderStageCreateInfo));
    res = tmp_VkPipelineShaderStageCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkPipelineShaderStageCreateInfo(uint32_t count, const VkPipelineShaderStageCreateInfo* a, const VkPipelineShaderStageCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkPipelineShaderStageCreateInfo = memcmp(a, b, count * sizeof(VkPipelineShaderStageCreateInfo)); if (cmpRes_VkPipelineShaderStageCreateInfo) { fprintf(stderr, "ERROR compare VkPipelineShaderStageCreateInfo: %d\n", cmpRes_VkPipelineShaderStageCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkPipelineShaderStageCreateInfo(uint32_t count, const VkPipelineShaderStageCreateInfo* a, const VkPipelineShaderStageCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].module))[i1] != 0x0) { fprintf(stderr, "ERROR VkShaderModule:module handle not remapped to 0 %p\n", (&(b[i0].module))[i1]); }
        }
    }
    return res;
}


VkGraphicsPipelineCreateInfo* genTestData_VkGraphicsPipelineCreateInfo() {
    uint32_t count = 4;
    VkGraphicsPipelineCreateInfo* res = nullptr;

    VkGraphicsPipelineCreateInfo* tmp_VkGraphicsPipelineCreateInfo = new VkGraphicsPipelineCreateInfo[count];
    memset(tmp_VkGraphicsPipelineCreateInfo, 0xabcddcba, count * sizeof(VkGraphicsPipelineCreateInfo));
    res = tmp_VkGraphicsPipelineCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkGraphicsPipelineCreateInfo + i0)->stageCount = 4;
        if ((res + i0)->stageCount) {
            VkPipelineShaderStageCreateInfo* tmp_VkPipelineShaderStageCreateInfo = new VkPipelineShaderStageCreateInfo[(res + i0)->stageCount];
            memset(tmp_VkPipelineShaderStageCreateInfo, 0xabcddcba, (res + i0)->stageCount * sizeof(VkPipelineShaderStageCreateInfo));
            memcpy((void*)&res[i0].pStages, (void**)(&tmp_VkPipelineShaderStageCreateInfo), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkGraphicsPipelineCreateInfo(uint32_t count, const VkGraphicsPipelineCreateInfo* a, const VkGraphicsPipelineCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkGraphicsPipelineCreateInfo = memcmp(a, b, count * sizeof(VkGraphicsPipelineCreateInfo)); if (cmpRes_VkGraphicsPipelineCreateInfo) { fprintf(stderr, "ERROR compare VkGraphicsPipelineCreateInfo: %d\n", cmpRes_VkGraphicsPipelineCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->stageCount != (b + i0)->stageCount) { fprintf(stderr, "ERROR compare counts VkGraphicsPipelineCreateInfo:VkPipelineShaderStageCreateInfo %u %u\n", (a + i0)->stageCount, (b + i0)->stageCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkGraphicsPipelineCreateInfo(uint32_t count, const VkGraphicsPipelineCreateInfo* a, const VkGraphicsPipelineCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
            for (uint32_t i2 = 0; i2 < (a + i0)->stageCount /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < 1; i3++) {
                if ((&(b[i0].pStages[i2].module))[i3] != 0x0) { fprintf(stderr, "ERROR VkShaderModule:module handle not remapped to 0 %p\n", (&(b[i0].pStages[i2].module))[i3]); }
                }
            }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].layout))[i1] != 0x0) { fprintf(stderr, "ERROR VkPipelineLayout:layout handle not remapped to 0 %p\n", (&(b[i0].layout))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].renderPass))[i1] != 0x0) { fprintf(stderr, "ERROR VkRenderPass:renderPass handle not remapped to 0 %p\n", (&(b[i0].renderPass))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].basePipelineHandle))[i1] != 0x0) { fprintf(stderr, "ERROR VkPipeline:basePipelineHandle handle not remapped to 0 %p\n", (&(b[i0].basePipelineHandle))[i1]); }
        }
    }
    return res;
}


VkComputePipelineCreateInfo* genTestData_VkComputePipelineCreateInfo() {
    uint32_t count = 4;
    VkComputePipelineCreateInfo* res = nullptr;

    VkComputePipelineCreateInfo* tmp_VkComputePipelineCreateInfo = new VkComputePipelineCreateInfo[count];
    memset(tmp_VkComputePipelineCreateInfo, 0xabcddcba, count * sizeof(VkComputePipelineCreateInfo));
    res = tmp_VkComputePipelineCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    // TODO: test data for non-ptr structs
    }
    return res;
}

bool deepCompare_VkComputePipelineCreateInfo(uint32_t count, const VkComputePipelineCreateInfo* a, const VkComputePipelineCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkComputePipelineCreateInfo = memcmp(a, b, count * sizeof(VkComputePipelineCreateInfo)); if (cmpRes_VkComputePipelineCreateInfo) { fprintf(stderr, "ERROR compare VkComputePipelineCreateInfo: %d\n", cmpRes_VkComputePipelineCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    int cmpRes_VkPipelineShaderStageCreateInfo = memcmp(&a[i0].stage, &b[i0].stage, 1 /* substruct */ * sizeof(VkPipelineShaderStageCreateInfo)); if (cmpRes_VkPipelineShaderStageCreateInfo) { fprintf(stderr, "ERROR compare VkPipelineShaderStageCreateInfo: %d\n", cmpRes_VkPipelineShaderStageCreateInfo); }
            for (uint32_t i2 = 0; i2 < 1 /* substruct */; i2++) {
            }
    }
    return res;
}

bool deepCompare_remap0_VkComputePipelineCreateInfo(uint32_t count, const VkComputePipelineCreateInfo* a, const VkComputePipelineCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
            for (uint32_t i2 = 0; i2 < 1 /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < 1; i3++) {
                if ((&((&(b[i0].stage))[i2].module))[i3] != 0x0) { fprintf(stderr, "ERROR VkShaderModule:module handle not remapped to 0 %p\n", (&((&(b[i0].stage))[i2].module))[i3]); }
                }
            }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].layout))[i1] != 0x0) { fprintf(stderr, "ERROR VkPipelineLayout:layout handle not remapped to 0 %p\n", (&(b[i0].layout))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].basePipelineHandle))[i1] != 0x0) { fprintf(stderr, "ERROR VkPipeline:basePipelineHandle handle not remapped to 0 %p\n", (&(b[i0].basePipelineHandle))[i1]); }
        }
    }
    return res;
}


VkPipelineLayoutCreateInfo* genTestData_VkPipelineLayoutCreateInfo() {
    uint32_t count = 4;
    VkPipelineLayoutCreateInfo* res = nullptr;

    VkPipelineLayoutCreateInfo* tmp_VkPipelineLayoutCreateInfo = new VkPipelineLayoutCreateInfo[count];
    memset(tmp_VkPipelineLayoutCreateInfo, 0xabcddcba, count * sizeof(VkPipelineLayoutCreateInfo));
    res = tmp_VkPipelineLayoutCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkPipelineLayoutCreateInfo + i0)->setLayoutCount = 4;
        if ((res + i0)->setLayoutCount) {
            VkDescriptorSetLayout* tmp_VkDescriptorSetLayout = new VkDescriptorSetLayout[(res + i0)->setLayoutCount];
            memset(tmp_VkDescriptorSetLayout, 0xabcddcba, (res + i0)->setLayoutCount * sizeof(VkDescriptorSetLayout));
            memcpy((void*)&res[i0].pSetLayouts, (void**)(&tmp_VkDescriptorSetLayout), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkPipelineLayoutCreateInfo(uint32_t count, const VkPipelineLayoutCreateInfo* a, const VkPipelineLayoutCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkPipelineLayoutCreateInfo = memcmp(a, b, count * sizeof(VkPipelineLayoutCreateInfo)); if (cmpRes_VkPipelineLayoutCreateInfo) { fprintf(stderr, "ERROR compare VkPipelineLayoutCreateInfo: %d\n", cmpRes_VkPipelineLayoutCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->setLayoutCount != (b + i0)->setLayoutCount) { fprintf(stderr, "ERROR compare counts VkPipelineLayoutCreateInfo:VkDescriptorSetLayout %u %u\n", (a + i0)->setLayoutCount, (b + i0)->setLayoutCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkPipelineLayoutCreateInfo(uint32_t count, const VkPipelineLayoutCreateInfo* a, const VkPipelineLayoutCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < (a + i0)->setLayoutCount; i1++) {
        if (b[i0].pSetLayouts[i1] != 0x0) { fprintf(stderr, "ERROR VkDescriptorSetLayout:pSetLayouts handle not remapped to 0 %p\n", b[i0].pSetLayouts[i1]); }
        }
    }
    return res;
}


VkDescriptorSetLayoutBinding* genTestData_VkDescriptorSetLayoutBinding() {
    uint32_t count = 4;
    VkDescriptorSetLayoutBinding* res = nullptr;

    VkDescriptorSetLayoutBinding* tmp_VkDescriptorSetLayoutBinding = new VkDescriptorSetLayoutBinding[count];
    memset(tmp_VkDescriptorSetLayoutBinding, 0xabcddcba, count * sizeof(VkDescriptorSetLayoutBinding));
    res = tmp_VkDescriptorSetLayoutBinding;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkDescriptorSetLayoutBinding + i0)->descriptorCount = 4;
        if ((res + i0)->descriptorCount) {
            VkSampler* tmp_VkSampler = new VkSampler[(res + i0)->descriptorCount];
            memset(tmp_VkSampler, 0xabcddcba, (res + i0)->descriptorCount * sizeof(VkSampler));
            memcpy((void*)&res[i0].pImmutableSamplers, (void**)(&tmp_VkSampler), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkDescriptorSetLayoutBinding(uint32_t count, const VkDescriptorSetLayoutBinding* a, const VkDescriptorSetLayoutBinding* b) {
    bool res = true;
    
    int cmpRes_VkDescriptorSetLayoutBinding = memcmp(a, b, count * sizeof(VkDescriptorSetLayoutBinding)); if (cmpRes_VkDescriptorSetLayoutBinding) { fprintf(stderr, "ERROR compare VkDescriptorSetLayoutBinding: %d\n", cmpRes_VkDescriptorSetLayoutBinding); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->descriptorCount != (b + i0)->descriptorCount) { fprintf(stderr, "ERROR compare counts VkDescriptorSetLayoutBinding:VkSampler %u %u\n", (a + i0)->descriptorCount, (b + i0)->descriptorCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkDescriptorSetLayoutBinding(uint32_t count, const VkDescriptorSetLayoutBinding* a, const VkDescriptorSetLayoutBinding* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < (a + i0)->descriptorCount; i1++) {
        if (b[i0].pImmutableSamplers[i1] != 0x0) { fprintf(stderr, "ERROR VkSampler:pImmutableSamplers handle not remapped to 0 %p\n", b[i0].pImmutableSamplers[i1]); }
        }
    }
    return res;
}


VkDescriptorSetLayoutCreateInfo* genTestData_VkDescriptorSetLayoutCreateInfo() {
    uint32_t count = 4;
    VkDescriptorSetLayoutCreateInfo* res = nullptr;

    VkDescriptorSetLayoutCreateInfo* tmp_VkDescriptorSetLayoutCreateInfo = new VkDescriptorSetLayoutCreateInfo[count];
    memset(tmp_VkDescriptorSetLayoutCreateInfo, 0xabcddcba, count * sizeof(VkDescriptorSetLayoutCreateInfo));
    res = tmp_VkDescriptorSetLayoutCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkDescriptorSetLayoutCreateInfo + i0)->bindingCount = 4;
        if ((res + i0)->bindingCount) {
            VkDescriptorSetLayoutBinding* tmp_VkDescriptorSetLayoutBinding = new VkDescriptorSetLayoutBinding[(res + i0)->bindingCount /* substruct */];
            memset(tmp_VkDescriptorSetLayoutBinding, 0xabcddcba, (res + i0)->bindingCount /* substruct */ * sizeof(VkDescriptorSetLayoutBinding));
            res[i0].pBindings = tmp_VkDescriptorSetLayoutBinding;
            for (uint32_t i2 = 0; i2 < (res + i0)->bindingCount /* substruct */; i2++) {
                (tmp_VkDescriptorSetLayoutBinding + i2)->descriptorCount = 4;
                if ((res[i0].pBindings + i2)->descriptorCount) {
                    VkSampler* tmp_VkSampler = new VkSampler[(res[i0].pBindings + i2)->descriptorCount];
                    memset(tmp_VkSampler, 0xabcddcba, (res[i0].pBindings + i2)->descriptorCount * sizeof(VkSampler));
                    memcpy((void*)&res[i0].pBindings[i2].pImmutableSamplers, (void**)(&tmp_VkSampler), sizeof(void*));
                }
            }
        }
    }
    return res;
}

bool deepCompare_VkDescriptorSetLayoutCreateInfo(uint32_t count, const VkDescriptorSetLayoutCreateInfo* a, const VkDescriptorSetLayoutCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkDescriptorSetLayoutCreateInfo = memcmp(a, b, count * sizeof(VkDescriptorSetLayoutCreateInfo)); if (cmpRes_VkDescriptorSetLayoutCreateInfo) { fprintf(stderr, "ERROR compare VkDescriptorSetLayoutCreateInfo: %d\n", cmpRes_VkDescriptorSetLayoutCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->bindingCount != (b + i0)->bindingCount) { fprintf(stderr, "ERROR compare counts VkDescriptorSetLayoutCreateInfo:VkDescriptorSetLayoutBinding %u %u\n", (a + i0)->bindingCount, (b + i0)->bindingCount); res = false; } 
    int cmpRes_VkDescriptorSetLayoutBinding = memcmp(a[i0].pBindings, b[i0].pBindings, (a + i0)->bindingCount /* substruct */ * sizeof(VkDescriptorSetLayoutBinding)); if (cmpRes_VkDescriptorSetLayoutBinding) { fprintf(stderr, "ERROR compare VkDescriptorSetLayoutBinding: %d\n", cmpRes_VkDescriptorSetLayoutBinding); }
            for (uint32_t i2 = 0; i2 < (a + i0)->bindingCount /* substruct */; i2++) {
                if ((a[i0].pBindings + i2)->descriptorCount != (b[i0].pBindings + i2)->descriptorCount) { fprintf(stderr, "ERROR compare counts VkDescriptorSetLayoutBinding:VkSampler %u %u\n", (a[i0].pBindings + i2)->descriptorCount, (b[i0].pBindings + i2)->descriptorCount); res = false; } 
            }
    }
    return res;
}

bool deepCompare_remap0_VkDescriptorSetLayoutCreateInfo(uint32_t count, const VkDescriptorSetLayoutCreateInfo* a, const VkDescriptorSetLayoutCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
            for (uint32_t i2 = 0; i2 < (a + i0)->bindingCount /* substruct */; i2++) {
                for (uint32_t i3 = 0; i3 < (a[i0].pBindings + i2)->descriptorCount; i3++) {
                if (b[i0].pBindings[i2].pImmutableSamplers[i3] != 0x0) { fprintf(stderr, "ERROR VkSampler:pImmutableSamplers handle not remapped to 0 %p\n", b[i0].pBindings[i2].pImmutableSamplers[i3]); }
                }
            }
    }
    return res;
}


VkDescriptorSetAllocateInfo* genTestData_VkDescriptorSetAllocateInfo() {
    uint32_t count = 4;
    VkDescriptorSetAllocateInfo* res = nullptr;

    VkDescriptorSetAllocateInfo* tmp_VkDescriptorSetAllocateInfo = new VkDescriptorSetAllocateInfo[count];
    memset(tmp_VkDescriptorSetAllocateInfo, 0xabcddcba, count * sizeof(VkDescriptorSetAllocateInfo));
    res = tmp_VkDescriptorSetAllocateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkDescriptorSetAllocateInfo + i0)->descriptorSetCount = 4;
        if ((res + i0)->descriptorSetCount) {
            VkDescriptorSetLayout* tmp_VkDescriptorSetLayout = new VkDescriptorSetLayout[(res + i0)->descriptorSetCount];
            memset(tmp_VkDescriptorSetLayout, 0xabcddcba, (res + i0)->descriptorSetCount * sizeof(VkDescriptorSetLayout));
            memcpy((void*)&res[i0].pSetLayouts, (void**)(&tmp_VkDescriptorSetLayout), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkDescriptorSetAllocateInfo(uint32_t count, const VkDescriptorSetAllocateInfo* a, const VkDescriptorSetAllocateInfo* b) {
    bool res = true;
    
    int cmpRes_VkDescriptorSetAllocateInfo = memcmp(a, b, count * sizeof(VkDescriptorSetAllocateInfo)); if (cmpRes_VkDescriptorSetAllocateInfo) { fprintf(stderr, "ERROR compare VkDescriptorSetAllocateInfo: %d\n", cmpRes_VkDescriptorSetAllocateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->descriptorSetCount != (b + i0)->descriptorSetCount) { fprintf(stderr, "ERROR compare counts VkDescriptorSetAllocateInfo:VkDescriptorSetLayout %u %u\n", (a + i0)->descriptorSetCount, (b + i0)->descriptorSetCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkDescriptorSetAllocateInfo(uint32_t count, const VkDescriptorSetAllocateInfo* a, const VkDescriptorSetAllocateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].descriptorPool))[i1] != 0x0) { fprintf(stderr, "ERROR VkDescriptorPool:descriptorPool handle not remapped to 0 %p\n", (&(b[i0].descriptorPool))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < (a + i0)->descriptorSetCount; i1++) {
        if (b[i0].pSetLayouts[i1] != 0x0) { fprintf(stderr, "ERROR VkDescriptorSetLayout:pSetLayouts handle not remapped to 0 %p\n", b[i0].pSetLayouts[i1]); }
        }
    }
    return res;
}


VkDescriptorImageInfo* genTestData_VkDescriptorImageInfo() {
    uint32_t count = 4;
    VkDescriptorImageInfo* res = nullptr;

    VkDescriptorImageInfo* tmp_VkDescriptorImageInfo = new VkDescriptorImageInfo[count];
    memset(tmp_VkDescriptorImageInfo, 0xabcddcba, count * sizeof(VkDescriptorImageInfo));
    res = tmp_VkDescriptorImageInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkDescriptorImageInfo(uint32_t count, const VkDescriptorImageInfo* a, const VkDescriptorImageInfo* b) {
    bool res = true;
    
    int cmpRes_VkDescriptorImageInfo = memcmp(a, b, count * sizeof(VkDescriptorImageInfo)); if (cmpRes_VkDescriptorImageInfo) { fprintf(stderr, "ERROR compare VkDescriptorImageInfo: %d\n", cmpRes_VkDescriptorImageInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkDescriptorImageInfo(uint32_t count, const VkDescriptorImageInfo* a, const VkDescriptorImageInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].sampler))[i1] != 0x0) { fprintf(stderr, "ERROR VkSampler:sampler handle not remapped to 0 %p\n", (&(b[i0].sampler))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].imageView))[i1] != 0x0) { fprintf(stderr, "ERROR VkImageView:imageView handle not remapped to 0 %p\n", (&(b[i0].imageView))[i1]); }
        }
    }
    return res;
}


VkDescriptorBufferInfo* genTestData_VkDescriptorBufferInfo() {
    uint32_t count = 4;
    VkDescriptorBufferInfo* res = nullptr;

    VkDescriptorBufferInfo* tmp_VkDescriptorBufferInfo = new VkDescriptorBufferInfo[count];
    memset(tmp_VkDescriptorBufferInfo, 0xabcddcba, count * sizeof(VkDescriptorBufferInfo));
    res = tmp_VkDescriptorBufferInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkDescriptorBufferInfo(uint32_t count, const VkDescriptorBufferInfo* a, const VkDescriptorBufferInfo* b) {
    bool res = true;
    
    int cmpRes_VkDescriptorBufferInfo = memcmp(a, b, count * sizeof(VkDescriptorBufferInfo)); if (cmpRes_VkDescriptorBufferInfo) { fprintf(stderr, "ERROR compare VkDescriptorBufferInfo: %d\n", cmpRes_VkDescriptorBufferInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkDescriptorBufferInfo(uint32_t count, const VkDescriptorBufferInfo* a, const VkDescriptorBufferInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].buffer))[i1] != 0x0) { fprintf(stderr, "ERROR VkBuffer:buffer handle not remapped to 0 %p\n", (&(b[i0].buffer))[i1]); }
        }
    }
    return res;
}


VkCopyDescriptorSet* genTestData_VkCopyDescriptorSet() {
    uint32_t count = 4;
    VkCopyDescriptorSet* res = nullptr;

    VkCopyDescriptorSet* tmp_VkCopyDescriptorSet = new VkCopyDescriptorSet[count];
    memset(tmp_VkCopyDescriptorSet, 0xabcddcba, count * sizeof(VkCopyDescriptorSet));
    res = tmp_VkCopyDescriptorSet;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkCopyDescriptorSet(uint32_t count, const VkCopyDescriptorSet* a, const VkCopyDescriptorSet* b) {
    bool res = true;
    
    int cmpRes_VkCopyDescriptorSet = memcmp(a, b, count * sizeof(VkCopyDescriptorSet)); if (cmpRes_VkCopyDescriptorSet) { fprintf(stderr, "ERROR compare VkCopyDescriptorSet: %d\n", cmpRes_VkCopyDescriptorSet); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkCopyDescriptorSet(uint32_t count, const VkCopyDescriptorSet* a, const VkCopyDescriptorSet* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].srcSet))[i1] != 0x0) { fprintf(stderr, "ERROR VkDescriptorSet:srcSet handle not remapped to 0 %p\n", (&(b[i0].srcSet))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].dstSet))[i1] != 0x0) { fprintf(stderr, "ERROR VkDescriptorSet:dstSet handle not remapped to 0 %p\n", (&(b[i0].dstSet))[i1]); }
        }
    }
    return res;
}


VkFramebufferCreateInfo* genTestData_VkFramebufferCreateInfo() {
    uint32_t count = 4;
    VkFramebufferCreateInfo* res = nullptr;

    VkFramebufferCreateInfo* tmp_VkFramebufferCreateInfo = new VkFramebufferCreateInfo[count];
    memset(tmp_VkFramebufferCreateInfo, 0xabcddcba, count * sizeof(VkFramebufferCreateInfo));
    res = tmp_VkFramebufferCreateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
        (tmp_VkFramebufferCreateInfo + i0)->attachmentCount = 4;
        if ((res + i0)->attachmentCount) {
            VkImageView* tmp_VkImageView = new VkImageView[(res + i0)->attachmentCount];
            memset(tmp_VkImageView, 0xabcddcba, (res + i0)->attachmentCount * sizeof(VkImageView));
            memcpy((void*)&res[i0].pAttachments, (void**)(&tmp_VkImageView), sizeof(void*));
        }
    }
    return res;
}

bool deepCompare_VkFramebufferCreateInfo(uint32_t count, const VkFramebufferCreateInfo* a, const VkFramebufferCreateInfo* b) {
    bool res = true;
    
    int cmpRes_VkFramebufferCreateInfo = memcmp(a, b, count * sizeof(VkFramebufferCreateInfo)); if (cmpRes_VkFramebufferCreateInfo) { fprintf(stderr, "ERROR compare VkFramebufferCreateInfo: %d\n", cmpRes_VkFramebufferCreateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
        if ((a + i0)->attachmentCount != (b + i0)->attachmentCount) { fprintf(stderr, "ERROR compare counts VkFramebufferCreateInfo:VkImageView %u %u\n", (a + i0)->attachmentCount, (b + i0)->attachmentCount); res = false; } 
    }
    return res;
}

bool deepCompare_remap0_VkFramebufferCreateInfo(uint32_t count, const VkFramebufferCreateInfo* a, const VkFramebufferCreateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].renderPass))[i1] != 0x0) { fprintf(stderr, "ERROR VkRenderPass:renderPass handle not remapped to 0 %p\n", (&(b[i0].renderPass))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < (a + i0)->attachmentCount; i1++) {
        if (b[i0].pAttachments[i1] != 0x0) { fprintf(stderr, "ERROR VkImageView:pAttachments handle not remapped to 0 %p\n", b[i0].pAttachments[i1]); }
        }
    }
    return res;
}


VkCommandBufferInheritanceInfo* genTestData_VkCommandBufferInheritanceInfo() {
    uint32_t count = 4;
    VkCommandBufferInheritanceInfo* res = nullptr;

    VkCommandBufferInheritanceInfo* tmp_VkCommandBufferInheritanceInfo = new VkCommandBufferInheritanceInfo[count];
    memset(tmp_VkCommandBufferInheritanceInfo, 0xabcddcba, count * sizeof(VkCommandBufferInheritanceInfo));
    res = tmp_VkCommandBufferInheritanceInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkCommandBufferInheritanceInfo(uint32_t count, const VkCommandBufferInheritanceInfo* a, const VkCommandBufferInheritanceInfo* b) {
    bool res = true;
    
    int cmpRes_VkCommandBufferInheritanceInfo = memcmp(a, b, count * sizeof(VkCommandBufferInheritanceInfo)); if (cmpRes_VkCommandBufferInheritanceInfo) { fprintf(stderr, "ERROR compare VkCommandBufferInheritanceInfo: %d\n", cmpRes_VkCommandBufferInheritanceInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkCommandBufferInheritanceInfo(uint32_t count, const VkCommandBufferInheritanceInfo* a, const VkCommandBufferInheritanceInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].renderPass))[i1] != 0x0) { fprintf(stderr, "ERROR VkRenderPass:renderPass handle not remapped to 0 %p\n", (&(b[i0].renderPass))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].framebuffer))[i1] != 0x0) { fprintf(stderr, "ERROR VkFramebuffer:framebuffer handle not remapped to 0 %p\n", (&(b[i0].framebuffer))[i1]); }
        }
    }
    return res;
}


VkCommandBufferAllocateInfo* genTestData_VkCommandBufferAllocateInfo() {
    uint32_t count = 4;
    VkCommandBufferAllocateInfo* res = nullptr;

    VkCommandBufferAllocateInfo* tmp_VkCommandBufferAllocateInfo = new VkCommandBufferAllocateInfo[count];
    memset(tmp_VkCommandBufferAllocateInfo, 0xabcddcba, count * sizeof(VkCommandBufferAllocateInfo));
    res = tmp_VkCommandBufferAllocateInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkCommandBufferAllocateInfo(uint32_t count, const VkCommandBufferAllocateInfo* a, const VkCommandBufferAllocateInfo* b) {
    bool res = true;
    
    int cmpRes_VkCommandBufferAllocateInfo = memcmp(a, b, count * sizeof(VkCommandBufferAllocateInfo)); if (cmpRes_VkCommandBufferAllocateInfo) { fprintf(stderr, "ERROR compare VkCommandBufferAllocateInfo: %d\n", cmpRes_VkCommandBufferAllocateInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkCommandBufferAllocateInfo(uint32_t count, const VkCommandBufferAllocateInfo* a, const VkCommandBufferAllocateInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].commandPool))[i1] != 0x0) { fprintf(stderr, "ERROR VkCommandPool:commandPool handle not remapped to 0 %p\n", (&(b[i0].commandPool))[i1]); }
        }
    }
    return res;
}


VkBufferMemoryBarrier* genTestData_VkBufferMemoryBarrier() {
    uint32_t count = 4;
    VkBufferMemoryBarrier* res = nullptr;

    VkBufferMemoryBarrier* tmp_VkBufferMemoryBarrier = new VkBufferMemoryBarrier[count];
    memset(tmp_VkBufferMemoryBarrier, 0xabcddcba, count * sizeof(VkBufferMemoryBarrier));
    res = tmp_VkBufferMemoryBarrier;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkBufferMemoryBarrier(uint32_t count, const VkBufferMemoryBarrier* a, const VkBufferMemoryBarrier* b) {
    bool res = true;
    
    int cmpRes_VkBufferMemoryBarrier = memcmp(a, b, count * sizeof(VkBufferMemoryBarrier)); if (cmpRes_VkBufferMemoryBarrier) { fprintf(stderr, "ERROR compare VkBufferMemoryBarrier: %d\n", cmpRes_VkBufferMemoryBarrier); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkBufferMemoryBarrier(uint32_t count, const VkBufferMemoryBarrier* a, const VkBufferMemoryBarrier* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].buffer))[i1] != 0x0) { fprintf(stderr, "ERROR VkBuffer:buffer handle not remapped to 0 %p\n", (&(b[i0].buffer))[i1]); }
        }
    }
    return res;
}


VkImageMemoryBarrier* genTestData_VkImageMemoryBarrier() {
    uint32_t count = 4;
    VkImageMemoryBarrier* res = nullptr;

    VkImageMemoryBarrier* tmp_VkImageMemoryBarrier = new VkImageMemoryBarrier[count];
    memset(tmp_VkImageMemoryBarrier, 0xabcddcba, count * sizeof(VkImageMemoryBarrier));
    res = tmp_VkImageMemoryBarrier;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkImageMemoryBarrier(uint32_t count, const VkImageMemoryBarrier* a, const VkImageMemoryBarrier* b) {
    bool res = true;
    
    int cmpRes_VkImageMemoryBarrier = memcmp(a, b, count * sizeof(VkImageMemoryBarrier)); if (cmpRes_VkImageMemoryBarrier) { fprintf(stderr, "ERROR compare VkImageMemoryBarrier: %d\n", cmpRes_VkImageMemoryBarrier); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkImageMemoryBarrier(uint32_t count, const VkImageMemoryBarrier* a, const VkImageMemoryBarrier* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].image))[i1] != 0x0) { fprintf(stderr, "ERROR VkImage:image handle not remapped to 0 %p\n", (&(b[i0].image))[i1]); }
        }
    }
    return res;
}


VkRenderPassBeginInfo* genTestData_VkRenderPassBeginInfo() {
    uint32_t count = 4;
    VkRenderPassBeginInfo* res = nullptr;

    VkRenderPassBeginInfo* tmp_VkRenderPassBeginInfo = new VkRenderPassBeginInfo[count];
    memset(tmp_VkRenderPassBeginInfo, 0xabcddcba, count * sizeof(VkRenderPassBeginInfo));
    res = tmp_VkRenderPassBeginInfo;
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_VkRenderPassBeginInfo(uint32_t count, const VkRenderPassBeginInfo* a, const VkRenderPassBeginInfo* b) {
    bool res = true;
    
    int cmpRes_VkRenderPassBeginInfo = memcmp(a, b, count * sizeof(VkRenderPassBeginInfo)); if (cmpRes_VkRenderPassBeginInfo) { fprintf(stderr, "ERROR compare VkRenderPassBeginInfo: %d\n", cmpRes_VkRenderPassBeginInfo); }
    for (uint32_t i0 = 0; i0 < count; i0++) {
    }
    return res;
}

bool deepCompare_remap0_VkRenderPassBeginInfo(uint32_t count, const VkRenderPassBeginInfo* a, const VkRenderPassBeginInfo* b) {
    bool res = true;
    
    for (uint32_t i0 = 0; i0 < count; i0++) {
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].renderPass))[i1] != 0x0) { fprintf(stderr, "ERROR VkRenderPass:renderPass handle not remapped to 0 %p\n", (&(b[i0].renderPass))[i1]); }
        }
        for (uint32_t i1 = 0; i1 < 1; i1++) {
        if ((&(b[i0].framebuffer))[i1] != 0x0) { fprintf(stderr, "ERROR VkFramebuffer:framebuffer handle not remapped to 0 %p\n", (&(b[i0].framebuffer))[i1]); }
        }
    }
    return res;
}




#define DEFINE_TEST_REMAP0(type) static type test_remap0_##type(type in) {     return (type)(uintptr_t)(0x0); } 
LIST_REASSIGNABLE_TYPES(DEFINE_TEST_REMAP0)

static vk_reassign_funcs test_remap0_reassigns;

#define SETUP_REMAP0(type)     test_remap0_reassigns.reassign_##type = test_remap0_##type;

int main(int argc, char** argv) {

    LIST_REASSIGNABLE_TYPES(SETUP_REMAP0)
    registerReassignmentFuncs(&test_remap0_reassigns);

    fprintf(stderr, "start of tests\n");
    
    {
        VkMappedMemoryRange* test_VkMappedMemoryRange = genTestData_VkMappedMemoryRange();
        fprintf(stderr, "bytes for VkMappedMemoryRange %zu\n", allocSz_VkMappedMemoryRange(4, test_VkMappedMemoryRange));
        std::vector<unsigned char> testBuf_VkMappedMemoryRange(allocSz_VkMappedMemoryRange(4, test_VkMappedMemoryRange), 0);
        VkMappedMemoryRange* testOut_VkMappedMemoryRange = initFromBuffer_VkMappedMemoryRange(4, test_VkMappedMemoryRange, testBuf_VkMappedMemoryRange.data());
        deepCopyAssign_VkMappedMemoryRange(4, test_VkMappedMemoryRange, testOut_VkMappedMemoryRange);
        if (!deepCompare_VkMappedMemoryRange(4, test_VkMappedMemoryRange, testOut_VkMappedMemoryRange)) fprintf(stderr, "ERROR marshaling VkMappedMemoryRange\n");
        deepCopyReassign_VkMappedMemoryRange(4, test_VkMappedMemoryRange, testOut_VkMappedMemoryRange);
        if (!deepCompare_remap0_VkMappedMemoryRange(4, test_VkMappedMemoryRange, testOut_VkMappedMemoryRange)) fprintf(stderr, "ERROR remapping VkMappedMemoryRange\n");
    }
    {
        VkSubmitInfo* test_VkSubmitInfo = genTestData_VkSubmitInfo();
        fprintf(stderr, "bytes for VkSubmitInfo %zu\n", allocSz_VkSubmitInfo(4, test_VkSubmitInfo));
        std::vector<unsigned char> testBuf_VkSubmitInfo(allocSz_VkSubmitInfo(4, test_VkSubmitInfo), 0);
        VkSubmitInfo* testOut_VkSubmitInfo = initFromBuffer_VkSubmitInfo(4, test_VkSubmitInfo, testBuf_VkSubmitInfo.data());
        deepCopyAssign_VkSubmitInfo(4, test_VkSubmitInfo, testOut_VkSubmitInfo);
        if (!deepCompare_VkSubmitInfo(4, test_VkSubmitInfo, testOut_VkSubmitInfo)) fprintf(stderr, "ERROR marshaling VkSubmitInfo\n");
        deepCopyReassign_VkSubmitInfo(4, test_VkSubmitInfo, testOut_VkSubmitInfo);
        if (!deepCompare_remap0_VkSubmitInfo(4, test_VkSubmitInfo, testOut_VkSubmitInfo)) fprintf(stderr, "ERROR remapping VkSubmitInfo\n");
    }
    {
        VkSparseMemoryBind* test_VkSparseMemoryBind = genTestData_VkSparseMemoryBind();
        fprintf(stderr, "bytes for VkSparseMemoryBind %zu\n", allocSz_VkSparseMemoryBind(4, test_VkSparseMemoryBind));
        std::vector<unsigned char> testBuf_VkSparseMemoryBind(allocSz_VkSparseMemoryBind(4, test_VkSparseMemoryBind), 0);
        VkSparseMemoryBind* testOut_VkSparseMemoryBind = initFromBuffer_VkSparseMemoryBind(4, test_VkSparseMemoryBind, testBuf_VkSparseMemoryBind.data());
        deepCopyAssign_VkSparseMemoryBind(4, test_VkSparseMemoryBind, testOut_VkSparseMemoryBind);
        if (!deepCompare_VkSparseMemoryBind(4, test_VkSparseMemoryBind, testOut_VkSparseMemoryBind)) fprintf(stderr, "ERROR marshaling VkSparseMemoryBind\n");
        deepCopyReassign_VkSparseMemoryBind(4, test_VkSparseMemoryBind, testOut_VkSparseMemoryBind);
        if (!deepCompare_remap0_VkSparseMemoryBind(4, test_VkSparseMemoryBind, testOut_VkSparseMemoryBind)) fprintf(stderr, "ERROR remapping VkSparseMemoryBind\n");
    }
    {
        VkSparseBufferMemoryBindInfo* test_VkSparseBufferMemoryBindInfo = genTestData_VkSparseBufferMemoryBindInfo();
        fprintf(stderr, "bytes for VkSparseBufferMemoryBindInfo %zu\n", allocSz_VkSparseBufferMemoryBindInfo(4, test_VkSparseBufferMemoryBindInfo));
        std::vector<unsigned char> testBuf_VkSparseBufferMemoryBindInfo(allocSz_VkSparseBufferMemoryBindInfo(4, test_VkSparseBufferMemoryBindInfo), 0);
        VkSparseBufferMemoryBindInfo* testOut_VkSparseBufferMemoryBindInfo = initFromBuffer_VkSparseBufferMemoryBindInfo(4, test_VkSparseBufferMemoryBindInfo, testBuf_VkSparseBufferMemoryBindInfo.data());
        deepCopyAssign_VkSparseBufferMemoryBindInfo(4, test_VkSparseBufferMemoryBindInfo, testOut_VkSparseBufferMemoryBindInfo);
        if (!deepCompare_VkSparseBufferMemoryBindInfo(4, test_VkSparseBufferMemoryBindInfo, testOut_VkSparseBufferMemoryBindInfo)) fprintf(stderr, "ERROR marshaling VkSparseBufferMemoryBindInfo\n");
        deepCopyReassign_VkSparseBufferMemoryBindInfo(4, test_VkSparseBufferMemoryBindInfo, testOut_VkSparseBufferMemoryBindInfo);
        if (!deepCompare_remap0_VkSparseBufferMemoryBindInfo(4, test_VkSparseBufferMemoryBindInfo, testOut_VkSparseBufferMemoryBindInfo)) fprintf(stderr, "ERROR remapping VkSparseBufferMemoryBindInfo\n");
    }
    {
        VkSparseImageOpaqueMemoryBindInfo* test_VkSparseImageOpaqueMemoryBindInfo = genTestData_VkSparseImageOpaqueMemoryBindInfo();
        fprintf(stderr, "bytes for VkSparseImageOpaqueMemoryBindInfo %zu\n", allocSz_VkSparseImageOpaqueMemoryBindInfo(4, test_VkSparseImageOpaqueMemoryBindInfo));
        std::vector<unsigned char> testBuf_VkSparseImageOpaqueMemoryBindInfo(allocSz_VkSparseImageOpaqueMemoryBindInfo(4, test_VkSparseImageOpaqueMemoryBindInfo), 0);
        VkSparseImageOpaqueMemoryBindInfo* testOut_VkSparseImageOpaqueMemoryBindInfo = initFromBuffer_VkSparseImageOpaqueMemoryBindInfo(4, test_VkSparseImageOpaqueMemoryBindInfo, testBuf_VkSparseImageOpaqueMemoryBindInfo.data());
        deepCopyAssign_VkSparseImageOpaqueMemoryBindInfo(4, test_VkSparseImageOpaqueMemoryBindInfo, testOut_VkSparseImageOpaqueMemoryBindInfo);
        if (!deepCompare_VkSparseImageOpaqueMemoryBindInfo(4, test_VkSparseImageOpaqueMemoryBindInfo, testOut_VkSparseImageOpaqueMemoryBindInfo)) fprintf(stderr, "ERROR marshaling VkSparseImageOpaqueMemoryBindInfo\n");
        deepCopyReassign_VkSparseImageOpaqueMemoryBindInfo(4, test_VkSparseImageOpaqueMemoryBindInfo, testOut_VkSparseImageOpaqueMemoryBindInfo);
        if (!deepCompare_remap0_VkSparseImageOpaqueMemoryBindInfo(4, test_VkSparseImageOpaqueMemoryBindInfo, testOut_VkSparseImageOpaqueMemoryBindInfo)) fprintf(stderr, "ERROR remapping VkSparseImageOpaqueMemoryBindInfo\n");
    }
    {
        VkSparseImageMemoryBindInfo* test_VkSparseImageMemoryBindInfo = genTestData_VkSparseImageMemoryBindInfo();
        fprintf(stderr, "bytes for VkSparseImageMemoryBindInfo %zu\n", allocSz_VkSparseImageMemoryBindInfo(4, test_VkSparseImageMemoryBindInfo));
        std::vector<unsigned char> testBuf_VkSparseImageMemoryBindInfo(allocSz_VkSparseImageMemoryBindInfo(4, test_VkSparseImageMemoryBindInfo), 0);
        VkSparseImageMemoryBindInfo* testOut_VkSparseImageMemoryBindInfo = initFromBuffer_VkSparseImageMemoryBindInfo(4, test_VkSparseImageMemoryBindInfo, testBuf_VkSparseImageMemoryBindInfo.data());
        deepCopyAssign_VkSparseImageMemoryBindInfo(4, test_VkSparseImageMemoryBindInfo, testOut_VkSparseImageMemoryBindInfo);
        if (!deepCompare_VkSparseImageMemoryBindInfo(4, test_VkSparseImageMemoryBindInfo, testOut_VkSparseImageMemoryBindInfo)) fprintf(stderr, "ERROR marshaling VkSparseImageMemoryBindInfo\n");
        deepCopyReassign_VkSparseImageMemoryBindInfo(4, test_VkSparseImageMemoryBindInfo, testOut_VkSparseImageMemoryBindInfo);
        if (!deepCompare_remap0_VkSparseImageMemoryBindInfo(4, test_VkSparseImageMemoryBindInfo, testOut_VkSparseImageMemoryBindInfo)) fprintf(stderr, "ERROR remapping VkSparseImageMemoryBindInfo\n");
    }
    {
        VkBindSparseInfo* test_VkBindSparseInfo = genTestData_VkBindSparseInfo();
        fprintf(stderr, "bytes for VkBindSparseInfo %zu\n", allocSz_VkBindSparseInfo(4, test_VkBindSparseInfo));
        std::vector<unsigned char> testBuf_VkBindSparseInfo(allocSz_VkBindSparseInfo(4, test_VkBindSparseInfo), 0);
        VkBindSparseInfo* testOut_VkBindSparseInfo = initFromBuffer_VkBindSparseInfo(4, test_VkBindSparseInfo, testBuf_VkBindSparseInfo.data());
        deepCopyAssign_VkBindSparseInfo(4, test_VkBindSparseInfo, testOut_VkBindSparseInfo);
        if (!deepCompare_VkBindSparseInfo(4, test_VkBindSparseInfo, testOut_VkBindSparseInfo)) fprintf(stderr, "ERROR marshaling VkBindSparseInfo\n");
        deepCopyReassign_VkBindSparseInfo(4, test_VkBindSparseInfo, testOut_VkBindSparseInfo);
        if (!deepCompare_remap0_VkBindSparseInfo(4, test_VkBindSparseInfo, testOut_VkBindSparseInfo)) fprintf(stderr, "ERROR remapping VkBindSparseInfo\n");
    }
    {
        VkBufferViewCreateInfo* test_VkBufferViewCreateInfo = genTestData_VkBufferViewCreateInfo();
        fprintf(stderr, "bytes for VkBufferViewCreateInfo %zu\n", allocSz_VkBufferViewCreateInfo(4, test_VkBufferViewCreateInfo));
        std::vector<unsigned char> testBuf_VkBufferViewCreateInfo(allocSz_VkBufferViewCreateInfo(4, test_VkBufferViewCreateInfo), 0);
        VkBufferViewCreateInfo* testOut_VkBufferViewCreateInfo = initFromBuffer_VkBufferViewCreateInfo(4, test_VkBufferViewCreateInfo, testBuf_VkBufferViewCreateInfo.data());
        deepCopyAssign_VkBufferViewCreateInfo(4, test_VkBufferViewCreateInfo, testOut_VkBufferViewCreateInfo);
        if (!deepCompare_VkBufferViewCreateInfo(4, test_VkBufferViewCreateInfo, testOut_VkBufferViewCreateInfo)) fprintf(stderr, "ERROR marshaling VkBufferViewCreateInfo\n");
        deepCopyReassign_VkBufferViewCreateInfo(4, test_VkBufferViewCreateInfo, testOut_VkBufferViewCreateInfo);
        if (!deepCompare_remap0_VkBufferViewCreateInfo(4, test_VkBufferViewCreateInfo, testOut_VkBufferViewCreateInfo)) fprintf(stderr, "ERROR remapping VkBufferViewCreateInfo\n");
    }
    {
        VkImageViewCreateInfo* test_VkImageViewCreateInfo = genTestData_VkImageViewCreateInfo();
        fprintf(stderr, "bytes for VkImageViewCreateInfo %zu\n", allocSz_VkImageViewCreateInfo(4, test_VkImageViewCreateInfo));
        std::vector<unsigned char> testBuf_VkImageViewCreateInfo(allocSz_VkImageViewCreateInfo(4, test_VkImageViewCreateInfo), 0);
        VkImageViewCreateInfo* testOut_VkImageViewCreateInfo = initFromBuffer_VkImageViewCreateInfo(4, test_VkImageViewCreateInfo, testBuf_VkImageViewCreateInfo.data());
        deepCopyAssign_VkImageViewCreateInfo(4, test_VkImageViewCreateInfo, testOut_VkImageViewCreateInfo);
        if (!deepCompare_VkImageViewCreateInfo(4, test_VkImageViewCreateInfo, testOut_VkImageViewCreateInfo)) fprintf(stderr, "ERROR marshaling VkImageViewCreateInfo\n");
        deepCopyReassign_VkImageViewCreateInfo(4, test_VkImageViewCreateInfo, testOut_VkImageViewCreateInfo);
        if (!deepCompare_remap0_VkImageViewCreateInfo(4, test_VkImageViewCreateInfo, testOut_VkImageViewCreateInfo)) fprintf(stderr, "ERROR remapping VkImageViewCreateInfo\n");
    }
    {
        VkPipelineShaderStageCreateInfo* test_VkPipelineShaderStageCreateInfo = genTestData_VkPipelineShaderStageCreateInfo();
        fprintf(stderr, "bytes for VkPipelineShaderStageCreateInfo %zu\n", allocSz_VkPipelineShaderStageCreateInfo(4, test_VkPipelineShaderStageCreateInfo));
        std::vector<unsigned char> testBuf_VkPipelineShaderStageCreateInfo(allocSz_VkPipelineShaderStageCreateInfo(4, test_VkPipelineShaderStageCreateInfo), 0);
        VkPipelineShaderStageCreateInfo* testOut_VkPipelineShaderStageCreateInfo = initFromBuffer_VkPipelineShaderStageCreateInfo(4, test_VkPipelineShaderStageCreateInfo, testBuf_VkPipelineShaderStageCreateInfo.data());
        deepCopyAssign_VkPipelineShaderStageCreateInfo(4, test_VkPipelineShaderStageCreateInfo, testOut_VkPipelineShaderStageCreateInfo);
        if (!deepCompare_VkPipelineShaderStageCreateInfo(4, test_VkPipelineShaderStageCreateInfo, testOut_VkPipelineShaderStageCreateInfo)) fprintf(stderr, "ERROR marshaling VkPipelineShaderStageCreateInfo\n");
        deepCopyReassign_VkPipelineShaderStageCreateInfo(4, test_VkPipelineShaderStageCreateInfo, testOut_VkPipelineShaderStageCreateInfo);
        if (!deepCompare_remap0_VkPipelineShaderStageCreateInfo(4, test_VkPipelineShaderStageCreateInfo, testOut_VkPipelineShaderStageCreateInfo)) fprintf(stderr, "ERROR remapping VkPipelineShaderStageCreateInfo\n");
    }
    {
        VkGraphicsPipelineCreateInfo* test_VkGraphicsPipelineCreateInfo = genTestData_VkGraphicsPipelineCreateInfo();
        fprintf(stderr, "bytes for VkGraphicsPipelineCreateInfo %zu\n", allocSz_VkGraphicsPipelineCreateInfo(4, test_VkGraphicsPipelineCreateInfo));
        std::vector<unsigned char> testBuf_VkGraphicsPipelineCreateInfo(allocSz_VkGraphicsPipelineCreateInfo(4, test_VkGraphicsPipelineCreateInfo), 0);
        VkGraphicsPipelineCreateInfo* testOut_VkGraphicsPipelineCreateInfo = initFromBuffer_VkGraphicsPipelineCreateInfo(4, test_VkGraphicsPipelineCreateInfo, testBuf_VkGraphicsPipelineCreateInfo.data());
        deepCopyAssign_VkGraphicsPipelineCreateInfo(4, test_VkGraphicsPipelineCreateInfo, testOut_VkGraphicsPipelineCreateInfo);
        if (!deepCompare_VkGraphicsPipelineCreateInfo(4, test_VkGraphicsPipelineCreateInfo, testOut_VkGraphicsPipelineCreateInfo)) fprintf(stderr, "ERROR marshaling VkGraphicsPipelineCreateInfo\n");
        deepCopyReassign_VkGraphicsPipelineCreateInfo(4, test_VkGraphicsPipelineCreateInfo, testOut_VkGraphicsPipelineCreateInfo);
        if (!deepCompare_remap0_VkGraphicsPipelineCreateInfo(4, test_VkGraphicsPipelineCreateInfo, testOut_VkGraphicsPipelineCreateInfo)) fprintf(stderr, "ERROR remapping VkGraphicsPipelineCreateInfo\n");
    }
    {
        VkComputePipelineCreateInfo* test_VkComputePipelineCreateInfo = genTestData_VkComputePipelineCreateInfo();
        fprintf(stderr, "bytes for VkComputePipelineCreateInfo %zu\n", allocSz_VkComputePipelineCreateInfo(4, test_VkComputePipelineCreateInfo));
        std::vector<unsigned char> testBuf_VkComputePipelineCreateInfo(allocSz_VkComputePipelineCreateInfo(4, test_VkComputePipelineCreateInfo), 0);
        VkComputePipelineCreateInfo* testOut_VkComputePipelineCreateInfo = initFromBuffer_VkComputePipelineCreateInfo(4, test_VkComputePipelineCreateInfo, testBuf_VkComputePipelineCreateInfo.data());
        deepCopyAssign_VkComputePipelineCreateInfo(4, test_VkComputePipelineCreateInfo, testOut_VkComputePipelineCreateInfo);
        if (!deepCompare_VkComputePipelineCreateInfo(4, test_VkComputePipelineCreateInfo, testOut_VkComputePipelineCreateInfo)) fprintf(stderr, "ERROR marshaling VkComputePipelineCreateInfo\n");
        deepCopyReassign_VkComputePipelineCreateInfo(4, test_VkComputePipelineCreateInfo, testOut_VkComputePipelineCreateInfo);
        if (!deepCompare_remap0_VkComputePipelineCreateInfo(4, test_VkComputePipelineCreateInfo, testOut_VkComputePipelineCreateInfo)) fprintf(stderr, "ERROR remapping VkComputePipelineCreateInfo\n");
    }
    {
        VkPipelineLayoutCreateInfo* test_VkPipelineLayoutCreateInfo = genTestData_VkPipelineLayoutCreateInfo();
        fprintf(stderr, "bytes for VkPipelineLayoutCreateInfo %zu\n", allocSz_VkPipelineLayoutCreateInfo(4, test_VkPipelineLayoutCreateInfo));
        std::vector<unsigned char> testBuf_VkPipelineLayoutCreateInfo(allocSz_VkPipelineLayoutCreateInfo(4, test_VkPipelineLayoutCreateInfo), 0);
        VkPipelineLayoutCreateInfo* testOut_VkPipelineLayoutCreateInfo = initFromBuffer_VkPipelineLayoutCreateInfo(4, test_VkPipelineLayoutCreateInfo, testBuf_VkPipelineLayoutCreateInfo.data());
        deepCopyAssign_VkPipelineLayoutCreateInfo(4, test_VkPipelineLayoutCreateInfo, testOut_VkPipelineLayoutCreateInfo);
        if (!deepCompare_VkPipelineLayoutCreateInfo(4, test_VkPipelineLayoutCreateInfo, testOut_VkPipelineLayoutCreateInfo)) fprintf(stderr, "ERROR marshaling VkPipelineLayoutCreateInfo\n");
        deepCopyReassign_VkPipelineLayoutCreateInfo(4, test_VkPipelineLayoutCreateInfo, testOut_VkPipelineLayoutCreateInfo);
        if (!deepCompare_remap0_VkPipelineLayoutCreateInfo(4, test_VkPipelineLayoutCreateInfo, testOut_VkPipelineLayoutCreateInfo)) fprintf(stderr, "ERROR remapping VkPipelineLayoutCreateInfo\n");
    }
    {
        VkDescriptorSetLayoutBinding* test_VkDescriptorSetLayoutBinding = genTestData_VkDescriptorSetLayoutBinding();
        fprintf(stderr, "bytes for VkDescriptorSetLayoutBinding %zu\n", allocSz_VkDescriptorSetLayoutBinding(4, test_VkDescriptorSetLayoutBinding));
        std::vector<unsigned char> testBuf_VkDescriptorSetLayoutBinding(allocSz_VkDescriptorSetLayoutBinding(4, test_VkDescriptorSetLayoutBinding), 0);
        VkDescriptorSetLayoutBinding* testOut_VkDescriptorSetLayoutBinding = initFromBuffer_VkDescriptorSetLayoutBinding(4, test_VkDescriptorSetLayoutBinding, testBuf_VkDescriptorSetLayoutBinding.data());
        deepCopyAssign_VkDescriptorSetLayoutBinding(4, test_VkDescriptorSetLayoutBinding, testOut_VkDescriptorSetLayoutBinding);
        if (!deepCompare_VkDescriptorSetLayoutBinding(4, test_VkDescriptorSetLayoutBinding, testOut_VkDescriptorSetLayoutBinding)) fprintf(stderr, "ERROR marshaling VkDescriptorSetLayoutBinding\n");
        deepCopyReassign_VkDescriptorSetLayoutBinding(4, test_VkDescriptorSetLayoutBinding, testOut_VkDescriptorSetLayoutBinding);
        if (!deepCompare_remap0_VkDescriptorSetLayoutBinding(4, test_VkDescriptorSetLayoutBinding, testOut_VkDescriptorSetLayoutBinding)) fprintf(stderr, "ERROR remapping VkDescriptorSetLayoutBinding\n");
    }
    {
        VkDescriptorSetLayoutCreateInfo* test_VkDescriptorSetLayoutCreateInfo = genTestData_VkDescriptorSetLayoutCreateInfo();
        fprintf(stderr, "bytes for VkDescriptorSetLayoutCreateInfo %zu\n", allocSz_VkDescriptorSetLayoutCreateInfo(4, test_VkDescriptorSetLayoutCreateInfo));
        std::vector<unsigned char> testBuf_VkDescriptorSetLayoutCreateInfo(allocSz_VkDescriptorSetLayoutCreateInfo(4, test_VkDescriptorSetLayoutCreateInfo), 0);
        VkDescriptorSetLayoutCreateInfo* testOut_VkDescriptorSetLayoutCreateInfo = initFromBuffer_VkDescriptorSetLayoutCreateInfo(4, test_VkDescriptorSetLayoutCreateInfo, testBuf_VkDescriptorSetLayoutCreateInfo.data());
        deepCopyAssign_VkDescriptorSetLayoutCreateInfo(4, test_VkDescriptorSetLayoutCreateInfo, testOut_VkDescriptorSetLayoutCreateInfo);
        if (!deepCompare_VkDescriptorSetLayoutCreateInfo(4, test_VkDescriptorSetLayoutCreateInfo, testOut_VkDescriptorSetLayoutCreateInfo)) fprintf(stderr, "ERROR marshaling VkDescriptorSetLayoutCreateInfo\n");
        deepCopyReassign_VkDescriptorSetLayoutCreateInfo(4, test_VkDescriptorSetLayoutCreateInfo, testOut_VkDescriptorSetLayoutCreateInfo);
        if (!deepCompare_remap0_VkDescriptorSetLayoutCreateInfo(4, test_VkDescriptorSetLayoutCreateInfo, testOut_VkDescriptorSetLayoutCreateInfo)) fprintf(stderr, "ERROR remapping VkDescriptorSetLayoutCreateInfo\n");
    }
    {
        VkDescriptorSetAllocateInfo* test_VkDescriptorSetAllocateInfo = genTestData_VkDescriptorSetAllocateInfo();
        fprintf(stderr, "bytes for VkDescriptorSetAllocateInfo %zu\n", allocSz_VkDescriptorSetAllocateInfo(4, test_VkDescriptorSetAllocateInfo));
        std::vector<unsigned char> testBuf_VkDescriptorSetAllocateInfo(allocSz_VkDescriptorSetAllocateInfo(4, test_VkDescriptorSetAllocateInfo), 0);
        VkDescriptorSetAllocateInfo* testOut_VkDescriptorSetAllocateInfo = initFromBuffer_VkDescriptorSetAllocateInfo(4, test_VkDescriptorSetAllocateInfo, testBuf_VkDescriptorSetAllocateInfo.data());
        deepCopyAssign_VkDescriptorSetAllocateInfo(4, test_VkDescriptorSetAllocateInfo, testOut_VkDescriptorSetAllocateInfo);
        if (!deepCompare_VkDescriptorSetAllocateInfo(4, test_VkDescriptorSetAllocateInfo, testOut_VkDescriptorSetAllocateInfo)) fprintf(stderr, "ERROR marshaling VkDescriptorSetAllocateInfo\n");
        deepCopyReassign_VkDescriptorSetAllocateInfo(4, test_VkDescriptorSetAllocateInfo, testOut_VkDescriptorSetAllocateInfo);
        if (!deepCompare_remap0_VkDescriptorSetAllocateInfo(4, test_VkDescriptorSetAllocateInfo, testOut_VkDescriptorSetAllocateInfo)) fprintf(stderr, "ERROR remapping VkDescriptorSetAllocateInfo\n");
    }
    {
        VkDescriptorImageInfo* test_VkDescriptorImageInfo = genTestData_VkDescriptorImageInfo();
        fprintf(stderr, "bytes for VkDescriptorImageInfo %zu\n", allocSz_VkDescriptorImageInfo(4, test_VkDescriptorImageInfo));
        std::vector<unsigned char> testBuf_VkDescriptorImageInfo(allocSz_VkDescriptorImageInfo(4, test_VkDescriptorImageInfo), 0);
        VkDescriptorImageInfo* testOut_VkDescriptorImageInfo = initFromBuffer_VkDescriptorImageInfo(4, test_VkDescriptorImageInfo, testBuf_VkDescriptorImageInfo.data());
        deepCopyAssign_VkDescriptorImageInfo(4, test_VkDescriptorImageInfo, testOut_VkDescriptorImageInfo);
        if (!deepCompare_VkDescriptorImageInfo(4, test_VkDescriptorImageInfo, testOut_VkDescriptorImageInfo)) fprintf(stderr, "ERROR marshaling VkDescriptorImageInfo\n");
        deepCopyReassign_VkDescriptorImageInfo(4, test_VkDescriptorImageInfo, testOut_VkDescriptorImageInfo);
        if (!deepCompare_remap0_VkDescriptorImageInfo(4, test_VkDescriptorImageInfo, testOut_VkDescriptorImageInfo)) fprintf(stderr, "ERROR remapping VkDescriptorImageInfo\n");
    }
    {
        VkDescriptorBufferInfo* test_VkDescriptorBufferInfo = genTestData_VkDescriptorBufferInfo();
        fprintf(stderr, "bytes for VkDescriptorBufferInfo %zu\n", allocSz_VkDescriptorBufferInfo(4, test_VkDescriptorBufferInfo));
        std::vector<unsigned char> testBuf_VkDescriptorBufferInfo(allocSz_VkDescriptorBufferInfo(4, test_VkDescriptorBufferInfo), 0);
        VkDescriptorBufferInfo* testOut_VkDescriptorBufferInfo = initFromBuffer_VkDescriptorBufferInfo(4, test_VkDescriptorBufferInfo, testBuf_VkDescriptorBufferInfo.data());
        deepCopyAssign_VkDescriptorBufferInfo(4, test_VkDescriptorBufferInfo, testOut_VkDescriptorBufferInfo);
        if (!deepCompare_VkDescriptorBufferInfo(4, test_VkDescriptorBufferInfo, testOut_VkDescriptorBufferInfo)) fprintf(stderr, "ERROR marshaling VkDescriptorBufferInfo\n");
        deepCopyReassign_VkDescriptorBufferInfo(4, test_VkDescriptorBufferInfo, testOut_VkDescriptorBufferInfo);
        if (!deepCompare_remap0_VkDescriptorBufferInfo(4, test_VkDescriptorBufferInfo, testOut_VkDescriptorBufferInfo)) fprintf(stderr, "ERROR remapping VkDescriptorBufferInfo\n");
    }
    {
        VkCopyDescriptorSet* test_VkCopyDescriptorSet = genTestData_VkCopyDescriptorSet();
        fprintf(stderr, "bytes for VkCopyDescriptorSet %zu\n", allocSz_VkCopyDescriptorSet(4, test_VkCopyDescriptorSet));
        std::vector<unsigned char> testBuf_VkCopyDescriptorSet(allocSz_VkCopyDescriptorSet(4, test_VkCopyDescriptorSet), 0);
        VkCopyDescriptorSet* testOut_VkCopyDescriptorSet = initFromBuffer_VkCopyDescriptorSet(4, test_VkCopyDescriptorSet, testBuf_VkCopyDescriptorSet.data());
        deepCopyAssign_VkCopyDescriptorSet(4, test_VkCopyDescriptorSet, testOut_VkCopyDescriptorSet);
        if (!deepCompare_VkCopyDescriptorSet(4, test_VkCopyDescriptorSet, testOut_VkCopyDescriptorSet)) fprintf(stderr, "ERROR marshaling VkCopyDescriptorSet\n");
        deepCopyReassign_VkCopyDescriptorSet(4, test_VkCopyDescriptorSet, testOut_VkCopyDescriptorSet);
        if (!deepCompare_remap0_VkCopyDescriptorSet(4, test_VkCopyDescriptorSet, testOut_VkCopyDescriptorSet)) fprintf(stderr, "ERROR remapping VkCopyDescriptorSet\n");
    }
    {
        VkFramebufferCreateInfo* test_VkFramebufferCreateInfo = genTestData_VkFramebufferCreateInfo();
        fprintf(stderr, "bytes for VkFramebufferCreateInfo %zu\n", allocSz_VkFramebufferCreateInfo(4, test_VkFramebufferCreateInfo));
        std::vector<unsigned char> testBuf_VkFramebufferCreateInfo(allocSz_VkFramebufferCreateInfo(4, test_VkFramebufferCreateInfo), 0);
        VkFramebufferCreateInfo* testOut_VkFramebufferCreateInfo = initFromBuffer_VkFramebufferCreateInfo(4, test_VkFramebufferCreateInfo, testBuf_VkFramebufferCreateInfo.data());
        deepCopyAssign_VkFramebufferCreateInfo(4, test_VkFramebufferCreateInfo, testOut_VkFramebufferCreateInfo);
        if (!deepCompare_VkFramebufferCreateInfo(4, test_VkFramebufferCreateInfo, testOut_VkFramebufferCreateInfo)) fprintf(stderr, "ERROR marshaling VkFramebufferCreateInfo\n");
        deepCopyReassign_VkFramebufferCreateInfo(4, test_VkFramebufferCreateInfo, testOut_VkFramebufferCreateInfo);
        if (!deepCompare_remap0_VkFramebufferCreateInfo(4, test_VkFramebufferCreateInfo, testOut_VkFramebufferCreateInfo)) fprintf(stderr, "ERROR remapping VkFramebufferCreateInfo\n");
    }
    {
        VkCommandBufferInheritanceInfo* test_VkCommandBufferInheritanceInfo = genTestData_VkCommandBufferInheritanceInfo();
        fprintf(stderr, "bytes for VkCommandBufferInheritanceInfo %zu\n", allocSz_VkCommandBufferInheritanceInfo(4, test_VkCommandBufferInheritanceInfo));
        std::vector<unsigned char> testBuf_VkCommandBufferInheritanceInfo(allocSz_VkCommandBufferInheritanceInfo(4, test_VkCommandBufferInheritanceInfo), 0);
        VkCommandBufferInheritanceInfo* testOut_VkCommandBufferInheritanceInfo = initFromBuffer_VkCommandBufferInheritanceInfo(4, test_VkCommandBufferInheritanceInfo, testBuf_VkCommandBufferInheritanceInfo.data());
        deepCopyAssign_VkCommandBufferInheritanceInfo(4, test_VkCommandBufferInheritanceInfo, testOut_VkCommandBufferInheritanceInfo);
        if (!deepCompare_VkCommandBufferInheritanceInfo(4, test_VkCommandBufferInheritanceInfo, testOut_VkCommandBufferInheritanceInfo)) fprintf(stderr, "ERROR marshaling VkCommandBufferInheritanceInfo\n");
        deepCopyReassign_VkCommandBufferInheritanceInfo(4, test_VkCommandBufferInheritanceInfo, testOut_VkCommandBufferInheritanceInfo);
        if (!deepCompare_remap0_VkCommandBufferInheritanceInfo(4, test_VkCommandBufferInheritanceInfo, testOut_VkCommandBufferInheritanceInfo)) fprintf(stderr, "ERROR remapping VkCommandBufferInheritanceInfo\n");
    }
    {
        VkCommandBufferAllocateInfo* test_VkCommandBufferAllocateInfo = genTestData_VkCommandBufferAllocateInfo();
        fprintf(stderr, "bytes for VkCommandBufferAllocateInfo %zu\n", allocSz_VkCommandBufferAllocateInfo(4, test_VkCommandBufferAllocateInfo));
        std::vector<unsigned char> testBuf_VkCommandBufferAllocateInfo(allocSz_VkCommandBufferAllocateInfo(4, test_VkCommandBufferAllocateInfo), 0);
        VkCommandBufferAllocateInfo* testOut_VkCommandBufferAllocateInfo = initFromBuffer_VkCommandBufferAllocateInfo(4, test_VkCommandBufferAllocateInfo, testBuf_VkCommandBufferAllocateInfo.data());
        deepCopyAssign_VkCommandBufferAllocateInfo(4, test_VkCommandBufferAllocateInfo, testOut_VkCommandBufferAllocateInfo);
        if (!deepCompare_VkCommandBufferAllocateInfo(4, test_VkCommandBufferAllocateInfo, testOut_VkCommandBufferAllocateInfo)) fprintf(stderr, "ERROR marshaling VkCommandBufferAllocateInfo\n");
        deepCopyReassign_VkCommandBufferAllocateInfo(4, test_VkCommandBufferAllocateInfo, testOut_VkCommandBufferAllocateInfo);
        if (!deepCompare_remap0_VkCommandBufferAllocateInfo(4, test_VkCommandBufferAllocateInfo, testOut_VkCommandBufferAllocateInfo)) fprintf(stderr, "ERROR remapping VkCommandBufferAllocateInfo\n");
    }
    {
        VkBufferMemoryBarrier* test_VkBufferMemoryBarrier = genTestData_VkBufferMemoryBarrier();
        fprintf(stderr, "bytes for VkBufferMemoryBarrier %zu\n", allocSz_VkBufferMemoryBarrier(4, test_VkBufferMemoryBarrier));
        std::vector<unsigned char> testBuf_VkBufferMemoryBarrier(allocSz_VkBufferMemoryBarrier(4, test_VkBufferMemoryBarrier), 0);
        VkBufferMemoryBarrier* testOut_VkBufferMemoryBarrier = initFromBuffer_VkBufferMemoryBarrier(4, test_VkBufferMemoryBarrier, testBuf_VkBufferMemoryBarrier.data());
        deepCopyAssign_VkBufferMemoryBarrier(4, test_VkBufferMemoryBarrier, testOut_VkBufferMemoryBarrier);
        if (!deepCompare_VkBufferMemoryBarrier(4, test_VkBufferMemoryBarrier, testOut_VkBufferMemoryBarrier)) fprintf(stderr, "ERROR marshaling VkBufferMemoryBarrier\n");
        deepCopyReassign_VkBufferMemoryBarrier(4, test_VkBufferMemoryBarrier, testOut_VkBufferMemoryBarrier);
        if (!deepCompare_remap0_VkBufferMemoryBarrier(4, test_VkBufferMemoryBarrier, testOut_VkBufferMemoryBarrier)) fprintf(stderr, "ERROR remapping VkBufferMemoryBarrier\n");
    }
    {
        VkImageMemoryBarrier* test_VkImageMemoryBarrier = genTestData_VkImageMemoryBarrier();
        fprintf(stderr, "bytes for VkImageMemoryBarrier %zu\n", allocSz_VkImageMemoryBarrier(4, test_VkImageMemoryBarrier));
        std::vector<unsigned char> testBuf_VkImageMemoryBarrier(allocSz_VkImageMemoryBarrier(4, test_VkImageMemoryBarrier), 0);
        VkImageMemoryBarrier* testOut_VkImageMemoryBarrier = initFromBuffer_VkImageMemoryBarrier(4, test_VkImageMemoryBarrier, testBuf_VkImageMemoryBarrier.data());
        deepCopyAssign_VkImageMemoryBarrier(4, test_VkImageMemoryBarrier, testOut_VkImageMemoryBarrier);
        if (!deepCompare_VkImageMemoryBarrier(4, test_VkImageMemoryBarrier, testOut_VkImageMemoryBarrier)) fprintf(stderr, "ERROR marshaling VkImageMemoryBarrier\n");
        deepCopyReassign_VkImageMemoryBarrier(4, test_VkImageMemoryBarrier, testOut_VkImageMemoryBarrier);
        if (!deepCompare_remap0_VkImageMemoryBarrier(4, test_VkImageMemoryBarrier, testOut_VkImageMemoryBarrier)) fprintf(stderr, "ERROR remapping VkImageMemoryBarrier\n");
    }
    {
        VkRenderPassBeginInfo* test_VkRenderPassBeginInfo = genTestData_VkRenderPassBeginInfo();
        fprintf(stderr, "bytes for VkRenderPassBeginInfo %zu\n", allocSz_VkRenderPassBeginInfo(4, test_VkRenderPassBeginInfo));
        std::vector<unsigned char> testBuf_VkRenderPassBeginInfo(allocSz_VkRenderPassBeginInfo(4, test_VkRenderPassBeginInfo), 0);
        VkRenderPassBeginInfo* testOut_VkRenderPassBeginInfo = initFromBuffer_VkRenderPassBeginInfo(4, test_VkRenderPassBeginInfo, testBuf_VkRenderPassBeginInfo.data());
        deepCopyAssign_VkRenderPassBeginInfo(4, test_VkRenderPassBeginInfo, testOut_VkRenderPassBeginInfo);
        if (!deepCompare_VkRenderPassBeginInfo(4, test_VkRenderPassBeginInfo, testOut_VkRenderPassBeginInfo)) fprintf(stderr, "ERROR marshaling VkRenderPassBeginInfo\n");
        deepCopyReassign_VkRenderPassBeginInfo(4, test_VkRenderPassBeginInfo, testOut_VkRenderPassBeginInfo);
        if (!deepCompare_remap0_VkRenderPassBeginInfo(4, test_VkRenderPassBeginInfo, testOut_VkRenderPassBeginInfo)) fprintf(stderr, "ERROR remapping VkRenderPassBeginInfo\n");
    }
    fprintf(stderr, "done with tests\n");
    return 0;
}
