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

VkReconstruction::VkReconstruction() = default;

void VkReconstruction::save(android::base::Stream* stream) {
}

void VkReconstruction::load(android::base::Stream* stream) {
}

VkReconstruction::ApiHandle VkReconstruction::createApiInfo() {
    return mApiTrace.add(ApiInfo(), 1);
}

void VkReconstruction::destroyApiInfo(VkReconstruction::ApiHandle h) {
    mApiTrace.remove(h);
    dump();
}

VkReconstruction::ApiInfo* VkReconstruction::getApiInfo(VkReconstruction::ApiHandle h) {
    return mApiTrace.get(h);
}

void VkReconstruction::dump() {
    fprintf(stderr, "%s: api trace dump\n", __func__);
    mApiTrace.forEachLiveEntry([](bool live, uint64_t handle, ApiInfo& info) {
        fprintf(stderr, "%s: [%s]\n", __func__, info.apiCall.DebugString().c_str());
    });
}

#define HANDLE_RECONSTRUCTION_API_DEF(type) \
    void VkReconstruction::addReconstruction_##type(const type* toAdd, uint32_t count) { \
        if (!toAdd) return; \
        for (uint32_t i = 0; i < count; ++i) { \
            mReconstructions_##type.add((uint64_t)(uintptr_t)toAdd[i], HandleReconstruction()); \
        } \
    } \
    void VkReconstruction::removeReconstruction_##type(const type* toRemove, uint32_t count) { \
        if (!toRemove) return; \
        for (uint32_t i = 0; i < count; ++i) { \
            mReconstructions_##type.remove((uint64_t)(uintptr_t)toRemove[i]); \
        } \
    } \
    void VkReconstruction::forEachReconstructionAddApiRef_##type(const type* toProcess, uint32_t count, uint64_t apiHandle) { \
        if (!toProcess) return; \
        for (uint32_t i = 0; i < count; ++i) { \
            auto item = mReconstructions_##type.get((uint64_t)(uintptr_t)toProcess[i]); \
            if (!item) continue; \
            item->apiRefs.push_back(apiHandle); \
        } \
    } \
    void VkReconstruction::forEachReconstructionDeleteApiRefs_##type(const type* toProcess, uint32_t count) { \
        if (!toProcess) return; \
        for (uint32_t i = 0; i < count; ++i) { \
            auto item = mReconstructions_##type.get((uint64_t)(uintptr_t)toProcess[i]); \
            if (!item) continue; \
            for (auto handle : item->apiRefs) { \
                destroyApiInfo(handle); \
            } \
            item->apiRefs.clear(); \
        } \
    } \

GOLDFISH_VK_LIST_HANDLE_TYPES(HANDLE_RECONSTRUCTION_API_DEF)
