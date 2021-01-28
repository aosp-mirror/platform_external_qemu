#include "CompositorVk.h"

#include <string.h>

#include <cinttypes>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>

#include "ErrorLog.h"
#include "vulkan/vk_util.h"

namespace CompositorVkShader {
#include "vulkan/CompositorFragmentShader.h"
#include "vulkan/CompositorVertexShader.h"
}  // namespace CompositorVkShader

static VkShaderModule createShaderModule(const goldfish_vk::VulkanDispatch &vk,
                                         VkDevice device,
                                         const std::vector<uint32_t> &code) {
    VkShaderModuleCreateInfo shaderModuleCi = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<uint32_t>(code.size() * sizeof(uint32_t)),
        .pCode = code.data()};
    VkShaderModule res;
    VK_CHECK(vk.vkCreateShaderModule(device, &shaderModuleCi, nullptr, &res));
    return res;
}

ComposeLayerVk::ComposeLayerVk(VkSampler vkSampler, VkImageView vkImageView,
                               const LayerTransform &layerTransform)
    : m_vkSampler(vkSampler),
      m_vkImageView(vkImageView),
      m_layerTransform(
          {.pos = layerTransform.pos, .texcoord = layerTransform.texcoord}) {}

std::unique_ptr<ComposeLayerVk> ComposeLayerVk::createFromHwc2ComposeLayer(
    VkSampler vkSampler, VkImageView vkImageView,
    const ComposeLayer &composeLayer, uint32_t cbWidth, uint32_t cbHeight,
    uint32_t dstWidth, uint32_t dstHeight) {
    // Calculate the posTransform and the texcoordTransform needed in the
    // uniform of the Compositor.vert shader. The posTransform should transform
    // the square(top = -1, bottom = 1, left = -1, right = 1) to the position
    // where the layer should be drawn in NDC space given the composeLayer.
    // texcoordTransform should transform the unit square(top = 0, bottom = 1,
    // left = 0, right = 1) to where we should sample the layer in the
    // normalized uv space given the composeLayer.
    const hwc_rect_t &posRect = composeLayer.displayFrame;
    const hwc_frect_t &texcoordRect = composeLayer.crop;

    int posWidth = posRect.right - posRect.left;
    int posHeight = posRect.bottom - posRect.top;

    float posScaleX = float(posWidth) / dstWidth;
    float posScaleY = float(posHeight) / dstHeight;

    float posTranslateX =
        -1.0f + posScaleX + 2.0f * float(posRect.left) / dstWidth;
    float posTranslateY =
        -1.0f + posScaleY + 2.0f * float(posRect.top) / dstHeight;

    float texcoordScalX =
        (texcoordRect.right - texcoordRect.left) / float(cbWidth);
    float texCoordScaleY =
        (texcoordRect.bottom - texcoordRect.top) / float(cbHeight);

    float texCoordTranslateX = texcoordRect.left / float(cbWidth);
    float texCoordTranslateY = texcoordRect.top / float(cbHeight);

    float texcoordRotation = 0.0f;

    const float pi = glm::pi<float>();

    switch (composeLayer.transform) {
        case HWC_TRANSFORM_ROT_90:
            texcoordRotation = pi * 0.5f;
            break;
        case HWC_TRANSFORM_ROT_180:
            texcoordRotation = pi;
            break;
        case HWC_TRANSFORM_ROT_270:
            texcoordRotation = pi * 1.5f;
            break;
        case HWC_TRANSFORM_FLIP_H:
            texcoordScalX *= -1.0f;
            break;
        case HWC_TRANSFORM_FLIP_V:
            texCoordScaleY *= -1.0f;
            break;
        case HWC_TRANSFORM_FLIP_H_ROT_90:
            texcoordRotation = pi * 0.5f;
            texcoordScalX *= -1.0f;
            break;
        case HWC_TRANSFORM_FLIP_V_ROT_90:
            texcoordRotation = pi * 0.5f;
            texCoordScaleY *= -1.0f;
            break;
        default:
            break;
    }

    ComposeLayerVk::LayerTransform layerTransform = {
        .pos =
            glm::translate(glm::mat4(1.0f),
                           glm::vec3(posTranslateX, posTranslateY, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(posScaleX, posScaleY, 1.0f)),
        .texcoord = glm::translate(glm::mat4(1.0f),
                                   glm::vec3(texCoordTranslateX,
                                             texCoordTranslateY, 0.0f)) *
                    glm::scale(glm::mat4(1.0f),
                               glm::vec3(texcoordScalX, texCoordScaleY, 1.0f)) *
                    glm::rotate(glm::mat4(1.0f), texcoordRotation,
                                glm::vec3(0.0f, 0.0f, 1.0f)),
    };

    return std::unique_ptr<ComposeLayerVk>(
        new ComposeLayerVk(vkSampler, vkImageView, layerTransform));
}

Composition::Composition(
    std::vector<std::unique_ptr<ComposeLayerVk>> composeLayers)
    : m_composeLayers(std::move(composeLayers)) {}

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
        std::vector<std::unique_ptr<ComposeLayerVk>> emptyCompositionLayers;
        res->setComposition(i, std::make_unique<Composition>(
                                   std::move(emptyCompositionLayers)));
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
    (void)m_renderTargetWidth;
    (void)m_renderTargetHeight;

    VkPhysicalDeviceProperties physicalDeviceProperties;
    m_vk.vkGetPhysicalDeviceProperties(m_vkPhysicalDevice,
                                       &physicalDeviceProperties);
    auto alignment =
        physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    m_uniformStorage.m_stride =
        ((sizeof(UniformBufferObject) - 1) / alignment + 1) * alignment;
}

