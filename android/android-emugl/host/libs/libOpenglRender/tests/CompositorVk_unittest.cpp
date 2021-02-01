// Copyright (C) 2021 The Android Open Source Project
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
#include "CompositorVk.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <optional>

#include "vulkan/VulkanDispatch.h"

static std::optional<uint32_t> findMemoryType(
    const goldfish_vk::VulkanDispatch &vk, VkPhysicalDevice device,
    uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vk.vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }
    return std::nullopt;
}

class CompositorVkTest : public ::testing::Test {
   protected:
    struct RenderTarget {
       public:
        static constexpr VkFormat k_vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
        static constexpr uint32_t k_bpp = 4;
        static constexpr VkImageLayout k_vkImageLayout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        const goldfish_vk::VulkanDispatch &m_vk;
        VkDevice m_vkDevice;
        VkPhysicalDevice m_vkPhysicalDevice;
        VkQueue m_vkQueue;
        uint32_t m_width;
        uint32_t m_height;

        VkImage m_vkImage;
        VkDeviceMemory m_imageVkDeviceMemory;
        VkImageView m_vkImageView;

        VkBuffer m_vkBuffer;
        VkDeviceMemory m_bufferVkDeviceMemory;
        uint32_t *m_memory;

        VkCommandPool m_vkCommandPool;
        VkCommandBuffer m_readCommandBuffer;
        VkCommandBuffer m_writeCommandBuffer;

        static std::unique_ptr<const RenderTarget> create(
            const goldfish_vk::VulkanDispatch &vk, VkDevice device,
            VkPhysicalDevice physicalDevice, VkQueue queue,
            VkCommandPool commandPool, uint32_t width, uint32_t height) {
            std::unique_ptr<RenderTarget> res(new RenderTarget(vk));
            res->m_vkDevice = device;
            res->m_vkPhysicalDevice = physicalDevice;
            res->m_vkQueue = queue;
            res->m_width = width;
            res->m_height = height;
            res->m_vkCommandPool = commandPool;
            if (!res->setUpImage()) {
                return nullptr;
            }
            if (!res->setUpBuffer()) {
                return nullptr;
            }
            if (!res->setUpCommandBuffer()) {
                return nullptr;
            }

            return res;
        }

        uint32_t numOfPixels() const { return m_width * m_height; }

        bool write(const std::vector<uint32_t> &pixels) const {
            if (pixels.size() != numOfPixels()) {
                return false;
            }
            std::copy(pixels.begin(), pixels.end(), m_memory);
            return submitCommandBufferAndWait(m_writeCommandBuffer);
        }

        std::optional<std::vector<uint32_t>> read() const {
            std::vector<uint32_t> res(numOfPixels());
            if (!submitCommandBufferAndWait(m_readCommandBuffer)) {
                return std::nullopt;
            }
            std::copy(m_memory, m_memory + numOfPixels(), res.begin());
            return res;
        }

        ~RenderTarget() {
            std::vector<VkCommandBuffer> toFree;
            if (m_writeCommandBuffer != VK_NULL_HANDLE) {
                toFree.push_back(m_writeCommandBuffer);
            }
            if (m_readCommandBuffer != VK_NULL_HANDLE) {
                toFree.push_back(m_readCommandBuffer);
            }
            if (!toFree.empty()) {
                m_vk.vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool,
                                          static_cast<uint32_t>(toFree.size()),
                                          toFree.data());
            }
            if (m_memory) {
                m_vk.vkUnmapMemory(m_vkDevice, m_bufferVkDeviceMemory);
            }
            if (m_bufferVkDeviceMemory != VK_NULL_HANDLE) {
                m_vk.vkFreeMemory(m_vkDevice, m_bufferVkDeviceMemory, nullptr);
            }
            if (m_vkBuffer != VK_NULL_HANDLE) {
                m_vk.vkDestroyBuffer(m_vkDevice, m_vkBuffer, nullptr);
            }
            if (m_vkImageView != VK_NULL_HANDLE) {
                m_vk.vkDestroyImageView(m_vkDevice, m_vkImageView, nullptr);
            }
            if (m_imageVkDeviceMemory != VK_NULL_HANDLE) {
                m_vk.vkFreeMemory(m_vkDevice, m_imageVkDeviceMemory, nullptr);
            }
            if (m_vkImage != VK_NULL_HANDLE) {
                m_vk.vkDestroyImage(m_vkDevice, m_vkImage, nullptr);
            }
        }

