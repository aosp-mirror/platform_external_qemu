#include "DisplayVk.h"

#include "ErrorLog.h"
#include "NativeSubWindow.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>

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
      m_surfaceState(nullptr),
      m_canComposite() {
    // TODO(kaiyili): validate the capabilites of the passed in Vulkan
    // components.
    VkCommandPoolCreateInfo commandPoolCi = {};
    commandPoolCi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCi.queueFamilyIndex = m_compositorQueueFamilyIndex;
    VK_CHECK(m_vk.vkCreateCommandPool(m_vkDevice, &commandPoolCi, nullptr,
                                      &m_vkCommandPool));
    VkFenceCreateInfo fenceCi = {};
    fenceCi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(m_vk.vkCreateFence(m_vkDevice, &fenceCi, nullptr,
                                &m_frameDrawCompleteFence));
    VkSemaphoreCreateInfo semaphoreCi = {};
    semaphoreCi.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(m_vk.vkCreateSemaphore(m_vkDevice, &semaphoreCi, nullptr,
                                    &m_imageReadySem));
    VK_CHECK(m_vk.vkCreateSemaphore(m_vkDevice, &semaphoreCi, nullptr,
                                    &m_frameDrawCompleteSem));

    VkSamplerCreateInfo samplerCi = {};
    samplerCi.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCi.magFilter = VK_FILTER_NEAREST;
    samplerCi.minFilter = VK_FILTER_NEAREST;
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

void DisplayVk::importVkImage(HandleType id, VkImage image, VkFormat format,
                              uint32_t width, uint32_t height) {
    if (m_colorBuffers.find(id) != m_colorBuffers.end()) {
        ERR("%s(%s:%d): DisplayVk can't import VkImage with duplicate id = "
            "%" PRIu64 ".\n",
            __FUNCTION__, __FILE__, static_cast<int>(__LINE__),
            static_cast<uint64_t>(id));
        ::abort();
    }
    ColorBufferInfo cbInfo = {};
    cbInfo.m_width = width;
    cbInfo.m_height = height;
    cbInfo.m_vkFormat = format;

    VkImageViewCreateInfo imageViewCi = {};
    imageViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCi.image = image;
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
                                    &cbInfo.m_vkImageView));
    m_colorBuffers.emplace(id, cbInfo);
}

void DisplayVk::releaseImport(HandleType id) {
    auto it = m_colorBuffers.find(id);
    if (it == m_colorBuffers.end()) {
        return;
    }
    m_vk.vkDestroyImageView(m_vkDevice, it->second.m_vkImageView, nullptr);
    m_colorBuffers.erase(it);
}

void DisplayVk::post(HandleType id) {
    if (m_swapChainStateVk == nullptr || m_compositorVk == nullptr) {
        ERR("%s(%s:%d): Haven't bound to a surface, can't post color buffer.\n",
            __FUNCTION__, __FILE__, static_cast<int>(__LINE__));
        return;
    }
    const auto &surfaceState = *m_surfaceState;

    VK_CHECK(m_vk.vkWaitForFences(m_vkDevice, 1, &m_frameDrawCompleteFence,
                                  VK_TRUE, UINT64_MAX));
    uint32_t imageIndex;
    VK_CHECK(m_vk.vkAcquireNextImageKHR(
        m_vkDevice, m_swapChainStateVk->getSwapChain(), UINT64_MAX,
        m_imageReadySem, VK_NULL_HANDLE, &imageIndex));
    if (!surfaceState.m_prevColorBuffer.has_value() ||
        surfaceState.m_prevColorBuffer.value() != id) {
        const auto &cb = m_colorBuffers.at(id);
        if (!canComposite(cb.m_vkFormat)) {
            ERR("%s(%s:%d): Can't composite the ColorBuffer(id = %" PRIu64
                "). The image(VkFormat = %" PRIu64 ") can be sampled from.\n",
                __FUNCTION__, __FILE__, static_cast<int>(__LINE__),
                static_cast<uint64_t>(id),
                static_cast<uint64_t>(cb.m_vkFormat));
            return;
        }
        auto composition = std::make_unique<Composition>(
            cb.m_vkImageView, m_compositionVkSampler, cb.m_width, cb.m_height);
        composition->m_transform =
            glm::scale(glm::mat3(1.0),
                       glm::vec2(static_cast<float>(surfaceState.m_width) /
                                     static_cast<float>(cb.m_width),
                                 static_cast<float>(surfaceState.m_height) /
                                     static_cast<float>(cb.m_height)));
        m_compositorVk->setComposition(imageIndex, std::move(composition));
    }

    auto cmdBuff = m_compositorVk->getCommandBuffer(imageIndex);

    VK_CHECK(m_vk.vkResetFences(m_vkDevice, 1, &m_frameDrawCompleteFence));
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_imageReadySem;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuff;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_frameDrawCompleteSem;
    VK_CHECK(m_vk.vkQueueSubmit(m_compositorVkQueue, 1, &submitInfo,
                                m_frameDrawCompleteFence));

    auto swapChain = m_swapChainStateVk->getSwapChain();
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_frameDrawCompleteSem;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
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
    m_vk.vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, format, &formatProps);
    bool res = formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    m_canComposite.emplace(format, res);
    return res;
}