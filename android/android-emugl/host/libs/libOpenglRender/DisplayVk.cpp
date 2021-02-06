#include "DisplayVk.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include "ErrorLog.h"

static void repaintCallback(void *) {}

DisplayVk::DisplayVk(const goldfish_vk::VulkanDispatch &vk,
                     VkPhysicalDevice vkPhysicalDevice,
                     uint32_t swapChainQueueFamilyIndex,
                     uint32_t compositorQueueFamilyIndex, VkDevice vkDevice,
                     VkQueue compositorVkQueue, VkQueue swapChainVkqueue)
    : m_vk(vk),
      m_vkPhysicalDevice(vkPhysicalDevice),
      m_swapChainQueueFamilyIndex(swapChainQueueFamilyIndex),
      m_compositorQueueFamilyIndex(compositorQueueFamilyIndex),
      m_swapChainVkQueue(swapChainVkqueue),
      m_vkDevice(vkDevice),
      m_compositorVkQueue(compositorVkQueue),
      m_vkCommandPool(VK_NULL_HANDLE),
      m_swapChainStateVk(nullptr),
      m_compositorVk(nullptr),
      m_surfaceState(nullptr) {
    // TODO(kaiyili): validate the capabilites of the passed in Vulkan
    // components.
    VkCommandPoolCreateInfo commandPoolCi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = m_compositorQueueFamilyIndex,
    };
    VK_CHECK(m_vk.vkCreateCommandPool(m_vkDevice, &commandPoolCi, nullptr,
                                      &m_vkCommandPool));
    VkFenceCreateInfo fenceCi = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                 .flags = VK_FENCE_CREATE_SIGNALED_BIT};
    VK_CHECK(m_vk.vkCreateFence(m_vkDevice, &fenceCi, nullptr,
                                &m_frameDrawCompleteFence));
    VkSemaphoreCreateInfo semaphoreCi = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHECK(m_vk.vkCreateSemaphore(m_vkDevice, &semaphoreCi, nullptr,
                                    &m_imageReadySem));
    VK_CHECK(m_vk.vkCreateSemaphore(m_vkDevice, &semaphoreCi, nullptr,
                                    &m_frameDrawCompleteSem));

    VkSamplerCreateInfo samplerCi = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
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
                                  &m_compositionVkSampler));
}

DisplayVk::~DisplayVk() {
    m_vk.vkDestroySampler(m_vkDevice, m_compositionVkSampler, nullptr);
    m_vk.vkDestroySemaphore(m_vkDevice, m_imageReadySem, nullptr);
    m_vk.vkDestroySemaphore(m_vkDevice, m_frameDrawCompleteSem, nullptr);
    m_vk.vkDestroyFence(m_vkDevice, m_frameDrawCompleteFence, nullptr);
    m_compositorVk.reset();
    m_swapChainStateVk.reset();
    m_vk.vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
}

