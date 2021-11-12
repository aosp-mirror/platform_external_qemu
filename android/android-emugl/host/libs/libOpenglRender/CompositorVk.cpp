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

#include <glm/gtx/matrix_transform_2d.hpp>
#include <cstring>
#include <optional>

#include "vulkan/vk_util.h"

namespace CompositorVkShader {
#include "vulkan/CompositorFragmentShader.h"
#include "vulkan/CompositorVertexShader.h"
}  // namespace CompositorVkShader

static VkShaderModule createShaderModule(const goldfish_vk::VulkanDispatch &vk,
                                         VkDevice device,
                                         const std::vector<uint32_t> &code) {
    VkShaderModuleCreateInfo shaderModuleCi = {};
    shaderModuleCi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCi.codeSize =
        static_cast<uint32_t>(code.size() * sizeof(uint32_t));
    shaderModuleCi.pCode = code.data();
    VkShaderModule res;
    VK_CHECK(vk.vkCreateShaderModule(device, &shaderModuleCi, nullptr, &res));
    return res;
}

Composition::Composition(VkImageView vkImageView, VkSampler vkSampler,
                         uint32_t width, uint32_t height)
    : m_vkImageView(vkImageView),
      m_vkSampler(vkSampler),
      m_width(width),
      m_height(height),
      m_transform(1.0f) {}

const std::vector<CompositorVk::Vertex> CompositorVk::k_vertices = {
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f}, {0.0f, 1.0f}},
};

const std::vector<uint16_t> CompositorVk::k_indices = {0, 1, 2, 2, 3, 0};

const VkExtent2D CompositorVk::k_emptyCompositionExtent = {1, 1};

std::unique_ptr<CompositorVk> CompositorVk::create(
    const goldfish_vk::VulkanDispatch &vk, VkDevice vkDevice,
    VkPhysicalDevice vkPhysicalDevice, VkQueue vkQueue, VkFormat format,
    VkImageLayout initialLayout, VkImageLayout finalLayout, uint32_t width,
    uint32_t height, const std::vector<VkImageView> &renderTargets,
    VkCommandPool commandPool) {
    auto res = std::unique_ptr<CompositorVk>(new CompositorVk(
        vk, vkDevice, vkPhysicalDevice, vkQueue, commandPool, width, height));
    res->setUpGraphicsPipeline(width, height, format, initialLayout,
                               finalLayout);
    res->setUpVertexBuffers();
    res->setUpFramebuffers(renderTargets, width, height);
    res->setUpUniformBuffers();
    res->setUpDescriptorSets();
    res->setUpCommandBuffers(width, height);
    res->setUpEmptyComposition(format);
    res->m_currentCompositions.resize(renderTargets.size());
    for (auto i = 0; i < renderTargets.size(); i++) {
        res->setComposition(
            i, std::make_unique<Composition>(res->m_emptyCompositionVkImageView,
                                             res->m_emptyCompositionVkSampler,
                                             k_emptyCompositionExtent.width,
                                             k_emptyCompositionExtent.height));
    }
    return res;
}

