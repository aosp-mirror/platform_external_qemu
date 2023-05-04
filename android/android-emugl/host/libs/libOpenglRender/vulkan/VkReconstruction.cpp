// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "VkReconstruction.h"

#include "aemu/base/containers/EntityManager.h"

#include "FrameBuffer.h"
#include "render-utils/IOStream.h"
#include "VkDecoder.h"

#include <unordered_map>

#define DEBUG_RECONSTRUCTION 0

#if DEBUG_RECONSTRUCTION

#define DEBUG_RECON(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);

#else

#define DEBUG_RECON(fmt,...)

#endif

static inline uint64_t ptrToU64(void* ptr) { return (uint64_t)(uintptr_t)ptr; }

VkReconstruction::VkReconstruction() = default;

std::vector<uint64_t> typeTagSortedHandles(const std::vector<uint64_t>& handles) {
    using EntityManagerTypeForHandles =
        android::base::EntityManager<32, 16, 16, int>;

    std::vector<uint64_t> res = handles;

    std::sort(res.begin(), res.end(), [](uint64_t lhs, uint64_t rhs) {
        return EntityManagerTypeForHandles::getHandleType(lhs) <
               EntityManagerTypeForHandles::getHandleType(rhs); });

    return res;
}

void VkReconstruction::save(android::base::Stream* stream) {
    DEBUG_RECON("start")

#if DEBUG_RECON
    dump();
#endif

    std::unordered_map<uint64_t, uint64_t> backDeps;

    mHandleReconstructions.forEachLiveComponent_const(
        [&backDeps](bool live, uint64_t componentHandle, uint64_t entityHandle, const HandleReconstruction& item) {
        for (auto handle : item.childHandles) {
            backDeps[handle] = entityHandle;
        }
    });

    std::vector<uint64_t> topoOrder;

    mHandleReconstructions.forEachLiveComponent_const(
        [&topoOrder, &backDeps](bool live, uint64_t componentHandle, uint64_t entityHandle, const HandleReconstruction& item) {
        // Start with populating the roots
        if (backDeps.find(entityHandle) == backDeps.end()) {
            DEBUG_RECON("found root: 0x%llx",
                        (unsigned long long)entityHandle);
            topoOrder.push_back(entityHandle);
        }
    });

    std::vector<uint64_t> next;

    std::unordered_map<uint64_t, uint64_t> uniqApiRefsToTopoOrder;
    std::unordered_map<uint64_t, std::vector<uint64_t>> uniqApiRefsByTopoOrder;
    std::unordered_map<uint64_t, std::vector<uint64_t>> uniqApiRefsByTopoAndDependencyOrder;

    size_t topoLevel = 0;

    topoOrder = typeTagSortedHandles(topoOrder);

    while (!topoOrder.empty()) {

        next.clear();

        for (auto handle : topoOrder) {
            auto item = mHandleReconstructions.get(handle);

            for (auto apiHandle : item->apiRefs) {

                if (uniqApiRefsToTopoOrder.find(apiHandle) == uniqApiRefsToTopoOrder.end()) {
                    DEBUG_RECON("level %zu: 0x%llx api ref: 0x%llx",
                                topoLevel,
                                (unsigned long long)handle,
                                (unsigned long long)apiHandle);
                    auto& refs = uniqApiRefsByTopoOrder[topoLevel];
                    refs.push_back(apiHandle);
                }

                uniqApiRefsToTopoOrder[apiHandle] = topoLevel;
            }

            for (auto childHandle : item->childHandles) {
                next.push_back(childHandle);
            }
        }

        next = typeTagSortedHandles(next);

        topoOrder = next;
        ++topoLevel;
    }

    uniqApiRefsByTopoOrder[topoLevel] = getOrderedUniqueModifyApis();
    ++topoLevel;

    size_t totalApiTraceSize = 0; // 4 bytes to store size of created handles

    for (size_t i = 0; i < topoLevel; ++i) {
        for (auto apiHandle : uniqApiRefsByTopoOrder[i]) {
            auto item = mApiTrace.get(apiHandle);
            totalApiTraceSize += 4; // opcode
            totalApiTraceSize += 4; // buffer size of trace
            totalApiTraceSize += item->traceBytes; // the actual trace
        }
    }

    DEBUG_RECON(
        "total api trace size: %zu",
        totalApiTraceSize);

    std::vector<uint64_t> createdHandleBuffer;

    for (size_t i = 0; i < topoLevel; ++i) {
        for (auto apiHandle : uniqApiRefsByTopoOrder[i]) {
            auto item = mApiTrace.get(apiHandle);
            for (auto createdHandle : item->createdHandles) {
                DEBUG_RECON("save handle: 0x%llx\n", createdHandle);
                createdHandleBuffer.push_back(createdHandle);
            }
        }
    }

    std::vector<uint8_t> apiTraceBuffer;
    apiTraceBuffer.resize(totalApiTraceSize);

    uint8_t* apiTracePtr = apiTraceBuffer.data();

    for (size_t i = 0; i < topoLevel; ++i) {
        for (auto apiHandle : uniqApiRefsByTopoOrder[i]) {
            auto item = mApiTrace.get(apiHandle);
            // 4 bytes for opcode, and 4 bytes for saveBufferRaw's size field
            memcpy(apiTracePtr, &item->opCode, sizeof(uint32_t));
            apiTracePtr += 4;
            uint32_t traceBytesForSnapshot = item->traceBytes + 8;
            memcpy(apiTracePtr, &traceBytesForSnapshot, sizeof(uint32_t)); // and 8 bytes for 'self' struct of { opcode, packetlen } as that is what decoder expects
            apiTracePtr += 4;
            memcpy(apiTracePtr, item->trace.data(), item->traceBytes);
            apiTracePtr += item->traceBytes;
        }
    }

    DEBUG_RECON(
        "created handle buffer size: %zu trace: %zu",
        createdHandleBuffer.size(), apiTraceBuffer.size());

    android::base::saveBufferRaw(stream, (char*)(createdHandleBuffer.data()), createdHandleBuffer.size() * sizeof(uint64_t));
    android::base::saveBufferRaw(stream, (char*)(apiTraceBuffer.data()), apiTraceBuffer.size());
}