CompositorVk::~CompositorVk() {
    m_vk.vkDestroySampler(m_vkDevice, m_emptyCompositionVkSampler, nullptr);
    m_vk.vkDestroyImageView(m_vkDevice, m_emptyCompositionVkImageView, nullptr);
    m_vk.vkFreeMemory(m_vkDevice, m_emptyCompositionVkDeviceMemory, nullptr);
    m_vk.vkDestroyImage(m_vkDevice, m_emptyCompositionVkImage, nullptr);
    m_vk.vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
    m_vk.vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool,
                              static_cast<uint32_t>(m_vkCommandBuffers.size()),
                              m_vkCommandBuffers.data());
    if (m_uniformStorage.m_vkDeviceMemory != VK_NULL_HANDLE) {
        m_vk.vkUnmapMemory(m_vkDevice, m_uniformStorage.m_vkDeviceMemory);
    }
    m_vk.vkDestroyBuffer(m_vkDevice, m_uniformStorage.m_vkBuffer, nullptr);
    m_vk.vkFreeMemory(m_vkDevice, m_uniformStorage.m_vkDeviceMemory, nullptr);
    while (!m_renderTargetVkFrameBuffers.empty()) {
        m_vk.vkDestroyFramebuffer(m_vkDevice,
                                  m_renderTargetVkFrameBuffers.back(), nullptr);
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

    VkPipelineShaderStageCreateInfo shaderStageCis[2] = {
        {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_VERTEX_BIT,
         .module = vertShaderMod,
         .pName = "main"},
        {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
         .module = fragShaderMod,
         .pName = "main"}};

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescription();
    VkPipelineVertexInputStateCreateInfo vertexInputStateCi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescription.size()),
        .pVertexAttributeDescriptions = attributeDescription.data()};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {.x = 0.0f,
                           .y = 0.0f,
                           .width = static_cast<float>(width),
                           .height = static_cast<float>(height),
                           .minDepth = 0.0f,
                           .maxDepth = 1.0f};

    VkRect2D scissor = {.offset = {0, 0}, .extent = {width, height}};

    VkPipelineViewportStateCreateInfo viewportStateCi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor};

    VkPipelineRasterizationStateCreateInfo rasterizerStateCi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f};

    VkPipelineMultisampleStateCreateInfo multisampleStateCi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE};

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_MAX,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    VkPipelineColorBlendStateCreateInfo colorBlendStateCi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment};

    VkDescriptorSetLayoutBinding layoutBindings[2] = {
        {.binding = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
         .pImmutableSamplers = nullptr},
        {.binding = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
         .pImmutableSamplers = nullptr}};

    VkDescriptorBindingFlagsEXT bindingFlags[] = {
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT, 0};
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsCi = {
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        .bindingCount = static_cast<uint32_t>(std::size(bindingFlags)),
        .pBindingFlags = bindingFlags,
    };
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCi = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindingFlagsCi,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
        .bindingCount = static_cast<uint32_t>(std::size(layoutBindings)),
        .pBindings = layoutBindings};
    VK_CHECK(m_vk.vkCreateDescriptorSetLayout(
        m_vkDevice, &descriptorSetLayoutCi, nullptr, &m_vkDescriptorSetLayout));

    VkPipelineLayoutCreateInfo pipelineLayoutCi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_vkDescriptorSetLayout,
        .pushConstantRangeCount = 0};

    VK_CHECK(m_vk.vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutCi, nullptr,
                                         &m_vkPipelineLayout));

    VkAttachmentDescription colorAttachment = {
        .format = renderTargetFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = initialLayout,
        .finalLayout = finalLayout};

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef};

    // TODO: to support multiple layer composition, we could run the same render
    // pass for multiple time. In that case, we should use explicit
    // VkImageMemoryBarriers to transform the image layout instead of relying on
    // renderpass to do it.
    VkSubpassDependency subpassDependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

    VkRenderPassCreateInfo renderPassCi = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency};

    VK_CHECK(m_vk.vkCreateRenderPass(m_vkDevice, &renderPassCi, nullptr,
                                     &m_vkRenderPass));

    VkGraphicsPipelineCreateInfo graphicsPipelineCi = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(std::size(shaderStageCis)),
        .pStages = shaderStageCis,
        .pVertexInputState = &vertexInputStateCi,
        .pInputAssemblyState = &inputAssemblyStateCi,
        .pViewportState = &viewportStateCi,
        .pRasterizationState = &rasterizerStateCi,
        .pMultisampleState = &multisampleStateCi,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlendStateCi,
        .pDynamicState = nullptr,
        .layout = m_vkPipelineLayout,
        .renderPass = m_vkRenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1};

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
    VkBufferCreateInfo bufferCi = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
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
    VkMemoryAllocateInfo memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = maybeMemoryTypeIndex.value()};
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
        VkFramebufferCreateInfo fbCi = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = m_vkRenderPass,
            .attachmentCount = 1,
            .pAttachments = &renderTargets[i],
            .width = width,
            .height = height,
            .layers = 1};
        VkFramebuffer framebuffer;
        VK_CHECK(
            m_vk.vkCreateFramebuffer(m_vkDevice, &fbCi, nullptr, &framebuffer));
        m_renderTargetVkFrameBuffers.push_back(framebuffer);
    }
}

