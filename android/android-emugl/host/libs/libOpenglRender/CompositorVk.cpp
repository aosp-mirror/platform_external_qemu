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

std::unique_ptr<CompositorVk> CompositorVk::create(
    const goldfish_vk::VulkanDispatch &vk, VkDevice vkDevice, VkFormat format,
    VkImageLayout initialLayout, VkImageLayout finalLayout, uint32_t width,
    uint32_t height, const std::vector<VkImageView> &renderTargets,
    VkCommandPool commandPool) {
    auto res = std::unique_ptr<CompositorVk>(
        new CompositorVk(vk, vkDevice, commandPool));
    res->setUpGraphicsPipeline(width, height, format, initialLayout,
                               finalLayout);
    res->setUpFramebuffers(renderTargets, width, height);
    res->setUpCommandBuffers(width, height);
    return res;
}

CompositorVk::CompositorVk(const goldfish_vk::VulkanDispatch &vk,
                           VkDevice vkDevice, VkCommandPool vkCommandPool)
    : m_vk(vk),
      m_vkDevice(vkDevice),
      m_vkPipelineLayout(VK_NULL_HANDLE),
      m_vkRenderPass(VK_NULL_HANDLE),
      m_graphicsVkPipeline(VK_NULL_HANDLE),
      m_vkCommandBuffers(0),
      m_renderTargetVkFrameBuffers(0),
      m_vkCommandPool(vkCommandPool) {}

CompositorVk::~CompositorVk() {
    m_vk.vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool,
                              static_cast<uint32_t>(m_vkCommandBuffers.size()),
                              m_vkCommandBuffers.data());
    while (!m_renderTargetVkFrameBuffers.empty()) {
        m_vk.vkDestroyFramebuffer(m_vkDevice,
                                  m_renderTargetVkFrameBuffers.back(), nullptr);
        m_renderTargetVkFrameBuffers.pop_back();
    }
    if (m_graphicsVkPipeline != VK_NULL_HANDLE) {
        m_vk.vkDestroyPipeline(m_vkDevice, m_graphicsVkPipeline, nullptr);
    }
    if (m_vkRenderPass != VK_NULL_HANDLE) {
        m_vk.vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);
    }
    if (m_vkPipelineLayout != VK_NULL_HANDLE) {
        m_vk.vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
    }
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

    VkPipelineVertexInputStateCreateInfo vertexInputStateCi = {};
    vertexInputStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCi.vertexBindingDescriptionCount = 0;
    vertexInputStateCi.vertexAttributeDescriptionCount = 0;

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
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCi = {};
    colorBlendStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCi.logicOpEnable = VK_FALSE;
    colorBlendStateCi.attachmentCount = 1;
    colorBlendStateCi.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutCi = {};
    pipelineLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCi.setLayoutCount = 0;
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
        m_vk.vkCmdDraw(cmdBuffer, 0, 1, 0, 0);
        m_vk.vkCmdEndRenderPass(cmdBuffer);

        VK_CHECK(m_vk.vkEndCommandBuffer(cmdBuffer));
    }
}

bool CompositorVk::validatePhysicalDeviceFeatures(
    const VkPhysicalDeviceFeatures2 &features) {
    auto descIndexingFeatures =
        vk_find_struct<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
            &features);
    if (descIndexingFeatures == nullptr) {
        return false;
    }
    return descIndexingFeatures->descriptorBindingSampledImageUpdateAfterBind ==
           VK_TRUE;
}

bool CompositorVk::validateQueueFamilyProperties(
    const VkQueueFamilyProperties &properties) {
    return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

bool CompositorVk::enablePhysicalDeviceFeatures(
    VkPhysicalDeviceFeatures2 &features) {
    auto descIndexingFeatures =
        vk_find_struct<VkPhysicalDeviceDescriptorIndexingFeaturesEXT>(
            &features);
    if (descIndexingFeatures == nullptr) {
        return false;
    }
    descIndexingFeatures->descriptorBindingSampledImageUpdateAfterBind =
        VK_TRUE;
    return true;
}

std::vector<const char *> CompositorVk::getRequiredDeviceExtensions() {
    return {VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
}

VkCommandBuffer CompositorVk::getCommandBuffer(uint32_t i) const {
    return m_vkCommandBuffers[i];
}