CompositorVk::CompositorVk(const goldfish_vk::VulkanDispatch &vk,
                           VkDevice vkDevice, VkPhysicalDevice vkPhysicalDevice,
                           VkQueue vkQueue, VkCommandPool vkCommandPool,
                           uint32_t renderTargetWidth,
                           uint32_t renderTargetHeight)
    : CompositorVkBase(vk, vkDevice, vkPhysicalDevice, vkQueue, vkCommandPool),
      m_renderTargetWidth(renderTargetWidth),
      m_renderTargetHeight(renderTargetHeight),
      m_emptyCompositionVkImage(VK_NULL_HANDLE),
      m_emptyCompositionVkDeviceMemory(VK_NULL_HANDLE),
      m_emptyCompositionVkImageView(VK_NULL_HANDLE),
      m_emptyCompositionVkSampler(VK_NULL_HANDLE),
      m_currentCompositions(0),
      m_uniformStorage({VK_NULL_HANDLE, VK_NULL_HANDLE, nullptr, 0}) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    m_vk.vkGetPhysicalDeviceProperties(m_vkPhysicalDevice,
                                       &physicalDeviceProperties);
    auto alignment =
        physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    m_uniformStorage.m_stride =
        ((sizeof(UniformBufferObject) - 1) / alignment + 1) * alignment;
}

      CompositorVk::~CompositorVk() {
          m_vk.vkDestroySampler(m_vkDevice, m_emptyCompositionVkSampler,
                                nullptr);
          m_vk.vkDestroyImageView(m_vkDevice, m_emptyCompositionVkImageView,
                                  nullptr);
          m_vk.vkFreeMemory(m_vkDevice, m_emptyCompositionVkDeviceMemory,
                            nullptr);
          m_vk.vkDestroyImage(m_vkDevice, m_emptyCompositionVkImage, nullptr);
          m_vk.vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
          m_vk.vkFreeCommandBuffers(
              m_vkDevice, m_vkCommandPool,
              static_cast<uint32_t>(m_vkCommandBuffers.size()),
              m_vkCommandBuffers.data());
          if (m_uniformStorage.m_vkDeviceMemory != VK_NULL_HANDLE) {
              m_vk.vkUnmapMemory(m_vkDevice, m_uniformStorage.m_vkDeviceMemory);
          }
          m_vk.vkDestroyBuffer(m_vkDevice, m_uniformStorage.m_vkBuffer,
                               nullptr);
          m_vk.vkFreeMemory(m_vkDevice, m_uniformStorage.m_vkDeviceMemory,
                            nullptr);
          while (!m_renderTargetVkFrameBuffers.empty()) {
              m_vk.vkDestroyFramebuffer(
                  m_vkDevice, m_renderTargetVkFrameBuffers.back(), nullptr);
              m_renderTargetVkFrameBuffers.pop_back();
          }
          m_vk.vkFreeMemory(m_vkDevice, m_vertexVkDeviceMemory, nullptr);
          m_vk.vkDestroyBuffer(m_vkDevice, m_vertexVkBuffer, nullptr);
          m_vk.vkFreeMemory(m_vkDevice, m_indexVkDeviceMemory, nullptr);
          m_vk.vkDestroyBuffer(m_vkDevice, m_indexVkBuffer, nullptr);
          m_vk.vkDestroyPipeline(m_vkDevice, m_graphicsVkPipeline, nullptr);
          m_vk.vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);
          m_vk.vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
          m_vk.vkDestroyDescriptorSetLayout(m_vkDevice, m_vkDescriptorSetLayout,
                                            nullptr);
      }