// We don't see composition requests with more than 10 layers from the guest for
// now. If we see rendering error or significant time spent on updating
// descriptors in setComposition, we should tune this number.
static const uint32_t kMaxLayersPerFrame = 10;

void CompositorVk::setUpDescriptorSets() {
    uint32_t numOfFrames =
        static_cast<uint32_t>(m_renderTargetVkFrameBuffers.size());

    uint32_t setsPerDescriptorType = numOfFrames * kMaxLayersPerFrame;

    VkDescriptorPoolSize descriptorPoolSizes[2] = {
        {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = setsPerDescriptorType},
        {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = setsPerDescriptorType}};

    VkDescriptorPoolCreateInfo descriptorPoolCi = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
        .maxSets = static_cast<uint32_t>(setsPerDescriptorType),
        .poolSizeCount = static_cast<uint32_t>(std::size(descriptorPoolSizes)),
        .pPoolSizes = descriptorPoolSizes};
    VK_CHECK(m_vk.vkCreateDescriptorPool(m_vkDevice, &descriptorPoolCi, nullptr,
                                         &m_vkDescriptorPool));
    std::vector<VkDescriptorSetLayout> layouts(setsPerDescriptorType,
                                               m_vkDescriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_vkDescriptorPool,
        .descriptorSetCount = setsPerDescriptorType,
        .pSetLayouts = layouts.data()};
    m_vkDescriptorSets.resize(setsPerDescriptorType);
    VK_CHECK(m_vk.vkAllocateDescriptorSets(m_vkDevice, &descriptorSetAllocInfo,
                                           m_vkDescriptorSets.data()));
    for (size_t i = 0; i < setsPerDescriptorType; i++) {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = m_uniformStorage.m_vkBuffer,
            .offset = i * m_uniformStorage.m_stride,
            .range = sizeof(UniformBufferObject)};
        VkWriteDescriptorSet descriptorSetWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_vkDescriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo};
        m_vk.vkUpdateDescriptorSets(m_vkDevice, 1, &descriptorSetWrite, 0,
                                    nullptr);
    }
}

