// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expresso or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "VkAndroidNativeBuffer.h"

#include "android/base/Log.h"

#include "cereal/common/goldfish_vk_private_defs.h"
#include "cereal/common/goldfish_vk_extension_structs.h"

#include "FrameBuffer.h"
#include "GrallocDefs.h"
#include "VkCommonOperations.h"
#include "VulkanDispatch.h"

#include <string.h>

namespace goldfish_vk {

bool parseAndroidNativeBufferInfo(
    const VkImageCreateInfo* pCreateInfo,
    AndroidNativeBufferInfo* info_out) {

    // Look through the extension chain.
    const void* curr_pNext = pCreateInfo->pNext;
    if (!curr_pNext) return false;

    uint32_t structType = goldfish_vk_struct_type(curr_pNext);

    return structType == VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID;
}

VkResult prepareAndroidNativeBufferImage(
    VulkanDispatch* vk,
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    const VkPhysicalDeviceMemoryProperties* memProps,
    AndroidNativeBufferInfo* out) {

    *out = {};

    out->device = device;
    out->vkFormat = pCreateInfo->format;
    out->extent = pCreateInfo->extent;
    out->usage = pCreateInfo->usage;

    for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; ++i) {
        out->queueFamilyIndices.push_back(
                pCreateInfo->pQueueFamilyIndices[i]);
    }

    const VkNativeBufferANDROID* nativeBufferANDROID =
        reinterpret_cast<const VkNativeBufferANDROID*>(pCreateInfo->pNext);

    out->format = nativeBufferANDROID->format;
    out->stride = nativeBufferANDROID->stride;
    out->colorBufferHandle = *(nativeBufferANDROID->handle);

    bool colorBufferVulkanCompatible =
        isColorBufferVulkanCompatible(out->colorBufferHandle);
    bool externalMemoryCompatible = false;

    auto emu = getGlobalVkEmulation();

    if (emu && emu->live) {
        externalMemoryCompatible =
            emu->deviceInfo.supportsExternalMemory;
    }

    if (colorBufferVulkanCompatible &&
        externalMemoryCompatible &&
        setupVkColorBuffer(out->colorBufferHandle, false /* not Vulkan only */, &out->isGlTexture)) {
        out->externallyBacked = true;
    }

    // delete the info struct and pass to vkCreateImage, and also add
    // transfer src capability to allow us to copy to CPU.
    VkImageCreateInfo infoNoNative = *pCreateInfo;
    infoNoNative.pNext = nullptr;
    infoNoNative.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    if (out->externallyBacked) {
        // Create the image with extension structure about external backing.
        VkExternalMemoryImageCreateInfo extImageCi = {
            VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, 0,
            VK_EXT_MEMORY_HANDLE_TYPE_BIT,
        };

        infoNoNative.pNext = &extImageCi;

        VkResult createResult =
            vk->vkCreateImage(
                device, &infoNoNative, pAllocator, &out->image);

        if (createResult != VK_SUCCESS) return createResult;

        // Now import the backing memory.
        const auto& cbInfo = getColorBufferInfo(out->colorBufferHandle);
        const auto& memInfo = cbInfo.memory;

        vk->vkGetImageMemoryRequirements(
            device, out->image, &out->memReqs);

        if (out->memReqs.size < memInfo.size) {
            out->memReqs.size = memInfo.size;
        }

        if (!importExternalMemory(vk, device, &memInfo, &out->imageMemory)) {
            fprintf(stderr, "%s: Failed to import external memory\n", __func__);
            return VK_ERROR_INITIALIZATION_FAILED;
        }

    } else {
        VkResult createResult =
            vk->vkCreateImage(
                device, &infoNoNative, pAllocator, &out->image);

        if (createResult != VK_SUCCESS) return createResult;

        vk->vkGetImageMemoryRequirements(
            device, out->image, &out->memReqs);

        uint32_t imageMemoryTypeIndex = 0;
        bool imageMemoryTypeIndexFound = false;

        for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
            bool supported =
                out->memReqs.memoryTypeBits & (1 << i);
            if (supported) {
                imageMemoryTypeIndex = i;
                imageMemoryTypeIndexFound = true;
                break;
            }
        }