class TrivialStream : public gfxstream::IOStream {
public:
    TrivialStream() : gfxstream::IOStream(4) { }

    void* allocBuffer(size_t minSize) {
        size_t allocSize = (m_bufsize < minSize ? minSize : m_bufsize);
        if (!m_buf) {
            m_buf = (unsigned char *)malloc(allocSize);
        }
        else if (m_bufsize < allocSize) {
            unsigned char *p = (unsigned char *)realloc(m_buf, allocSize);
            if (p != NULL) {
                m_buf = p;
                m_bufsize = allocSize;
            } else {
                ERR("realloc (%zu) failed\n", allocSize);
                free(m_buf);
                m_buf = NULL;
                m_bufsize = 0;
            }
        }

        return m_buf;
    }

    int commitBuffer(size_t size) {
        if (size == 0) return 0;
        return writeFully(m_buf, size);
    }

    int writeFully(const void *buf, size_t len) {
        return 0;
    }

    const unsigned char* readFully(void *buf, size_t len) {
        return NULL;
    }

    virtual void* getDmaForReading(uint64_t guest_paddr) { return nullptr; }
    virtual void unlockDma(uint64_t guest_paddr) { }

protected:
    virtual const unsigned char *readRaw(void *buf, size_t *inout_len) {
        return nullptr;
    }
    virtual void onSave(android::base::Stream* stream) { }
    virtual unsigned char* onLoad(android::base::Stream* stream) { }
};

