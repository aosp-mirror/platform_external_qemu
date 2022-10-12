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
#pragma once

#include "aemu/base/files/Stream.h"
#include "aemu/base/containers/EntityManager.h"

#include "VulkanHandleMapping.h"
#include "VulkanHandles.h"

#include "common/goldfish_vk_marshaling.h"

// A class that captures all important data structures for
// reconstructing a Vulkan system state via trimmed API record and replay.
class VkReconstruction {
public:
    VkReconstruction();

    void save(android::base::Stream* stream);
    void load(android::base::Stream* stream);

    struct ApiInfo {
        // Fast
        uint32_t opCode;
        std::vector<uint8_t> trace;
        size_t traceBytes = 0;
        // Book-keeping for which handles were created by this API
        std::vector<uint64_t> createdHandles;
    };

    using ApiTrace =
        android::base::EntityManager<32, 16, 16, ApiInfo>;
    using ApiHandle = ApiTrace::EntityHandle;

    struct HandleReconstruction {
        std::vector<ApiHandle> apiRefs;
        std::vector<uint64_t> childHandles;
    };

    using HandleReconstructions =
        android::base::UnpackedComponentManager<32, 16, 16, HandleReconstruction>;

    struct HandleModification {
        std::vector<ApiHandle> apiRefs;
        uint32_t order = 0;
    };

    using HandleModifications =
        android::base::UnpackedComponentManager<32, 16, 16, HandleModification>;

    ApiHandle createApiInfo();
    void destroyApiInfo(ApiHandle h);

    ApiInfo* getApiInfo(ApiHandle h);

    void setApiTrace(ApiInfo* apiInfo, uint32_t opcode, const uint8_t* traceBegin, size_t traceBytes);

    void dump();

    void addHandles(const uint64_t* toAdd, uint32_t count);
    void removeHandles(const uint64_t* toRemove, uint32_t count);

    void forEachHandleAddApi(const uint64_t* toProcess, uint32_t count, uint64_t apiHandle);
    void forEachHandleDeleteApi(const uint64_t* toProcess, uint32_t count);

    void addHandleDependency(const uint64_t* handles, uint32_t count, uint64_t parentHandle);

    void setCreatedHandlesForApi(uint64_t apiHandle, const uint64_t* created, uint32_t count);

    void forEachHandleAddModifyApi(const uint64_t* toProcess, uint32_t count, uint64_t apiHandle);

    void setModifiedHandlesForApi(uint64_t apiHandle, const uint64_t* modified, uint32_t count);
private:

    std::vector<uint64_t> getOrderedUniqueModifyApis() const;

    ApiTrace mApiTrace;

    HandleReconstructions mHandleReconstructions;
    HandleModifications mHandleModifications;

    std::vector<uint8_t> mLoadedTrace;
};