void CompositorVk::setUpCommandBuffers(uint32_t width, uint32_t height) {
    VkCommandBufferAllocateInfo cmdBuffAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_vkCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount =
            static_cast<uint32_t>(m_renderTargetVkFrameBuffers.size())};
    m_vkCommandBuffers.resize(m_renderTargetVkFrameBuffers.size());
    VK_CHECK(m_vk.vkAllocateCommandBuffers(m_vkDevice, &cmdBuffAllocInfo,
                                           m_vkCommandBuffers.data()));

    for (size_t i = 0; i < m_vkCommandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        const auto &cmdBuffer = m_vkCommandBuffers[i];
        VK_CHECK(m_vk.vkBeginCommandBuffer(cmdBuffer, &beginInfo));

        VkClearValue clearColor = {
            .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderPassBeginInfo renderPassBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = m_vkRenderPass,
            .framebuffer = m_renderTargetVkFrameBuffers[i],
            .renderArea = {.offset = {0, 0}, .extent = {width, height}},
            .clearValueCount = 1,
            .pClearValues = &clearColor};
        m_vk.vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo,
                                  VK_SUBPASS_CONTENTS_INLINE);
        m_vk.vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               m_graphicsVkPipeline);
        VkDeviceSize offsets[] = {0};
        m_vk.vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertexVkBuffer,
                                    offsets);
        m_vk.vkCmdBindIndexBuffer(cmdBuffer, m_indexVkBuffer, 0,
                                  VK_INDEX_TYPE_UINT16);
        for (uint32_t j = 0; j < kMaxLayersPerFrame; ++j) {
            m_vk.vkCmdBindDescriptorSets(
                cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout,
                0, 1, &m_vkDescriptorSets[i * kMaxLayersPerFrame + j], 0,
                nullptr);
            m_vk.vkCmdDrawIndexed(
                cmdBuffer, static_cast<uint32_t>(k_indices.size()), 1, 0, 0, 0);
        }
        m_vk.vkCmdEndRenderPass(cmdBuffer);

        VK_CHECK(m_vk.vkEndCommandBuffer(cmdBuffer));
    }
}

void CompositorVk::setUpEmptyComposition(VkFormat format) {
    VkImageCreateInfo imageCi = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = k_emptyCompositionExtent.width,
                   .height = k_emptyCompositionExtent.height,
                   .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
    VK_CHECK(m_vk.vkCreateImage(m_vkDevice, &imageCi, nullptr,
                                &m_emptyCompositionVkImage));

    VkMemoryRequirements memRequirements;
    m_vk.vkGetImageMemoryRequirements(m_vkDevice, m_emptyCompositionVkImage,
                                      &memRequirements);
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                               .value()};
    VK_CHECK(m_vk.vkAllocateMemory(m_vkDevice, &allocInfo, nullptr,
                                   &m_emptyCompositionVkDeviceMemory));
    VK_CHECK(m_vk.vkBindImageMemory(m_vkDevice, m_emptyCompositionVkImage,
                                    m_emptyCompositionVkDeviceMemory, 0));
    runSingleTimeCommands(m_vkQueue, [&, this](const auto &cmdBuff) {
        recordImageLayoutTransformCommands(
            cmdBuff, m_emptyCompositionVkImage, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkClearColorValue clearColor = {.float32 = {0.0, 0.0, 0.0, 1.0}};
        VkImageSubresourceRange range = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1};
        m_vk.vkCmdClearColorImage(cmdBuff, m_emptyCompositionVkImage,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  &clearColor, 1, &range);
        recordImageLayoutTransformCommands(
            cmdBuff, m_emptyCompositionVkImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

    VkImageViewCreateInfo imageViewCi = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_emptyCompositionVkImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                              .baseMipLevel = 0,
                              .levelCount = 1,
                              .baseArrayLayer = 0,
                              .layerCount = 1}
    };
    VK_CHECK(m_vk.vkCreateImageView(m_vkDevice, &imageViewCi, nullptr,
                                    &m_emptyCompositionVkImageView));

    VkSamplerCreateInfo samplerCi = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE};
    VK_CHECK(m_vk.vkCreateSampler(m_vkDevice, &samplerCi, nullptr,
                                  &m_emptyCompositionVkSampler));
}