void CompositorVk::setUpGraphicsPipeline(uint32_t width, uint32_t height,
                                         VkFormat renderTargetFormat,
                                         VkImageLayout initialLayout,
                                         VkImageLayout finalLayout) {
    const std::vector<uint32_t> vertSpvBuff(
        CompositorVkShader::compositorVertexShader,
        std::end(CompositorVkShader::compositorVertexShader));
    const std::vector<uint32_t> fragSpvBuff(
        CompositorVkShader::compositorFragmentShader,
        std::end(CompositorVkShader::compositorFragmentShader));
    const auto vertShaderMod =
        createShaderModule(m_vk, m_vkDevice, vertSpvBuff);
    const auto fragShaderMod =
        createShaderModule(m_vk, m_vkDevice, fragSpvBuff);

    VkPipelineShaderStageCreateInfo shaderStageCis[2] = {};
    shaderStageCis[0].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCis[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCis[0].module = vertShaderMod;
    shaderStageCis[0].pName = "main";
    shaderStageCis[1].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCis[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCis[1].module = fragShaderMod;
    shaderStageCis[1].pName = "main";

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescription();
    VkPipelineVertexInputStateCreateInfo vertexInputStateCi = {};
    vertexInputStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCi.vertexBindingDescriptionCount = 1;
    vertexInputStateCi.pVertexBindingDescriptions = &bindingDescription;
    vertexInputStateCi.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescription.size());
    vertexInputStateCi.pVertexAttributeDescriptions =
        attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCi = {};
    inputAssemblyStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCi.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = width;
    scissor.extent.height = height;

    VkPipelineViewportStateCreateInfo viewportStateCi = {};
    viewportStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCi.viewportCount = 1;
    viewportStateCi.pViewports = &viewport;
    viewportStateCi.scissorCount = 1;
    viewportStateCi.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerStateCi = {};
    rasterizerStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerStateCi.depthClampEnable = VK_FALSE;
    rasterizerStateCi.rasterizerDiscardEnable = VK_FALSE;
    rasterizerStateCi.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerStateCi.lineWidth = 1.0f;
    rasterizerStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerStateCi.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerStateCi.depthBiasEnable = VK_FALSE;
    rasterizerStateCi.depthBiasConstantFactor = 0.0f;
    rasterizerStateCi.depthBiasClamp = 0.0f;
    rasterizerStateCi.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampleStateCi = {};
    multisampleStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCi.sampleShadingEnable = VK_FALSE;
    multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCi.minSampleShading = 1.0f;
    multisampleStateCi.pSampleMask = nullptr;
    multisampleStateCi.alphaToCoverageEnable = VK_FALSE;
    multisampleStateCi.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MAX;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCi = {};
    colorBlendStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCi.logicOpEnable = VK_FALSE;
    colorBlendStateCi.attachmentCount = 1;
    colorBlendStateCi.pAttachments = &colorBlendAttachment;

    VkDescriptorSetLayoutBinding layoutBindings[2] = {};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCi = {};
    descriptorSetLayoutCi.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCi.bindingCount =
        static_cast<uint32_t>(std::size(layoutBindings));
    descriptorSetLayoutCi.pBindings = layoutBindings;
    descriptorSetLayoutCi.pNext = nullptr;
    descriptorSetLayoutCi.flags = {};
    VK_CHECK(m_vk.vkCreateDescriptorSetLayout(
        m_vkDevice, &descriptorSetLayoutCi, nullptr, &m_vkDescriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCi = {};
    pipelineLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCi.setLayoutCount = 1;
    pipelineLayoutCi.pSetLayouts = &m_vkDescriptorSetLayout;
    pipelineLayoutCi.pushConstantRangeCount = 0;

    VK_CHECK(m_vk.vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutCi, nullptr,
                                         &m_vkPipelineLayout));

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = renderTargetFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = initialLayout;
    colorAttachment.finalLayout = finalLayout;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // TODO: to support multiple layer composition, we could run the same render
    // pass for multiple time. In that case, we should use explicit
    // VkImageMemoryBarriers to transform the image layout instead of relying on
    // renderpass to do it.
    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCi = {};
    renderPassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCi.attachmentCount = 1;
    renderPassCi.pAttachments = &colorAttachment;
    renderPassCi.subpassCount = 1;
    renderPassCi.pSubpasses = &subpass;
    renderPassCi.dependencyCount = 1;
    renderPassCi.pDependencies = &subpassDependency;

    VK_CHECK(m_vk.vkCreateRenderPass(m_vkDevice, &renderPassCi, nullptr,
                                     &m_vkRenderPass));

    VkGraphicsPipelineCreateInfo graphicsPipelineCi = {};
    graphicsPipelineCi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCi.stageCount =
        static_cast<uint32_t>(std::size(shaderStageCis));
    graphicsPipelineCi.pStages = shaderStageCis;
    graphicsPipelineCi.pVertexInputState = &vertexInputStateCi;
    graphicsPipelineCi.pInputAssemblyState = &inputAssemblyStateCi;
    graphicsPipelineCi.pViewportState = &viewportStateCi;
    graphicsPipelineCi.pRasterizationState = &rasterizerStateCi;
    graphicsPipelineCi.pMultisampleState = &multisampleStateCi;
    graphicsPipelineCi.pDepthStencilState = nullptr;
    graphicsPipelineCi.pColorBlendState = &colorBlendStateCi;
    graphicsPipelineCi.pDynamicState = nullptr;
    graphicsPipelineCi.layout = m_vkPipelineLayout;
    graphicsPipelineCi.renderPass = m_vkRenderPass;
    graphicsPipelineCi.subpass = 0;
    graphicsPipelineCi.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCi.basePipelineIndex = -1;

    VK_CHECK(m_vk.vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1,
                                            &graphicsPipelineCi, nullptr,
                                            &m_graphicsVkPipeline));

    m_vk.vkDestroyShaderModule(m_vkDevice, vertShaderMod, nullptr);
    m_vk.vkDestroyShaderModule(m_vkDevice, fragShaderMod, nullptr);
}

// Create a VkBuffer and a bound VkDeviceMemory. When the specified memory type
// can't be found, return std::nullopt. When Vulkan call fails, terminate the
// program.
std::optional<std::tuple<VkBuffer, VkDeviceMemory>> CompositorVk::createBuffer(
    VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memProperty) const {
    VkBufferCreateInfo bufferCi = {};
    bufferCi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCi.size = size;
    bufferCi.usage = usage;
    bufferCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer resBuffer;
    VK_CHECK(m_vk.vkCreateBuffer(m_vkDevice, &bufferCi, nullptr, &resBuffer));
    VkMemoryRequirements memRequirements;
    m_vk.vkGetBufferMemoryRequirements(m_vkDevice, resBuffer, &memRequirements);
    VkPhysicalDeviceMemoryProperties physicalMemProperties;
    m_vk.vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice,
                                             &physicalMemProperties);
    auto maybeMemoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits, memProperty);
    if (!maybeMemoryTypeIndex.has_value()) {
        m_vk.vkDestroyBuffer(m_vkDevice, resBuffer, nullptr);
        return std::nullopt;
    }
    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memRequirements.size;
    memAllocInfo.memoryTypeIndex = maybeMemoryTypeIndex.value();
    VkDeviceMemory resMemory;
    VK_CHECK(
        m_vk.vkAllocateMemory(m_vkDevice, &memAllocInfo, nullptr, &resMemory));
    VK_CHECK(m_vk.vkBindBufferMemory(m_vkDevice, resBuffer, resMemory, 0));
    return std::make_tuple(resBuffer, resMemory);
}