void DisplayVk::bindToSurface(VkSurfaceKHR surface, uint32_t width,
                              uint32_t height) {
    if (!SwapChainStateVk::validateQueueFamilyProperties(
            m_vk, m_vkPhysicalDevice, surface, m_swapChainQueueFamilyIndex)) {
        ERR("%s(%s:%d): DisplayVk can't create VkSwapchainKHR with given "
            "VkDevice and VkSurfaceKHR.\n",
            __FUNCTION__, __FILE__, static_cast<int>(__LINE__));
        ::abort();
    }
    auto swapChainCi = SwapChainStateVk::createSwapChainCi(
        m_vk, surface, m_vkPhysicalDevice, width, height,
        {m_swapChainQueueFamilyIndex, m_compositorQueueFamilyIndex});
    VkFormatProperties formatProps;
    m_vk.vkGetPhysicalDeviceFormatProperties(
        m_vkPhysicalDevice, swapChainCi->imageFormat, &formatProps);
    if (!(formatProps.optimalTilingFeatures &
          VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
        ERR("%s(%s:%d): DisplayVk: The image format chosen for present VkImage "
            "can't be used as the color attachment, and therefore can't be "
            "used as the render target of CompositorVk.\n",
            __FUNCTION__, __FILE__, static_cast<int>(__LINE__));
        ::abort();
    }
    m_swapChainStateVk =
        std::make_unique<SwapChainStateVk>(m_vk, m_vkDevice, *swapChainCi);
    m_compositorVk = CompositorVk::create(
        m_vk, m_vkDevice, m_vkPhysicalDevice, m_compositorVkQueue,
        m_swapChainStateVk->getFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, width, height,
        m_swapChainStateVk->getVkImageViews(), m_vkCommandPool);
    auto surfaceState = std::make_unique<SurfaceState>();
    surfaceState->m_height = height;
    surfaceState->m_width = width;
    m_surfaceState = std::move(surfaceState);
}

std::shared_ptr<DisplayVk::DisplayBufferInfo> DisplayVk::createDisplayBuffer(
    VkImage image, VkFormat format, uint32_t width, uint32_t height) {
    return std::shared_ptr<DisplayBufferInfo>(
        new DisplayBufferInfo(m_vk, m_vkDevice, width, height, format, image));
}

void DisplayVk::post(
    const std::shared_ptr<DisplayBufferInfo> &displayBufferPtr) {
    if (m_swapChainStateVk == nullptr || m_compositorVk == nullptr) {
        ERR("%s(%s:%d): Haven't bound to a surface, can't post color buffer.\n",
            __FUNCTION__, __FILE__, static_cast<int>(__LINE__));
        return;
    }
    auto &surfaceState = *m_surfaceState;
    const auto &db = *displayBufferPtr;

    VK_CHECK(m_vk.vkWaitForFences(m_vkDevice, 1, &m_frameDrawCompleteFence,
                                  VK_TRUE, UINT64_MAX));
    uint32_t imageIndex;
    VK_CHECK(m_vk.vkAcquireNextImageKHR(
        m_vkDevice, m_swapChainStateVk->getSwapChain(), UINT64_MAX,
        m_imageReadySem, VK_NULL_HANDLE, &imageIndex));
    auto maybePrevDisplayBuffer = surfaceState.m_prevDisplayBuffer.lock();
    if (!maybePrevDisplayBuffer || maybePrevDisplayBuffer != displayBufferPtr) {
        if (!canComposite(db.m_vkFormat)) {
            ERR("%s(%s:%d): Can't composite the DisplayBuffer(0x%" PRIxPTR
                "). The image(VkFormat = %" PRIu64 ") can be sampled from.\n",
                __FUNCTION__, __FILE__, static_cast<int>(__LINE__),
                reinterpret_cast<uintptr_t>(displayBufferPtr.get()),
                static_cast<uint64_t>(db.m_vkFormat));
            return;
        }
        auto composition = std::make_unique<Composition>(
            db.m_vkImageView, m_compositionVkSampler, db.m_width, db.m_height);
        composition->m_transform =
            glm::scale(glm::mat3(1.0),
                       glm::vec2(static_cast<float>(surfaceState.m_width) /
                                     static_cast<float>(db.m_width),
                                 static_cast<float>(surfaceState.m_height) /
                                     static_cast<float>(db.m_height)));
        m_compositorVk->setComposition(imageIndex, std::move(composition));
        surfaceState.m_prevDisplayBuffer = displayBufferPtr;
    }

    auto cmdBuff = m_compositorVk->getCommandBuffer(imageIndex);

    VK_CHECK(m_vk.vkResetFences(m_vkDevice, 1, &m_frameDrawCompleteFence));
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores = &m_imageReadySem,
                               .pWaitDstStageMask = waitStages,
                               .commandBufferCount = 1,
                               .pCommandBuffers = &cmdBuff,
                               .signalSemaphoreCount = 1,
                               .pSignalSemaphores = &m_frameDrawCompleteSem};
    VK_CHECK(m_vk.vkQueueSubmit(m_compositorVkQueue, 1, &submitInfo,
                                m_frameDrawCompleteFence));

    auto swapChain = m_swapChainStateVk->getSwapChain();
    VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                    .waitSemaphoreCount = 1,
                                    .pWaitSemaphores = &m_frameDrawCompleteSem,
                                    .swapchainCount = 1,
                                    .pSwapchains = &swapChain,
                                    .pImageIndices = &imageIndex};
    VK_CHECK(m_vk.vkQueuePresentKHR(m_swapChainVkQueue, &presentInfo));
    VK_CHECK(m_vk.vkWaitForFences(m_vkDevice, 1, &m_frameDrawCompleteFence,
                                  VK_TRUE, UINT64_MAX));
}

bool DisplayVk::canComposite(VkFormat format) {
    auto it = m_canComposite.find(format);
    if (it != m_canComposite.end()) {
        return it->second;
    }
    VkFormatProperties formatProps = {};
    m_vk.vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, format,
                                             &formatProps);
    bool res =
        formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    m_canComposite.emplace(format, res);
    return res;
}

DisplayVk::DisplayBufferInfo::DisplayBufferInfo(
    const goldfish_vk::VulkanDispatch &vk, VkDevice vkDevice, uint32_t width,
    uint32_t height, VkFormat format, VkImage image)
    : m_vk(vk),
      m_vkDevice(vkDevice),
      m_width(width),
      m_height(height),
      m_vkFormat(format),
      m_vkImageView(VK_NULL_HANDLE) {
    VkImageViewCreateInfo imageViewCi = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                       .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    VK_CHECK(m_vk.vkCreateImageView(m_vkDevice, &imageViewCi, nullptr,
                                    &m_vkImageView));
}

DisplayVk::DisplayBufferInfo::~DisplayBufferInfo() {
    m_vk.vkDestroyImageView(m_vkDevice, m_vkImageView, nullptr);
}