void CompositorVk::setUpUniformBuffers() {
    auto numOfFrames = m_renderTargetVkFrameBuffers.size();
    VkDeviceSize size =
        m_uniformStorage.m_stride * numOfFrames * kMaxLayersPerFrame;
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

void CompositorVk::setComposition(uint32_t rtIndex,
                                  std::unique_ptr<Composition> &&composition) {
    m_currentCompositions[rtIndex] = std::move(composition);
    const auto &currentComposition = *m_currentCompositions[rtIndex];
    if (currentComposition.m_composeLayers.size() > kMaxLayersPerFrame) {
        ERR("%s(%s:%d): CompositorVk can't compose more than %" PRIu32
            " layers, %" PRIu32 " layers asked.\n",
            __FUNCTION__, __FILE__, static_cast<int>(__LINE__),
            kMaxLayersPerFrame,
            static_cast<uint32_t>(currentComposition.m_composeLayers.size()));
    }

    memset(reinterpret_cast<uint8_t *>(m_uniformStorage.m_data) +
               (rtIndex * kMaxLayersPerFrame + 0) * m_uniformStorage.m_stride,
           0, sizeof(ComposeLayerVk::LayerTransform) * kMaxLayersPerFrame);

    std::vector<VkDescriptorImageInfo> imageInfos(
        currentComposition.m_composeLayers.size());
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    for (size_t i = 0; i < currentComposition.m_composeLayers.size(); ++i) {
        const auto &layer = currentComposition.m_composeLayers[i];
        imageInfos[i] = VkDescriptorImageInfo(
            {.sampler = layer->m_vkSampler,
             .imageView = layer->m_vkImageView,
             .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        const VkDescriptorImageInfo &imageInfo = imageInfos[i];
        descriptorWrites.emplace_back(VkWriteDescriptorSet(
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .dstSet = m_vkDescriptorSets[rtIndex * kMaxLayersPerFrame + i],
             .dstBinding = 0,
             .dstArrayElement = 0,
             .descriptorCount = 1,
             .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             .pImageInfo = &imageInfo}));
        memcpy(
            reinterpret_cast<uint8_t *>(m_uniformStorage.m_data) +
                (rtIndex * kMaxLayersPerFrame + i) * m_uniformStorage.m_stride,
            &layer->m_layerTransform, sizeof(ComposeLayerVk::LayerTransform));
    }
    m_vk.vkUpdateDescriptorSets(m_vkDevice, descriptorWrites.size(),
                                descriptorWrites.data(), 0, nullptr);

    for (size_t i = currentComposition.m_composeLayers.size();
         i < kMaxLayersPerFrame; ++i) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = m_emptyCompositionVkSampler,
            .imageView = m_emptyCompositionVkImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet descriptorSetWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_vkDescriptorSets[rtIndex * kMaxLayersPerFrame + i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo};
        m_vk.vkUpdateDescriptorSets(m_vkDevice, 1, &descriptorSetWrite, 0,
                                    nullptr);
    }
}

VkVertexInputBindingDescription CompositorVk::Vertex::getBindingDescription() {
    return {.binding = 0,
            .stride = sizeof(struct Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
}

std::array<VkVertexInputAttributeDescription, 2>
CompositorVk::Vertex::getAttributeDescription() {
    return {VkVertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(struct Vertex, pos)},
            VkVertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(struct Vertex, texPos)}};
}