       private:
        RenderTarget(const goldfish_vk::VulkanDispatch &vk)
            : m_vk(vk),
              m_vkImage(VK_NULL_HANDLE),
              m_imageVkDeviceMemory(VK_NULL_HANDLE),
              m_vkImageView(VK_NULL_HANDLE),
              m_vkBuffer(VK_NULL_HANDLE),
              m_bufferVkDeviceMemory(VK_NULL_HANDLE),
              m_memory(nullptr),
              m_readCommandBuffer(VK_NULL_HANDLE),
              m_writeCommandBuffer(VK_NULL_HANDLE) {}

        bool setUpImage() {
            VkImageCreateInfo imageCi{};
            imageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCi.imageType = VK_IMAGE_TYPE_2D;
            imageCi.extent.width = m_width;
            imageCi.extent.height = m_height;
            imageCi.extent.depth = 1;
            imageCi.mipLevels = 1;
            imageCi.arrayLayers = 1;
            imageCi.format = k_vkFormat;
            imageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCi.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            imageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCi.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCi.flags = 0;
            if (m_vk.vkCreateImage(m_vkDevice, &imageCi, nullptr, &m_vkImage) !=
                VK_SUCCESS) {
                m_vkImage = VK_NULL_HANDLE;
                return false;
            }

            VkMemoryRequirements memRequirements;
            m_vk.vkGetImageMemoryRequirements(m_vkDevice, m_vkImage,
                                              &memRequirements);
            auto maybeMemoryTypeIndex = findMemoryType(
                m_vk, m_vkPhysicalDevice, memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (!maybeMemoryTypeIndex.has_value()) {
                return false;
            }
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = maybeMemoryTypeIndex.value();
            if (m_vk.vkAllocateMemory(m_vkDevice, &allocInfo, nullptr,
                                      &m_imageVkDeviceMemory) != VK_SUCCESS) {
                m_imageVkDeviceMemory = VK_NULL_HANDLE;
                return false;
            }
            if (m_vk.vkBindImageMemory(m_vkDevice, m_vkImage,
                                       m_imageVkDeviceMemory,
                                       0) != VK_SUCCESS) {
                return false;
            }

            VkCommandBuffer cmdBuff;
            VkCommandBufferAllocateInfo cmdBuffAllocInfo = {};
            cmdBuffAllocInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBuffAllocInfo.commandPool = m_vkCommandPool;
            cmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBuffAllocInfo.commandBufferCount = 1;
            if (m_vk.vkAllocateCommandBuffers(m_vkDevice, &cmdBuffAllocInfo,
                                              &cmdBuff) != VK_SUCCESS) {
                return false;
            }
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            bool res =
                m_vk.vkBeginCommandBuffer(cmdBuff, &beginInfo) == VK_SUCCESS;
            if (res) {
                recordImageLayoutTransformCommands(
                    cmdBuff, VK_IMAGE_LAYOUT_UNDEFINED, k_vkImageLayout);
                res = m_vk.vkEndCommandBuffer(cmdBuff) == VK_SUCCESS;
            }
            if (res) {
                res = submitCommandBufferAndWait(cmdBuff);
            }
            m_vk.vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &cmdBuff);
            if (!res) {
                return false;
            }
            VkImageViewCreateInfo imageViewCi = {};
            imageViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCi.image = m_vkImage;
            imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCi.format = k_vkFormat;
            imageViewCi.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCi.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCi.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCi.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCi.subresourceRange.baseMipLevel = 0;
            imageViewCi.subresourceRange.levelCount = 1;
            imageViewCi.subresourceRange.baseArrayLayer = 0;
            imageViewCi.subresourceRange.layerCount = 1;
            if (m_vk.vkCreateImageView(m_vkDevice, &imageViewCi, nullptr,
                                       &m_vkImageView) != VK_SUCCESS) {
                return false;
            }
            return true;
        }