void VkReconstruction::load(android::base::Stream* stream) {
    DEBUG_RECON("start. assuming VkDecoderGlobalState has been cleared for loading already");
    mApiTrace.clear();
    mHandleReconstructions.clear();

    std::vector<uint8_t> createdHandleBuffer;
    std::vector<uint8_t> apiTraceBuffer;

    android::base::loadBuffer(stream, &createdHandleBuffer);
    android::base::loadBuffer(stream, &apiTraceBuffer);

    DEBUG_RECON(
        "created handle buffer size: %zu trace: %zu",
        createdHandleBuffer.size(), apiTraceBuffer.size());

    uint32_t createdHandleBufferSize = createdHandleBuffer.size();

    mLoadedTrace.resize(4 + createdHandleBufferSize + apiTraceBuffer.size());

    unsigned char* finalTraceData =
        (unsigned char*)(mLoadedTrace.data());

    memcpy(finalTraceData, &createdHandleBufferSize, sizeof(uint32_t));
    memcpy(finalTraceData + 4, createdHandleBuffer.data(), createdHandleBufferSize);
    memcpy(finalTraceData + 4 + createdHandleBufferSize, apiTraceBuffer.data(), apiTraceBuffer.size());

    VkDecoder decoderForLoading;
    // A decoder that is set for snapshot load will load up the created handles first,
    // if any, allowing us to 'catch' the results as they are decoded.
    decoderForLoading.setForSnapshotLoad(true);
    TrivialStream trivialStream;

    DEBUG_RECON("start decoding trace");

    // TODO: This needs to be the puid seqno ptr
    auto resources = ProcessResources::create();
    decoderForLoading.decode(mLoadedTrace.data(), mLoadedTrace.size(),
                             &trivialStream, resources.get());

    DEBUG_RECON("finished decoding trace");
}

VkReconstruction::ApiHandle VkReconstruction::createApiInfo() {
    auto handle = mApiTrace.add(ApiInfo(), 1);
    return handle;
}

void VkReconstruction::destroyApiInfo(VkReconstruction::ApiHandle h) {
    auto item = mApiTrace.get(h);

    if (!item) return;

    item->traceBytes = 0;
    item->createdHandles.clear();

    mApiTrace.remove(h);
}

VkReconstruction::ApiInfo* VkReconstruction::getApiInfo(VkReconstruction::ApiHandle h) {
    return mApiTrace.get(h);
}

void VkReconstruction::setApiTrace(VkReconstruction::ApiInfo* apiInfo, uint32_t opCode, const uint8_t* traceBegin, size_t traceBytes) {
    if (apiInfo->trace.size() < traceBytes) apiInfo->trace.resize(traceBytes);
    apiInfo->opCode = opCode;
    memcpy(apiInfo->trace.data(), traceBegin, traceBytes);
    apiInfo->traceBytes = traceBytes;
}

void VkReconstruction::dump() {
    fprintf(stderr, "%s: api trace dump\n", __func__);

    size_t traceBytesTotal = 0;

    mApiTrace.forEachLiveEntry_const([&traceBytesTotal](bool live, uint64_t handle, const ApiInfo& info) {
         fprintf(stderr, "VkReconstruction::%s: api handle 0x%llx: %s\n", __func__, (unsigned long long)handle, goldfish_vk::api_opcode_to_string(info.opCode));
         traceBytesTotal += info.traceBytes;
    });

    mHandleReconstructions.forEachLiveComponent_const([this](bool live, uint64_t componentHandle, uint64_t entityHandle, const HandleReconstruction& reconstruction) {
        fprintf(stderr, "VkReconstruction::%s: %p handle 0x%llx api refs:\n", __func__, this, (unsigned long long)entityHandle);
        for (auto apiHandle : reconstruction.apiRefs) {
            auto apiInfo = mApiTrace.get(apiHandle);
            const char* apiName = apiInfo ? goldfish_vk::api_opcode_to_string(apiInfo->opCode) : "unalloced";
            fprintf(stderr, "VkReconstruction::%s:     0x%llx: %s\n", __func__, (unsigned long long)apiHandle, apiName);
            for (auto createdHandle : apiInfo->createdHandles) {
                fprintf(stderr, "VkReconstruction::%s:         created 0x%llx\n", __func__, (unsigned long long)createdHandle);
            }
        }
    });

    mHandleModifications.forEachLiveComponent_const([this](bool live, uint64_t componentHandle, uint64_t entityHandle, const HandleModification& modification) {
        fprintf(stderr, "VkReconstruction::%s: mod: %p handle 0x%llx api refs:\n", __func__, this, (unsigned long long)entityHandle);
        for (auto apiHandle : modification.apiRefs) {
            auto apiInfo = mApiTrace.get(apiHandle);
            const char* apiName = apiInfo ? goldfish_vk::api_opcode_to_string(apiInfo->opCode) : "unalloced";
            fprintf(stderr, "VkReconstruction::%s: mod:     0x%llx: %s\n", __func__, (unsigned long long)apiHandle, apiName);
        }
    });

    fprintf(stderr, "%s: total trace bytes: %zu\n", __func__, traceBytesTotal);
}