        if (!imageMemoryTypeIndexFound) {
            LOG(ERROR) <<
                "VK_ANDROID_native_buffer: could not obtain "
                "image memory type index";
            teardownAndroidNativeBufferImage(vk, out);
            return VK_ERROR_OUT_OF_DEVICE_MEMORY;
        }

        out->imageMemoryTypeIndex = imageMemoryTypeIndex;

        VkMemoryAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
            out->memReqs.size,
            out->imageMemoryTypeIndex,
        };

        if (VK_SUCCESS !=
            vk->vkAllocateMemory(
                device, &allocInfo, nullptr,
                &out->imageMemory)) {
            LOG(ERROR) <<
                "VK_ANDROID_native_buffer: could not allocate "
                "image memory. requested size: " <<
                    out->memReqs.size;
            teardownAndroidNativeBufferImage(vk, out);
            return VK_ERROR_OUT_OF_DEVICE_MEMORY;
        }
    }

    if (VK_SUCCESS !=
        vk->vkBindImageMemory(
            device, out->image, out->imageMemory, 0)) {
        LOG(ERROR) <<
            "VK_ANDROID_native_buffer: could not bind "
            "image memory.";
        teardownAndroidNativeBufferImage(vk, out);
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    // Allocate a staging memory and set up the staging buffer.
    // TODO: Make this shared as well if we can get that to
    // work on Windows with NVIDIA.
    {
        bool stagingIndexRes =
            getStagingMemoryTypeIndex(
                vk, device, memProps, &out->stagingMemoryTypeIndex);

        if (!stagingIndexRes) {
            LOG(ERROR) <<
                "VK_ANDROID_native_buffer: could not obtain "
                "staging memory type index";
            teardownAndroidNativeBufferImage(vk, out);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        VkMemoryAllocateInfo allocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
            out->memReqs.size,
            out->stagingMemoryTypeIndex,
        };

        if (VK_SUCCESS !=
            vk->vkAllocateMemory(
                device, &allocInfo, nullptr,
                &out->stagingMemory)) {
            LOG(ERROR) <<
                "VK_ANDROID_native_buffer: could not allocate "
                "staging memory. requested size: " <<
                    out->memReqs.size;
            teardownAndroidNativeBufferImage(vk, out);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        VkBufferCreateInfo stagingBufferCreateInfo = {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
            out->memReqs.size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            out->sharingMode,
            (uint32_t)out->queueFamilyIndices.size(),
            out->queueFamilyIndices.size() ? out->queueFamilyIndices.data() : nullptr,
        };

        if (VK_SUCCESS !=
            vk->vkCreateBuffer(
                device, &stagingBufferCreateInfo, nullptr,
                &out->stagingBuffer)) {
            LOG(ERROR) <<
                "VK_ANDROID_native_buffer: could not create "
                "staging buffer.";
            teardownAndroidNativeBufferImage(vk, out);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        if (VK_SUCCESS !=
            vk->vkBindBufferMemory(
                device, out->stagingBuffer, out->stagingMemory, 0)) {
            LOG(ERROR) <<
                "VK_ANDROID_native_buffer: could not bind "
                "staging buffer to staging memory.";
            teardownAndroidNativeBufferImage(vk, out);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        if (VK_SUCCESS !=
            vk->vkMapMemory(
                device, out->stagingMemory, 0,
                out->memReqs.size, 0,
                (void**)&out->mappedStagingPtr)) {
            LOG(ERROR) <<
                "VK_ANDROID_native_buffer: could not map "
                "staging buffer.";
            teardownAndroidNativeBufferImage(vk, out);
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
    }

    return VK_SUCCESS;
}

void teardownAndroidNativeBufferImage(
    VulkanDispatch* vk, AndroidNativeBufferInfo* anbInfo) {
    auto device = anbInfo->device;

    auto image = anbInfo->image;
    auto imageMemory = anbInfo->imageMemory;

    auto stagingBuffer = anbInfo->stagingBuffer;
    auto mappedPtr = anbInfo->mappedStagingPtr;
    auto stagingMemory = anbInfo->stagingMemory;

    if (image) vk->vkDestroyImage(device, image, nullptr);
    if (imageMemory) vk->vkFreeMemory(device, imageMemory, nullptr);
    if (stagingBuffer) vk->vkDestroyBuffer(device, stagingBuffer, nullptr);
    if (mappedPtr) vk->vkUnmapMemory(device, stagingMemory);
    if (stagingMemory) vk->vkFreeMemory(device, stagingMemory, nullptr);

    for (auto queueState : anbInfo->queueStates) {
        queueState.teardown(vk, device);
    }
    anbInfo->queueStates.clear();

    anbInfo->acquireQueueState.teardown(vk, device);

    if (anbInfo->externallyBacked) {
        teardownVkColorBuffer(anbInfo->colorBufferHandle);
    }

    *anbInfo = {};
}

void getGralloc0Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      int* usage_out) {
    // Pick some default flexible values for gralloc usage for now.
    (void)format;
    (void)imageUsage;
    *usage_out =
        GRALLOC_USAGE_SW_READ_OFTEN |
        GRALLOC_USAGE_SW_WRITE_OFTEN |
        GRALLOC_USAGE_HW_RENDER |
        GRALLOC_USAGE_HW_TEXTURE;
}

// Taken from Android GrallocUsageConversion.h
void getGralloc1Usage(VkFormat format, VkImageUsageFlags imageUsage,
                      VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
                      uint64_t* consumerUsage_out,
                      uint64_t* producerUsage_out) {
    // Pick some default flexible values for gralloc usage for now.
    (void)format;
    (void)imageUsage;
    (void)swapchainImageUsage;

    constexpr int usage =
        GRALLOC_USAGE_SW_READ_OFTEN |
        GRALLOC_USAGE_SW_WRITE_OFTEN |
        GRALLOC_USAGE_HW_RENDER |
        GRALLOC_USAGE_HW_TEXTURE;

    constexpr uint64_t PRODUCER_MASK =
            GRALLOC1_PRODUCER_USAGE_CPU_READ |
            /* GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN | */
            GRALLOC1_PRODUCER_USAGE_CPU_WRITE |
            /* GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN | */
            GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET |
            GRALLOC1_PRODUCER_USAGE_PROTECTED |
            GRALLOC1_PRODUCER_USAGE_CAMERA |
            GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER |
            GRALLOC1_PRODUCER_USAGE_SENSOR_DIRECT_DATA;
    constexpr uint64_t CONSUMER_MASK =
            GRALLOC1_CONSUMER_USAGE_CPU_READ |
            /* GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN | */
            GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE |
            GRALLOC1_CONSUMER_USAGE_HWCOMPOSER |
            GRALLOC1_CONSUMER_USAGE_CLIENT_TARGET |
            GRALLOC1_CONSUMER_USAGE_CURSOR |
            GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER |
            GRALLOC1_CONSUMER_USAGE_CAMERA |
            GRALLOC1_CONSUMER_USAGE_RENDERSCRIPT |
            GRALLOC1_CONSUMER_USAGE_GPU_DATA_BUFFER;

    *producerUsage_out = static_cast<uint64_t>(usage) & PRODUCER_MASK;
    *consumerUsage_out = static_cast<uint64_t>(usage) & CONSUMER_MASK;

    if ((static_cast<uint32_t>(usage) & GRALLOC_USAGE_SW_READ_OFTEN) ==
        GRALLOC_USAGE_SW_READ_OFTEN) {
        *producerUsage_out |= GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN;
        *consumerUsage_out |= GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN;
    }

    if ((static_cast<uint32_t>(usage) & GRALLOC_USAGE_SW_WRITE_OFTEN) ==
        GRALLOC_USAGE_SW_WRITE_OFTEN) {
        *producerUsage_out |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN;
    }
}

void AndroidNativeBufferInfo::QueueState::setup(
    VulkanDispatch* vk,
    VkDevice device,
    VkQueue queueIn,
    uint32_t queueFamilyIndexIn) {

    queue = queueIn;
    queueFamilyIndex = queueFamilyIndexIn;

    VkCommandPoolCreateInfo poolCreateInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        queueFamilyIndex,
    };

    vk->vkCreateCommandPool(
        device,
        &poolCreateInfo,
        nullptr,
        &pool);

    VkCommandBufferAllocateInfo cbAllocInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
        pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
    };

    vk->vkAllocateCommandBuffers(
        device,
        &cbAllocInfo,
        &cb);

    vk->vkAllocateCommandBuffers(
        device,
        &cbAllocInfo,
        &cb2);

    VkFenceCreateInfo fenceCreateInfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 0, 0,
    };

    vk->vkCreateFence(
        device,
        &fenceCreateInfo,
        nullptr,
        &fence);
}