        void recordImageLayoutTransformCommands(VkCommandBuffer cmdBuff,
                                                VkImageLayout oldLayout,
                                                VkImageLayout newLayout) {
            VkImageMemoryBarrier imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarrier.oldLayout = oldLayout;
            imageBarrier.newLayout = newLayout;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.image = m_vkImage;
            imageBarrier.subresourceRange.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange.baseMipLevel = 0;
            imageBarrier.subresourceRange.levelCount = 1;
            imageBarrier.subresourceRange.baseArrayLayer = 0;
            imageBarrier.subresourceRange.layerCount = 1;
            m_vk.vkCmdPipelineBarrier(cmdBuff,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
                                      nullptr, 0, nullptr, 1, &imageBarrier);
        }

        bool submitCommandBufferAndWait(VkCommandBuffer cmdBuff) const {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuff;
            if (m_vk.vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE) !=
                VK_SUCCESS) {
                return false;
            }
            if (m_vk.vkQueueWaitIdle(m_vkQueue) != VK_SUCCESS) {
                return false;
            }
            return true;
        }

        bool setUpBuffer() {
            VkBufferCreateInfo bufferCi = {};
            bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCi.size = m_width * m_height * k_bpp;
            bufferCi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (m_vk.vkCreateBuffer(m_vkDevice, &bufferCi, nullptr,
                                    &m_vkBuffer) != VK_SUCCESS) {
                m_vkBuffer = VK_NULL_HANDLE;
                return false;
            }

            VkMemoryRequirements memRequirements;
            m_vk.vkGetBufferMemoryRequirements(m_vkDevice, m_vkBuffer,
                                               &memRequirements);
            auto maybeMemoryTypeIndex = findMemoryType(
                m_vk, m_vkPhysicalDevice, memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            if (!maybeMemoryTypeIndex.has_value()) {
                return false;
            }
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = maybeMemoryTypeIndex.value();
            if (m_vk.vkAllocateMemory(m_vkDevice, &allocInfo, nullptr,
                                      &m_bufferVkDeviceMemory) != VK_SUCCESS) {
                m_bufferVkDeviceMemory = VK_NULL_HANDLE;
                return false;
            }
            if (m_vk.vkBindBufferMemory(m_vkDevice, m_vkBuffer,
                                        m_bufferVkDeviceMemory,
                                        0) != VK_SUCCESS) {
                return false;
            }
            if (m_vk.vkMapMemory(
                    m_vkDevice, m_bufferVkDeviceMemory, 0, bufferCi.size, 0,
                    reinterpret_cast<void **>(&m_memory)) != VK_SUCCESS) {
                m_memory = nullptr;
                return false;
            }
            return true;
        }

        bool setUpCommandBuffer() {
            std::array<VkCommandBuffer, 2> cmdBuffs;
            VkCommandBufferAllocateInfo cmdBuffAllocInfo = {};
            cmdBuffAllocInfo.sType =
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBuffAllocInfo.commandPool = m_vkCommandPool;
            cmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBuffAllocInfo.commandBufferCount =
                static_cast<uint32_t>(cmdBuffs.size());
            if (m_vk.vkAllocateCommandBuffers(m_vkDevice, &cmdBuffAllocInfo,
                                              cmdBuffs.data()) != VK_SUCCESS) {
                return false;
            }
            m_readCommandBuffer = cmdBuffs[0];
            m_writeCommandBuffer = cmdBuffs[1];

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            if (m_vk.vkBeginCommandBuffer(m_readCommandBuffer, &beginInfo) !=
                VK_SUCCESS) {
                return false;
            }
            recordImageLayoutTransformCommands(
                m_readCommandBuffer, k_vkImageLayout,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {m_width, m_height, 1};
            m_vk.vkCmdCopyImageToBuffer(m_readCommandBuffer, m_vkImage,
                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                        m_vkBuffer, 1, &region);
            recordImageLayoutTransformCommands(
                m_readCommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                k_vkImageLayout);
            if (m_vk.vkEndCommandBuffer(m_readCommandBuffer) != VK_SUCCESS) {
                return false;
            }

            if (m_vk.vkBeginCommandBuffer(m_writeCommandBuffer, &beginInfo) !=
                VK_SUCCESS) {
                return false;
            }
            recordImageLayoutTransformCommands(
                m_writeCommandBuffer, k_vkImageLayout,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            m_vk.vkCmdCopyBufferToImage(
                m_writeCommandBuffer, m_vkBuffer, m_vkImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            recordImageLayoutTransformCommands(
                m_writeCommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                k_vkImageLayout);
            if (m_vk.vkEndCommandBuffer(m_writeCommandBuffer) != VK_SUCCESS) {
                return false;
            }
            return true;
        }
    };

    static void SetUpTestCase() { k_vk = emugl::vkDispatch(false); }

    static constexpr uint32_t k_numOfRenderTargets = 10;
    static constexpr uint32_t k_renderTargetWidth = 255;
    static constexpr uint32_t k_renderTargetHeight = 255;
    static constexpr uint32_t k_renderTargetNumOfPixels =
        k_renderTargetWidth * k_renderTargetHeight;

    void SetUp() override {
#ifndef __linux__
        // TODO(b/179477624): Currently we only ensure the test works
        // on Linux. SwiftShader on Windows and macOS needs to be
        // updated before these tests can be run there.
        m_earlySkipped = true;
        GTEST_SKIP_("System not supported. Test skipped.");
#endif

        ASSERT_NE(k_vk, nullptr);
        createInstance();
        pickPhysicalDevice();
        createLogicalDevice();

        VkCommandPoolCreateInfo commandPoolCi = {};
        commandPoolCi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCi.queueFamilyIndex = m_compositorQueueFamilyIndex;
        ASSERT_EQ(k_vk->vkCreateCommandPool(m_vkDevice, &commandPoolCi, nullptr,
                                            &m_vkCommandPool),
                  VK_SUCCESS);
        k_vk->vkGetDeviceQueue(m_vkDevice, m_compositorQueueFamilyIndex, 0,
                               &m_compositorVkQueue);
        ASSERT_TRUE(m_compositorVkQueue != VK_NULL_HANDLE);

        for (uint32_t i = 0; i < k_numOfRenderTargets; i++) {
            auto renderTarget = RenderTarget::create(
                *k_vk, m_vkDevice, m_vkPhysicalDevice, m_compositorVkQueue,
                m_vkCommandPool, k_renderTargetHeight, k_renderTargetWidth);
            ASSERT_NE(renderTarget, nullptr);
            m_renderTargets.emplace_back(std::move(renderTarget));
        }

        m_renderTargetImageViews.resize(m_renderTargets.size());
        ASSERT_EQ(
            std::transform(
                m_renderTargets.begin(), m_renderTargets.end(),
                m_renderTargetImageViews.begin(),
                [](const std::unique_ptr<const RenderTarget> &renderTarget) {
                    return renderTarget->m_vkImageView;
                }),
            m_renderTargetImageViews.end());
    }

    void TearDown() override {
        if (m_earlySkipped) {
            return;
        }

        m_renderTargets.clear();
        k_vk->vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
        k_vk->vkDestroyDevice(m_vkDevice, nullptr);
        m_vkDevice = VK_NULL_HANDLE;
        k_vk->vkDestroyInstance(m_vkInstance, nullptr);
        m_vkInstance = VK_NULL_HANDLE;
    }

    std::unique_ptr<CompositorVk> createCompositor() {
        return CompositorVk::create(*k_vk, m_vkDevice, RenderTarget::k_vkFormat,
                                    RenderTarget::k_vkImageLayout,
                                    RenderTarget::k_vkImageLayout,
                                    k_renderTargetWidth, k_renderTargetHeight,
                                    m_renderTargetImageViews, m_vkCommandPool);
    }

    static const goldfish_vk::VulkanDispatch *k_vk;
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    uint32_t m_compositorQueueFamilyIndex = 0;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    std::vector<std::unique_ptr<const RenderTarget>> m_renderTargets;
    std::vector<VkImageView> m_renderTargetImageViews;
    VkCommandPool m_vkCommandPool = VK_NULL_HANDLE;
    VkQueue m_compositorVkQueue = VK_NULL_HANDLE;
    bool m_earlySkipped = false;

   private:
    void createInstance() {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = "emulator CompositorVk unittest";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;
        VkInstanceCreateInfo instanceCi = {};
        instanceCi.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCi.ppEnabledExtensionNames = nullptr;
        instanceCi.enabledExtensionCount = 0;
        instanceCi.pApplicationInfo = &appInfo;
        ASSERT_EQ(k_vk->vkCreateInstance(&instanceCi, nullptr, &m_vkInstance),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkInstance != VK_NULL_HANDLE);
    }

    void pickPhysicalDevice() {
        uint32_t physicalDeviceCount = 0;
        ASSERT_EQ(k_vk->vkEnumeratePhysicalDevices(
                      m_vkInstance, &physicalDeviceCount, nullptr),
                  VK_SUCCESS);
        ASSERT_GT(physicalDeviceCount, 0);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        ASSERT_EQ(
            k_vk->vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount,
                                             physicalDevices.data()),
            VK_SUCCESS);
        for (const auto &device : physicalDevices) {
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures =
                {};
            descIndexingFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            features.pNext = &descIndexingFeatures;
            k_vk->vkGetPhysicalDeviceFeatures2(device, &features);
            if (!CompositorVk::validatePhysicalDeviceFeatures(features)) {
                continue;
            }
            uint32_t queueFamilyCount = 0;
            k_vk->vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, nullptr);
            ASSERT_GT(queueFamilyCount, 0);
            std::vector<VkQueueFamilyProperties> queueFamilyProperties(
                queueFamilyCount);
            k_vk->vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, queueFamilyProperties.data());
            uint32_t queueFamilyIndex = 0;
            for (; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
                if (CompositorVk::validateQueueFamilyProperties(
                        queueFamilyProperties[queueFamilyIndex])) {
                    break;
                }
            }
            if (queueFamilyIndex == queueFamilyCount) {
                continue;
            }

            m_compositorQueueFamilyIndex = queueFamilyIndex;
            m_vkPhysicalDevice = device;
            return;
        }
        FAIL() << "Can't find a suitable VkPhysicalDevice.";
    }

    void createLogicalDevice() {
        const float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCi = {};
        queueCi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCi.queueFamilyIndex = m_compositorQueueFamilyIndex;
        queueCi.queueCount = 1;
        queueCi.pQueuePriorities = &queuePriority;
        VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {};
        descIndexingFeatures.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        VkPhysicalDeviceFeatures2 features = {};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features.pNext = &descIndexingFeatures;
        ASSERT_TRUE(CompositorVk::enablePhysicalDeviceFeatures(features));
        const std::vector<const char *> enabledDeviceExtensions =
            CompositorVk::getRequiredDeviceExtensions();
        VkDeviceCreateInfo deviceCi = {};
        deviceCi.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCi.pQueueCreateInfos = &queueCi;
        deviceCi.queueCreateInfoCount = 1;
        deviceCi.pEnabledFeatures = nullptr;
        deviceCi.enabledLayerCount = 0;
        deviceCi.enabledExtensionCount =
            static_cast<uint32_t>(enabledDeviceExtensions.size());
        deviceCi.ppEnabledExtensionNames = enabledDeviceExtensions.data();
        deviceCi.pNext = &features;
        ASSERT_EQ(k_vk->vkCreateDevice(m_vkPhysicalDevice, &deviceCi, nullptr,
                                       &m_vkDevice),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkDevice != VK_NULL_HANDLE);
    }
};

