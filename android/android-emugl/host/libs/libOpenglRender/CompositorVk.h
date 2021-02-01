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
#pragma once

#include <array>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <tuple>
#include <variant>
#include <vector>

#include "vulkan/cereal/common/goldfish_vk_dispatch.h"
#include "vulkan/vk_util.h"

class Composition {
   public:
    VkImageView m_vkImageView;
    VkSampler m_vkSampler;
    uint32_t m_width;
    uint32_t m_height;

    // transform matrix in screen coordinates
    glm::mat3 m_transform;

    Composition(VkImageView, VkSampler, uint32_t width, uint32_t height);
};

struct CompositorVkBase
    : public vk_util::RunSingleTimeCommand<
          CompositorVkBase,
          vk_util::FindMemoryType<
              CompositorVkBase,
              vk_util::RecordImageLayoutTransformCommands<CompositorVkBase>>> {
    const goldfish_vk::VulkanDispatch &m_vk;
    const VkDevice m_vkDevice;
    const VkPhysicalDevice m_vkPhysicalDevice;
    const VkQueue m_vkQueue;
    VkDescriptorSetLayout m_vkDescriptorSetLayout;
    VkPipelineLayout m_vkPipelineLayout;
    VkRenderPass m_vkRenderPass;
    VkPipeline m_graphicsVkPipeline;
    VkBuffer m_vertexVkBuffer;
    VkDeviceMemory m_vertexVkDeviceMemory;
    VkBuffer m_indexVkBuffer;
    VkDeviceMemory m_indexVkDeviceMemory;
    std::vector<VkCommandBuffer> m_vkCommandBuffers;
    VkDescriptorPool m_vkDescriptorPool;
    std::vector<VkDescriptorSet> m_vkDescriptorSets;
    std::vector<VkFramebuffer> m_renderTargetVkFrameBuffers;

    VkCommandPool m_vkCommandPool;

    explicit CompositorVkBase(const goldfish_vk::VulkanDispatch &vk,
                              VkDevice device, VkPhysicalDevice physicalDevice,
                              VkQueue queue, VkCommandPool commandPool)
        : m_vk(vk),
          m_vkDevice(device),
          m_vkPhysicalDevice(physicalDevice),
          m_vkQueue(queue),
          m_vkDescriptorSetLayout(VK_NULL_HANDLE),
          m_vkPipelineLayout(VK_NULL_HANDLE),
          m_vkRenderPass(VK_NULL_HANDLE),
          m_graphicsVkPipeline(VK_NULL_HANDLE),
          m_vertexVkBuffer(VK_NULL_HANDLE),
          m_vertexVkDeviceMemory(VK_NULL_HANDLE),
          m_indexVkBuffer(VK_NULL_HANDLE),
          m_indexVkDeviceMemory(VK_NULL_HANDLE),
          m_vkCommandBuffers(0),
          m_vkDescriptorPool(VK_NULL_HANDLE),
          m_vkDescriptorSets(0),
          m_renderTargetVkFrameBuffers(0),
          m_vkCommandPool(commandPool) {}
};

class CompositorVk : protected CompositorVkBase {
   public:
    static std::unique_ptr<CompositorVk> create(
        const goldfish_vk::VulkanDispatch &vk, VkDevice, VkPhysicalDevice,
        VkQueue, VkFormat, VkImageLayout initialLayout,
        VkImageLayout finalLayout, uint32_t width, uint32_t height,
        const std::vector<VkImageView> &renderTargets, VkCommandPool);
    static bool validatePhysicalDeviceFeatures(
        const VkPhysicalDeviceFeatures2 &features);
    static bool validateQueueFamilyProperties(
        const VkQueueFamilyProperties &properties);
    static bool enablePhysicalDeviceFeatures(
        VkPhysicalDeviceFeatures2 &features);
    static std::vector<const char *> getRequiredDeviceExtensions();

    ~CompositorVk();
    VkCommandBuffer getCommandBuffer(uint32_t i) const;
    void setComposition(uint32_t i, std::unique_ptr<Composition> &&composition);

   private:
    explicit CompositorVk(const goldfish_vk::VulkanDispatch &, VkDevice,
                          VkPhysicalDevice, VkQueue, VkCommandPool,
                          uint32_t renderTargetWidth,
                          uint32_t renderTargetHeight);
    void setUpGraphicsPipeline(uint32_t width, uint32_t height,
                               VkFormat renderTargetFormat,
                               VkImageLayout initialLayout,
                               VkImageLayout finalLayout);
    void setUpVertexBuffers();
    void setUpFramebuffers(const std::vector<VkImageView> &, uint32_t width,
                           uint32_t height);
    void setUpDescriptorSets();
    void setUpCommandBuffers(uint32_t width, uint32_t height);
    void setUpEmptyComposition(VkFormat);
    void setUpUniformBuffers();

    std::optional<std::tuple<VkBuffer, VkDeviceMemory>> createBuffer(
        VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags) const;
    std::tuple<VkBuffer, VkDeviceMemory> createStagingBufferWithData(
        const void *data, VkDeviceSize size) const;
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize) const;

    struct UniformBufferObject {
        alignas(16) glm::mat4 transform;
    };

    struct Vertex {
        alignas(8) glm::vec2 pos;
        alignas(8) glm::vec2 texPos;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2>
        getAttributeDescription();
    };

    static const std::vector<Vertex> k_vertices;
    static const std::vector<uint16_t> k_indices;
    static const VkExtent2D k_emptyCompositionExtent;

    uint32_t m_renderTargetWidth;
    uint32_t m_renderTargetHeight;
    VkImage m_emptyCompositionVkImage;
    VkDeviceMemory m_emptyCompositionVkDeviceMemory;
    VkImageView m_emptyCompositionVkImageView;
    VkSampler m_emptyCompositionVkSampler;

    std::vector<std::unique_ptr<Composition>> m_currentCompositions;
    struct UniformStorage {
        VkBuffer m_vkBuffer;
        VkDeviceMemory m_vkDeviceMemory;
        void *m_data;
        VkDeviceSize m_stride;
    } m_uniformStorage;
};