void AndroidNativeBufferInfo::QueueState::teardown(
    VulkanDispatch* vk, VkDevice device) {

    if (cb) vk->vkFreeCommandBuffers(device, pool, 1, &cb);
    if (pool) vk->vkDestroyCommandPool(device, pool, nullptr);
    if (fence) vk->vkDestroyFence(device, fence, nullptr);

    queue = VK_NULL_HANDLE;
    pool = VK_NULL_HANDLE;
    cb = VK_NULL_HANDLE;
    fence = VK_NULL_HANDLE;
    queueFamilyIndex = 0;
}

VkResult setAndroidNativeImageSemaphoreSignaled(
    VulkanDispatch* vk,
    VkDevice device,
    VkQueue defaultQueue,
    uint32_t defaultQueueFamilyIndex,
    VkSemaphore semaphore,
    VkFence fence,
    AndroidNativeBufferInfo* anbInfo) {

    auto fb = FrameBuffer::getFB();

    bool firstTimeSetup =
        !anbInfo->everSynced &&
        !anbInfo->everAcquired;

    anbInfo->everAcquired = true;
        // fprintf(stderr, "%s: call\n", __func__);

    if (firstTimeSetup) {

        VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
            0, nullptr, nullptr,
            0, nullptr,
            1, &semaphore,
        };

        vk->vkQueueSubmit(defaultQueue, 1, &submitInfo, fence);
    } else {

        const AndroidNativeBufferInfo::QueueState& queueState =
                anbInfo->queueStates[anbInfo->lastUsedQueueFamilyIndex];

        // For GL interop, transfer back to present layout from general.
        if (anbInfo->isGlTexture) {
            fb->setColorBufferInUse(anbInfo->colorBufferHandle, true);

            VkCommandBufferBeginInfo beginInfo = {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                0,
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                nullptr /* no inheritance info */,
            };

            vk->vkBeginCommandBuffer(queueState.cb2, &beginInfo);

            VkImageMemoryBarrier backToPresentSrc = {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 0,
                VK_ACCESS_HOST_READ_BIT, 0,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                anbInfo->image,
                {
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    0, 1, 0, 1,
                },
            };

            vk->vkCmdPipelineBarrier(queueState.cb2,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                    nullptr, 0, nullptr, 1, &backToPresentSrc);

            vk->vkEndCommandBuffer(queueState.cb2);

            VkSubmitInfo submitInfo = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO,
                0,
                0,
                nullptr,
                nullptr,
                1,
                &queueState.cb2,
                1,
                &semaphore,
            };

            vk->vkQueueSubmit(queueState.queue, 1, &submitInfo, fence);
        } else {
            const AndroidNativeBufferInfo::QueueState&
                queueState = anbInfo->queueStates[anbInfo->lastUsedQueueFamilyIndex];
            VkSubmitInfo submitInfo = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
                0, nullptr, nullptr,
                0, nullptr,
                1, &semaphore,
            };
            vk->vkQueueSubmit(queueState.queue, 1, &submitInfo, fence);
        }
    }

    return VK_SUCCESS;
}