std::tuple<VkBuffer, VkDeviceMemory> CompositorVk::createStagingBufferWithData(
    const void *srcData, VkDeviceSize size) const {
    auto [stagingBuffer, stagingBufferMemory] =
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            .value();
    void *data;
    VK_CHECK(
        m_vk.vkMapMemory(m_vkDevice, stagingBufferMemory, 0, size, 0, &data));
    memcpy(data, srcData, size);
    m_vk.vkUnmapMemory(m_vkDevice, stagingBufferMemory);
    return std::make_tuple(stagingBuffer, stagingBufferMemory);
}

void CompositorVk::copyBuffer(VkBuffer src, VkBuffer dst,
                              VkDeviceSize size) const {
    runSingleTimeCommands(m_vkQueue, [&, this](const auto &cmdBuff) {
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        m_vk.vkCmdCopyBuffer(cmdBuff, src, dst, 1, &copyRegion);
    });
}

void CompositorVk::setUpVertexBuffers() {
    const VkDeviceSize vertexBufferSize =
        sizeof(k_vertices[0]) * k_vertices.size();
    std::tie(m_vertexVkBuffer, m_vertexVkDeviceMemory) =
        createBuffer(vertexBufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            .value();
    auto [vertexStagingBuffer, vertexStagingBufferMemory] =
        createStagingBufferWithData(k_vertices.data(), vertexBufferSize);
    copyBuffer(vertexStagingBuffer, m_vertexVkBuffer, vertexBufferSize);
    m_vk.vkDestroyBuffer(m_vkDevice, vertexStagingBuffer, nullptr);
    m_vk.vkFreeMemory(m_vkDevice, vertexStagingBufferMemory, nullptr);

    VkDeviceSize indexBufferSize = sizeof(k_indices[0]) * k_indices.size();
    auto [indexStagingBuffer, indexStagingBufferMemory] =
        createStagingBufferWithData(k_indices.data(), indexBufferSize);
    std::tie(m_indexVkBuffer, m_indexVkDeviceMemory) =
        createBuffer(
            indexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            .value();
    copyBuffer(indexStagingBuffer, m_indexVkBuffer, indexBufferSize);
    m_vk.vkDestroyBuffer(m_vkDevice, indexStagingBuffer, nullptr);
    m_vk.vkFreeMemory(m_vkDevice, indexStagingBufferMemory, nullptr);
}

void CompositorVk::setUpFramebuffers(
    const std::vector<VkImageView> &renderTargets, uint32_t width,
    uint32_t height) {
    for (size_t i = 0; i < renderTargets.size(); i++) {
        VkFramebufferCreateInfo fbCi = {};
        fbCi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCi.renderPass = m_vkRenderPass;
        fbCi.attachmentCount = 1;
        fbCi.pAttachments = &renderTargets[i];
        fbCi.width = width;
        fbCi.height = height;
        fbCi.layers = 1;
        VkFramebuffer framebuffer;
        VK_CHECK(
            m_vk.vkCreateFramebuffer(m_vkDevice, &fbCi, nullptr, &framebuffer));
        m_renderTargetVkFrameBuffers.push_back(framebuffer);
    }
}

void CompositorVk::setUpDescriptorSets() {
    uint32_t numOfFrames =
        static_cast<uint32_t>(m_renderTargetVkFrameBuffers.size());

    VkDescriptorPoolSize descriptorPoolSizes[2] = {};
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSizes[0].descriptorCount = numOfFrames;
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[1].descriptorCount = numOfFrames;

    VkDescriptorPoolCreateInfo descriptorPoolCi = {};
    descriptorPoolCi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCi.poolSizeCount =
        static_cast<uint32_t>(std::size(descriptorPoolSizes));
    descriptorPoolCi.pPoolSizes = descriptorPoolSizes;
    descriptorPoolCi.maxSets = static_cast<uint32_t>(numOfFrames);
    descriptorPoolCi.flags = {};
    VK_CHECK(m_vk.vkCreateDescriptorPool(m_vkDevice, &descriptorPoolCi, nullptr,
                                         &m_vkDescriptorPool));
    std::vector<VkDescriptorSetLayout> layouts(numOfFrames,
                                               m_vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
    descriptorSetAllocInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = m_vkDescriptorPool;
    descriptorSetAllocInfo.descriptorSetCount = numOfFrames;
    descriptorSetAllocInfo.pSetLayouts = layouts.data();
    m_vkDescriptorSets.resize(numOfFrames);
    VK_CHECK(m_vk.vkAllocateDescriptorSets(m_vkDevice, &descriptorSetAllocInfo,
                                           m_vkDescriptorSets.data()));
    for (size_t i = 0; i < numOfFrames; i++) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_uniformStorage.m_vkBuffer;
        bufferInfo.offset = i * m_uniformStorage.m_stride;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorSetWrite = {};
        descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrite.dstSet = m_vkDescriptorSets[i];
        descriptorSetWrite.dstBinding = 1;
        descriptorSetWrite.dstArrayElement = 0;
        descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetWrite.descriptorCount = 1;
        descriptorSetWrite.pBufferInfo = &bufferInfo;

        m_vk.vkUpdateDescriptorSets(m_vkDevice, 1, &descriptorSetWrite, 0,
                                    nullptr);
    }
}

void CompositorVk::setUpCommandBuffers(uint32_t width, uint32_t height) {
    VkCommandBufferAllocateInfo cmdBuffAllocInfo = {};
    cmdBuffAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBuffAllocInfo.commandPool = m_vkCommandPool;
    cmdBuffAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBuffAllocInfo.commandBufferCount =
        static_cast<uint32_t>(m_renderTargetVkFrameBuffers.size());
    m_vkCommandBuffers.resize(m_renderTargetVkFrameBuffers.size());
    VK_CHECK(m_vk.vkAllocateCommandBuffers(m_vkDevice, &cmdBuffAllocInfo,
                                           m_vkCommandBuffers.data()));

    for (size_t i = 0; i < m_vkCommandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        const auto &cmdBuffer = m_vkCommandBuffers[i];
        VK_CHECK(m_vk.vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        VkClearValue clearColor = {};
        clearColor.color.float32[0] = 0.0f;
        clearColor.color.float32[1] = 0.0f;
        clearColor.color.float32[2] = 0.0f;
        clearColor.color.float32[3] = 1.0f;

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = m_vkRenderPass;
        renderPassBeginInfo.framebuffer = m_renderTargetVkFrameBuffers[i];
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearColor;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = {width, height};

        m_vk.vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo,
                                  VK_SUBPASS_CONTENTS_INLINE);
        m_vk.vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               m_graphicsVkPipeline);
        VkDeviceSize offsets[] = {0};
        m_vk.vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertexVkBuffer,
                                    offsets);
        m_vk.vkCmdBindIndexBuffer(cmdBuffer, m_indexVkBuffer, 0,
                                  VK_INDEX_TYPE_UINT16);
        m_vk.vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                     m_vkPipelineLayout, 0, 1,
                                     &m_vkDescriptorSets[i], 0, nullptr);
        m_vk.vkCmdDrawIndexed(
            cmdBuffer, static_cast<uint32_t>(k_indices.size()), 1, 0, 0, 0);
        m_vk.vkCmdEndRenderPass(cmdBuffer);

        VK_CHECK(m_vk.vkEndCommandBuffer(cmdBuffer));
    }
}

