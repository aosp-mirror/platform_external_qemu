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

#include "android/base/containers/EntityManager.h"

#include "VulkanHandleMapping.h"
#include "VulkanHandles.h"

#include "goldfish_vk_baseprotodefs.pb.h"
#include "common/goldfish_vk_baseprotoconversion.h"

// A class that captures all important data structures for
// reconstructing a Vulkan system state via trimmed API record and replay.
class VkReconstruction {
public:
    VkReconstruction();

    struct ApiInfo {
        goldfish_vk_proto::VkApiCall apiCall;
    };

    using ApiTrace =
        android::base::EntityManager<32, 16, 16, ApiInfo>;
    using ApiHandle = ApiTrace::EntityHandle;

    struct HandleReconstruction {
        std::vector<ApiHandle> apiRefs;
    };

    using HandleReconstructions =
        android::base::UnpackedComponentManager<32, 16, 16, HandleReconstruction>;

    ApiHandle createApiInfo();
    void destroyApiInfo(ApiHandle h);

    ApiInfo* getApiInfo(ApiHandle h);

    void dump();

#define HANDLE_RECONSTRUCTION_API_DECL(type) \
    void addReconstruction_##type(const type* toAdd, uint32_t count); \
    void removeReconstruction_##type(const type* toRemove, uint32_t count); \
    void forEachReconstructionAddApiRef_##type(const type* toProcess, uint32_t count, uint64_t apiHandle); \
    void forEachReconstructionDeleteApiRefs_##type(const type* toProcess, uint32_t count); \

    GOLDFISH_VK_LIST_HANDLE_TYPES(HANDLE_RECONSTRUCTION_API_DECL)

private:

    ApiTrace mApiTrace;

#define DEFINE_HANDLE_RECONSTRUCTION_MEMBER(type) \
    HandleReconstructions mReconstructions_##type; \

    GOLDFISH_VK_LIST_HANDLE_TYPES(DEFINE_HANDLE_RECONSTRUCTION_MEMBER)
};