VkResult syncImageToColorBuffer(
    VulkanDispatch* vk,
    uint32_t queueFamilyIndex,
    VkQueue queue,
    uint32_t waitSemaphoreCount,
    const VkSemaphore* pWaitSemaphores,
    int* pNativeFenceFd,
    AndroidNativeBufferInfo* anbInfo) {

    auto fb = FrameBuffer::getFB();
    fb->lock();

    // Implicitly synchronized
    *pNativeFenceFd = -1;

    anbInfo->everSynced = true;
    anbInfo->lastUsedQueueFamilyIndex = queueFamilyIndex;

    // Setup queue state for this queue family index.
    if (queueFamilyIndex >= anbInfo->queueStates.size()) {
        anbInfo->queueStates.resize(queueFamilyIndex + 1);
    }

    auto& queueState = anbInfo->queueStates[queueFamilyIndex];

    if (!queueState.queue) {
        queueState.setup(
            vk, anbInfo->device, queue, queueFamilyIndex);
    }

    // Record our synchronization commands.
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        nullptr /* no inheritance info */,
    };

    vk->vkBeginCommandBuffer(queueState.cb, &beginInfo);

    // If GL texture, transfer image layout to "general" for GL interop.
    if (anbInfo->isGlTexture) {
        VkImageMemoryBarrier present2GeneralBarrier = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 0,
            VK_ACCESS_HOST_READ_BIT, 0,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            anbInfo->image,
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1, 0, 1,
            },
        };

        vk->vkCmdPipelineBarrier(
            queueState.cb,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &present2GeneralBarrier);

    } else {
        // Not a GL texture. Read it back and put it back in present layout.

        // From the spec: If an application does not need the contents of a resource
        // to remain valid when transferring from one queue family to another, then
        // the ownership transfer should be skipped.
        // We definitely need to transition the image to
        // VK_TRANSFER_SRC_OPTIMAL and back.
        VkImageMemoryBarrier presentToTransferSrc = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 0,
            0,
            VK_ACCESS_HOST_READ_BIT,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            anbInfo->image,
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1, 0, 1,
            },
        };

        vk->vkCmdPipelineBarrier(
            queueState.cb,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &presentToTransferSrc);

        VkBufferImageCopy region = {
            0 /* buffer offset */,
            anbInfo->extent.width,
            anbInfo->extent.height,
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 0, 1,
            },
            { 0, 0, 0 },
            anbInfo->extent,
        };

        vk->vkCmdCopyImageToBuffer(
            queueState.cb,
            anbInfo->image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            anbInfo->stagingBuffer,
            1, &region);

        // Transfer back to present src.
        VkImageMemoryBarrier backToPresentSrc = {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 0,
            VK_ACCESS_HOST_READ_BIT,
            0,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            anbInfo->image,
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1, 0, 1,
            },
        };

        vk->vkCmdPipelineBarrier(
            queueState.cb,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &backToPresentSrc);

    }

    vk->vkEndCommandBuffer(queueState.cb);

    std::vector<VkPipelineStageFlags> pipelineStageFlags;
    pipelineStageFlags.resize(waitSemaphoreCount, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

    VkSubmitInfo submitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
        waitSemaphoreCount, pWaitSemaphores,
        pipelineStageFlags.data(),
        1, &queueState.cb,
        0, nullptr,
    };

    vk->vkQueueSubmit(queueState.queue, 1, &submitInfo, queueState.fence);

    static constexpr uint64_t ANB_MAX_WAIT_NS =
        5ULL * 1000ULL * 1000ULL * 1000ULL;

    vk->vkWaitForFences(
        anbInfo->device, 1, &queueState.fence, VK_TRUE, ANB_MAX_WAIT_NS);
    vk->vkResetFences(anbInfo->device, 1, &queueState.fence);

    fb->unlock();

    if (anbInfo->isGlTexture) {
        fb->setColorBufferInUse(anbInfo->colorBufferHandle, false);
    } else {

        VkMappedMemoryRange toInvalidate = {
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
            anbInfo->stagingMemory,
            0, VK_WHOLE_SIZE,
        };

        vk->vkInvalidateMappedMemoryRanges(
            anbInfo->device, 1, &toInvalidate);

        uint32_t colorBufferHandle = anbInfo->colorBufferHandle;

        // Copy to from staging buffer to color buffer
        uint32_t bpp = 4; /* format always rgba8...not */
        switch (anbInfo->vkFormat) {
            case VK_FORMAT_R5G6B5_UNORM_PACK16:
                bpp = 2;
                break;
            case VK_FORMAT_R8G8B8_UNORM:
                bpp = 3;
                break;
            default:
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_UNORM:
                bpp = 4;
                break;
        }

        FrameBuffer::getFB()->
            replaceColorBufferContents(
                colorBufferHandle,
                anbInfo->mappedStagingPtr,
                bpp * anbInfo->extent.width * anbInfo->extent.height);
    }

    return VK_SUCCESS;
}

} // namespace goldfish_vk