void CompositorVk::setUpEmptyComposition(VkFormat format) {
    VkImageCreateInfo imageCi = {};
    imageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCi.imageType = VK_IMAGE_TYPE_2D;
    imageCi.extent.width = k_emptyCompositionExtent.width;
    imageCi.extent.height = k_emptyCompositionExtent.height;
    imageCi.extent.depth = 1;
    imageCi.mipLevels = 1;
    imageCi.arrayLayers = 1;
    imageCi.format = format;
    imageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCi.usage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCi.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCi.flags = 0;
    VK_CHECK(m_vk.vkCreateImage(m_vkDevice, &imageCi, nullptr,
                                &m_emptyCompositionVkImage));

    VkMemoryRequirements memRequirements;
    m_vk.vkGetImageMemoryRequirements(m_vkDevice, m_emptyCompositionVkImage,
                                      &memRequirements);
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            .value();
    VK_CHECK(m_vk.vkAllocateMemory(m_vkDevice, &allocInfo, nullptr,
                                   &m_emptyCompositionVkDeviceMemory));
    VK_CHECK(m_vk.vkBindImageMemory(m_vkDevice, m_emptyCompositionVkImage,
                                    m_emptyCompositionVkDeviceMemory, 0));
    runSingleTimeCommands(m_vkQueue, [&, this](const auto &cmdBuff) {
        recordImageLayoutTransformCommands(
            cmdBuff, m_emptyCompositionVkImage, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkClearColorValue clearColor = {0.0, 0.0, 0.0, 1.0};
        VkBufferImageCopy region = {};
        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;
        m_vk.vkCmdClearColorImage(cmdBuff, m_emptyCompositionVkImage,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  &clearColor, 1, &range);
        recordImageLayoutTransformCommands(
            cmdBuff, m_emptyCompositionVkImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    VkImageViewCreateInfo imageViewCi = {};
    imageViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCi.image = m_emptyCompositionVkImage;
    imageViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCi.format = format;
    imageViewCi.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCi.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCi.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCi.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCi.subresourceRange.baseMipLevel = 0;
    imageViewCi.subresourceRange.levelCount = 1;
    imageViewCi.subresourceRange.baseArrayLayer = 0;
    imageViewCi.subresourceRange.layerCount = 1;
    VK_CHECK(m_vk.vkCreateImageView(m_vkDevice, &imageViewCi, nullptr,
                                    &m_emptyCompositionVkImageView));

    VkSamplerCreateInfo samplerCi = {};
    samplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCi.magFilter = VK_FILTER_LINEAR;
    samplerCi.minFilter = VK_FILTER_LINEAR;
    samplerCi.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCi.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCi.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCi.anisotropyEnable = VK_FALSE;
    samplerCi.maxAnisotropy = 1.0f;
    samplerCi.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerCi.unnormalizedCoordinates = VK_FALSE;
    samplerCi.compareEnable = VK_FALSE;
    samplerCi.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCi.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCi.mipLodBias = 0.0f;
    samplerCi.minLod = 0.0f;
    samplerCi.maxLod = 0.0f;
    VK_CHECK(m_vk.vkCreateSampler(m_vkDevice, &samplerCi, nullptr,
                                  &m_emptyCompositionVkSampler));
}

void CompositorVk::setUpUniformBuffers() {
    auto numOfFrames = m_renderTargetVkFrameBuffers.size();
    VkDeviceSize size = m_uniformStorage.m_stride * numOfFrames;
    auto maybeBuffer = createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                        VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    auto buffer = std::make_tuple<VkBuffer, VkDeviceMemory>(VK_NULL_HANDLE,
                                                            VK_NULL_HANDLE);
    if (maybeBuffer.has_value()) {
        buffer = maybeBuffer.value();
    } else {
        buffer = createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                     .value();
    }
    std::tie(m_uniformStorage.m_vkBuffer, m_uniformStorage.m_vkDeviceMemory) =
        buffer;
    VK_CHECK(m_vk.vkMapMemory(
        m_vkDevice, m_uniformStorage.m_vkDeviceMemory, 0, size, 0,
        reinterpret_cast<void **>(&m_uniformStorage.m_data)));
}

bool CompositorVk::validateQueueFamilyProperties(
    const VkQueueFamilyProperties &properties) {
    return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

VkCommandBuffer CompositorVk::getCommandBuffer(uint32_t i) const {
    return m_vkCommandBuffers[i];
}

void CompositorVk::setComposition(uint32_t i,
                                  std::unique_ptr<Composition> &&composition) {
    m_currentCompositions[i] = std::move(composition);
    const auto &currentComposition = *m_currentCompositions[i];
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = currentComposition.m_vkImageView;
    imageInfo.sampler = currentComposition.m_vkSampler;
    VkWriteDescriptorSet descriptorSetWrite = {};
    descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWrite.dstSet = m_vkDescriptorSets[i];
    descriptorSetWrite.dstBinding = 0;
    descriptorSetWrite.dstArrayElement = 0;
    descriptorSetWrite.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetWrite.descriptorCount = 1;
    descriptorSetWrite.pImageInfo = &imageInfo;
    m_vk.vkUpdateDescriptorSets(m_vkDevice, 1, &descriptorSetWrite, 0, nullptr);

    auto local2screen = glm::translate(glm::mat3(1.0f), glm::vec2(1.0f, 1.0f));
    local2screen =
        glm::scale(
            glm::mat3(1.0f),
            glm::vec2(static_cast<float>(currentComposition.m_width) / 2.0f,
                      static_cast<float>(currentComposition.m_height) / 2.0f)) *
        local2screen;
    auto screen2ndc =
        glm::scale(glm::mat3(1.0f),
                   glm::vec2(2.0f / static_cast<float>(m_renderTargetWidth),
                             2.0f / static_cast<float>(m_renderTargetHeight)));
    screen2ndc =
        glm::translate(glm::mat3(1.0f), glm::vec2(-1.0f, -1.0f)) * screen2ndc;
    auto t = screen2ndc * currentComposition.m_transform * local2screen;
    UniformBufferObject ubo{};
    ubo.transform = glm::mat4(glm::vec4(t[0][0], t[0][1], 0.0f, t[0][2]),
                              glm::vec4(t[1][0], t[1][1], 0.0f, t[1][2]),
                              glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                              glm::vec4(t[2][0], t[2][1], 0.0f, t[2][2]));
    memcpy(reinterpret_cast<uint8_t *>(m_uniformStorage.m_data) +
               i * m_uniformStorage.m_stride,
           &ubo, sizeof(ubo));
}

VkVertexInputBindingDescription CompositorVk::Vertex::getBindingDescription() {
    VkVertexInputBindingDescription res = {};
    res.binding = 0;
    res.stride = sizeof(struct Vertex);
    res.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return res;
}

std::array<VkVertexInputAttributeDescription, 2>
CompositorVk::Vertex::getAttributeDescription() {
    std::array<VkVertexInputAttributeDescription, 2> res = {};
    res[0].binding = 0;
    res[0].location = 0;
    res[0].format = VK_FORMAT_R32G32_SFLOAT;
    res[0].offset = offsetof(struct Vertex, pos);
    res[1].binding = 0;
    res[1].location = 1;
    res[1].format = VK_FORMAT_R32G32_SFLOAT;
    res[1].offset = offsetof(struct Vertex, texPos);
    return res;
}