void VkReconstruction::addHandles(const uint64_t* toAdd, uint32_t count) {
    if (!toAdd) return;

    for (uint32_t i = 0; i < count; ++i) {
        DEBUG_RECON("add 0x%llx", (unsigned long long)toAdd[i]);
        mHandleReconstructions.add(toAdd[i], HandleReconstruction());
    }
}

void VkReconstruction::removeHandles(const uint64_t* toRemove, uint32_t count) {
    if (!toRemove) return;

    forEachHandleDeleteApi(toRemove, count);

    for (uint32_t i = 0; i < count; ++i) {
        DEBUG_RECON("remove 0x%llx", (unsigned long long)toRemove[i]);
        auto item = mHandleReconstructions.get(toRemove[i]);

        if (!item) continue;

        mHandleReconstructions.remove(toRemove[i]);

        removeHandles(item->childHandles.data(), item->childHandles.size());

        item->childHandles.clear();
    }
}

void VkReconstruction::forEachHandleAddApi(const uint64_t* toProcess, uint32_t count, uint64_t apiHandle) {
    if (!toProcess) return;

    for (uint32_t i = 0; i < count; ++i) {
        auto item = mHandleReconstructions.get(toProcess[i]);

        if (!item) continue;

        item->apiRefs.push_back(apiHandle);
    }
}

void VkReconstruction::forEachHandleDeleteApi(const uint64_t* toProcess, uint32_t count) {
    if (!toProcess) return;

    for (uint32_t i = 0; i < count; ++i) {

        auto item = mHandleReconstructions.get(toProcess[i]);

        if (!item) continue;

        for (auto handle : item->apiRefs) {
            destroyApiInfo(handle);
        }

        item->apiRefs.clear();

        auto modifyItem = mHandleModifications.get(toProcess[i]);

        if (!modifyItem) continue;

        modifyItem->apiRefs.clear();
    }
}

void VkReconstruction::addHandleDependency(const uint64_t* handles, uint32_t count, uint64_t parentHandle) {
    if (!handles) return;

    auto item = mHandleReconstructions.get(parentHandle);

    if (!item) return;

    for (uint32_t i = 0; i < count; ++i) {
        item->childHandles.push_back(handles[i]);
    }
}

void VkReconstruction::setCreatedHandlesForApi(uint64_t apiHandle, const uint64_t* created, uint32_t count) {
    if (!created) return;

    auto item = mApiTrace.get(apiHandle);

    if (!item) return;

    for (uint32_t i = 0; i < count; ++i) {
        item->createdHandles.push_back(created[i]);
    }
}

void VkReconstruction::forEachHandleAddModifyApi(const uint64_t* toProcess, uint32_t count, uint64_t apiHandle) {
    if (!toProcess) return;

    for (uint32_t i = 0; i < count; ++i) {
        mHandleModifications.add(toProcess[i], HandleModification());

        auto item = mHandleModifications.get(toProcess[i]);

        if (!item) continue;

        item->apiRefs.push_back(apiHandle);
    }
}

std::vector<uint64_t> VkReconstruction::getOrderedUniqueModifyApis() const {
    std::vector<HandleModification> orderedModifies;

    // Now add all handle modifications to the trace, ordered by the .order field.
    mHandleModifications.forEachLiveComponent_const(
        [this, &orderedModifies](bool live, uint64_t componentHandle, uint64_t entityHandle, const HandleModification& mod) {
        orderedModifies.push_back(mod);
    });

    // Sort by the |order| field for each modify API
    // since it may be important to apply modifies in a particular
    // order (e.g., when dealing with descriptor set updates
    // or commands in a command buffer).
    std::sort(orderedModifies.begin(), orderedModifies.end(),
        [](const HandleModification& lhs, const HandleModification& rhs) {
        return lhs.order < rhs.order;
    });

    std::unordered_set<uint64_t> usedModifyApis;
    std::vector<uint64_t> orderedUniqueModifyApis;

    for (const auto& mod : orderedModifies) {
        for (auto apiRef : mod.apiRefs) {
            if (usedModifyApis.find(apiRef) == usedModifyApis.end()) {
                orderedUniqueModifyApis.push_back(apiRef);
                usedModifyApis.insert(apiRef);
            }
        }
    }

    return orderedUniqueModifyApis;
}
