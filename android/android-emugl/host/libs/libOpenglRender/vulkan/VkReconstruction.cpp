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

#include "VkDecoder.h"

#include <unordered_map>

VkReconstruction::VkReconstruction() = default;

void VkReconstruction::save(android::base::Stream* stream) {
    size_t totalApiTraceSize = 0;
    mApiTrace.forEachLiveEntry([&totalApiTraceSize](bool live, uint64_t handle, ApiInfo& info) {
         totalApiTraceSize += 8;
         totalApiTraceSize += info.traceBytes;
    });

    stream->putBe32(totalApiTraceSize);

    // TODO: ensure api trace can be loaded in this sequence without violating dependencies
    // We need to topo sort then save them in that order

    std::unordered_map<uint64_t, uint64_t> backDeps;

    mHandleReconstructions.forEachLiveComponent(
        [&backDeps](bool live, uint64_t componentHandle, uint64_t entityHandle, HandleReconstruction& item) {
        for (auto handle : item.childHandles) {
            backDeps[handle] = entityHandle;
        }
    });

    std::vector<uint64_t> topoOrder;

    mHandleReconstructions.forEachLiveComponent(
        [&topoOrder, &backDeps](bool live, uint64_t componentHandle, uint64_t entityHandle, HandleReconstruction& item) {
        // Start with populating the roots
        if (backDeps.find(entityHandle) == backDeps.end()) {
            topoOrder.push_back(entityHandle);
        }
    });

    std::vector<uint64_t> next;

    std::unordered_map<uint64_t, uint64_t> uniqApiRefsToTopoOrder;
    std::unordered_map<uint64_t, std::vector<uint64_t>> uniqApiRefsByTopoOrder;

    size_t topoLevel = 0;

    while (!topoOrder.empty()) {
        next.clear();

        for (auto handle : topoOrder) {
            auto item = mHandleReconstructions.get(handle);

            for (auto apiHandle : item->apiRefs) {

                if (uniqApiRefsToTopoOrder.find(apiHandle) == uniqApiRefsToTopoOrder.end()) {
                    auto& refs = uniqApiRefsByTopoOrder[topoLevel];
                    refs.push_back(apiHandle);
                }

                uniqApiRefsToTopoOrder[apiHandle] = topoLevel;
            }

            for (auto childHandle : item->childHandles) {
                next.push_back(childHandle);
            }
        }

        topoOrder = next;
        ++topoLevel;
    }

    for (size_t i = 0; i < topoLevel; ++i) {
        for (auto apiHandle : uniqApiRefsByTopoOrder[i]) {
            auto item = mApiTrace.get(apiHandle);
            stream->putBe32(item->opCode);
            stream->putBe32(item->traceBytes);
            android::base::saveBufferRaw(stream, (char*)item->trace.data(), item->traceBytes);
        }
    }
}

void VkReconstruction::load(android::base::Stream* stream) {
    mApiTrace.clear();
    mHandleReconstructions.clear();

    // Load what is to be decoded.
    android::base::loadBuffer(stream, &mLoadedTrace);

    // Loading the snapshot via decoding
    // will implicitly repopulate mApiTrace and mHandleReconstructions,
    // assuming stuff was saved in topological order.
    VkDecoder decoderForLoading;
    decoderForLoading.decode(mLoadedTrace.data(), mLoadedTrace.size(), nullptr /* TODO: trivial return stream */);
}

VkReconstruction::ApiHandle VkReconstruction::createApiInfo() {
    auto handle = mApiTrace.add(ApiInfo(), 1);
    return handle;
}

void VkReconstruction::destroyApiInfo(VkReconstruction::ApiHandle h) {
    auto item = mApiTrace.get(h);

    if (!item) return;

    item->apiCall.Clear();

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

    mApiTrace.forEachLiveEntry([&traceBytesTotal](bool live, uint64_t handle, ApiInfo& info) {
         fprintf(stderr, "VkReconstruction::%s: api handle 0x%llx: %s\n", __func__, (unsigned long long)handle, goldfish_vk::api_opcode_to_string(info.opCode));
         traceBytesTotal += info.traceBytes;
    });

    mHandleReconstructions.forEachLiveComponent([this](bool live, uint64_t componentHandle, uint64_t entityHandle, HandleReconstruction& reconstruction) {
        fprintf(stderr, "VkReconstruction::%s: %p handle 0x%llx api refs:\n", __func__, this, (unsigned long long)entityHandle);
        for (auto apiHandle : reconstruction.apiRefs) {
            auto apiInfo = mApiTrace.get(apiHandle);
            const char* apiName = apiInfo ? goldfish_vk::api_opcode_to_string(apiInfo->opCode) : "unalloced";
            fprintf(stderr, "VkReconstruction::%s:     0x%llx: %s\n", __func__, (unsigned long long)apiHandle, apiName);
        }
    });

    fprintf(stderr, "%s: total trace bytes: %zu\n", __func__, traceBytesTotal);
}

void VkReconstruction::addHandles(const uint64_t* toAdd, uint32_t count) {
    if (!toAdd) return;

    for (uint32_t i = 0; i < count; ++i) {
        fprintf(stderr, "VkReconstruction::%s add 0x%llx\n", __func__, (unsigned long long)toAdd[i]);
        mHandleReconstructions.add((uint64_t)(uintptr_t)toAdd[i], HandleReconstruction());
    }
}

void VkReconstruction::removeHandles(const uint64_t* toRemove, uint32_t count) {
    if (!toRemove) return;

    forEachHandleDeleteApi(toRemove, count);

    for (uint32_t i = 0; i < count; ++i) {
        fprintf(stderr, "VkReconstruction::%s remove 0x%llx\n", __func__, (unsigned long long)toRemove[i]);
        auto item = mHandleReconstructions.get((uint64_t)(uintptr_t)toRemove[i]);

        if (!item) continue;

        mHandleReconstructions.remove((uint64_t)(uintptr_t)toRemove[i]);

        removeHandles(item->childHandles.data(), item->childHandles.size());

        item->childHandles.clear();
    }
}

void VkReconstruction::forEachHandleAddApi(const uint64_t* toProcess, uint32_t count, uint64_t apiHandle) {
    if (!toProcess) return;

    for (uint32_t i = 0; i < count; ++i) {
        auto item = mHandleReconstructions.get((uint64_t)(uintptr_t)toProcess[i]);

        if (!item) continue;

        item->apiRefs.push_back(apiHandle);
    }
}

void VkReconstruction::forEachHandleDeleteApi(const uint64_t* toProcess, uint32_t count) {
    if (!toProcess) return;

    for (uint32_t i = 0; i < count; ++i) {

        auto item = mHandleReconstructions.get((uint64_t)(uintptr_t)toProcess[i]);

        if (!item) continue;

        for (auto handle : item->apiRefs) {
            destroyApiInfo(handle);
        }

        item->apiRefs.clear();
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