const goldfish_vk::VulkanDispatch *CompositorVkTest::k_vk = nullptr;

TEST_F(CompositorVkTest, Init) {
    ASSERT_NE(createCompositor(), nullptr);
}

TEST_F(CompositorVkTest, ValidatePhysicalDeviceFeatures) {
    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    ASSERT_FALSE(CompositorVk::validatePhysicalDeviceFeatures(features));
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {};
    descIndexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    features.pNext = &descIndexingFeatures;
    ASSERT_FALSE(CompositorVk::validatePhysicalDeviceFeatures(features));
    descIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    ASSERT_TRUE(CompositorVk::validatePhysicalDeviceFeatures(features));
}

TEST_F(CompositorVkTest, ValidateQueueFamilyProperties) {
    VkQueueFamilyProperties properties = {};
    properties.queueFlags &= ~VK_QUEUE_GRAPHICS_BIT;
    ASSERT_FALSE(CompositorVk::validateQueueFamilyProperties(properties));
    properties.queueFlags |= VK_QUEUE_GRAPHICS_BIT;
    ASSERT_TRUE(CompositorVk::validateQueueFamilyProperties(properties));
}

TEST_F(CompositorVkTest, EnablePhysicalDeviceFeatures) {
    VkPhysicalDeviceFeatures2 features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    ASSERT_FALSE(CompositorVk::enablePhysicalDeviceFeatures(features));
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {};
    descIndexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    features.pNext = &descIndexingFeatures;
    ASSERT_TRUE(CompositorVk::enablePhysicalDeviceFeatures(features));
    ASSERT_EQ(descIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind,
              VK_TRUE);
}

TEST_F(CompositorVkTest, EmptyCompositionShouldDrawABlackFrame) {
    std::vector<uint32_t> pixels(k_renderTargetNumOfPixels);
    for (uint32_t i = 0; i < k_renderTargetNumOfPixels; i++) {
        uint8_t v = static_cast<uint8_t>((i / 4) & 0xff);
        uint8_t *pixel = reinterpret_cast<uint8_t *>(&pixels[i]);
        pixel[0] = v;
        pixel[1] = v;
        pixel[2] = v;
        pixel[3] = 0xff;
    }
    for (uint32_t i = 0; i < k_numOfRenderTargets; i++) {
        ASSERT_TRUE(m_renderTargets[i]->write(pixels));
        auto maybeImageBytes = m_renderTargets[i]->read();
        ASSERT_TRUE(maybeImageBytes.has_value());
        for (uint32_t i = 0; i < k_renderTargetNumOfPixels; i++) {
            ASSERT_EQ(pixels[i], maybeImageBytes.value()[i]);
        }
    }

    auto compositor = createCompositor();
    ASSERT_NE(compositor, nullptr);

    // render to render targets with event index
    std::vector<VkCommandBuffer> cmdBuffs = {};
    for (uint32_t i = 0; i < k_numOfRenderTargets; i++) {
        if (i % 2 == 0) {
            cmdBuffs.emplace_back(compositor->getCommandBuffer(i));
        }
    }
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffs.size());
    submitInfo.pCommandBuffers = cmdBuffs.data();
    ASSERT_EQ(k_vk->vkQueueSubmit(m_compositorVkQueue, 1, &submitInfo,
                                  VK_NULL_HANDLE),
              VK_SUCCESS);

    ASSERT_EQ(k_vk->vkQueueWaitIdle(m_compositorVkQueue), VK_SUCCESS);
    for (uint32_t i = 0; i < k_numOfRenderTargets; i++) {
        auto maybeImagePixels = m_renderTargets[i]->read();
        ASSERT_TRUE(maybeImagePixels.has_value());
        auto imagePixels = maybeImagePixels.value();
        for (uint32_t j = 0; j < k_renderTargetNumOfPixels; j++) {
            const auto pixel =
                reinterpret_cast<const uint8_t *>(&imagePixels[j]);
            // should only render to render targets with even index
            if (i % 2 == 0) {
                ASSERT_EQ(pixel[0], 0);
                ASSERT_EQ(pixel[1], 0);
                ASSERT_EQ(pixel[2], 0);
                ASSERT_EQ(pixel[3], 0xff);
            } else {
                ASSERT_EQ(pixels[j], imagePixels[j]);
            }
        }
    }
}