#include "SwapChainStateVk.h"

#include "vulkan/vk_util.h"

SwapChainStateVk::SwapChainStateVk(const goldfish_vk::VulkanDispatch &vk,
                                   VkDevice vkDevice,
                                   const VkSwapchainCreateInfoKHR &swapChainCi)
    : m_vk(vk),
      m_vkDevice(vkDevice),
      m_vkSwapChain(VK_NULL_HANDLE),
      m_vkImages(0),
      m_vkImageViews(0) {
    VK_CHECK(m_vk.vkCreateSwapchainKHR(m_vkDevice, &swapChainCi, nullptr,
                                       &m_vkSwapChain));
    uint32_t imageCount = 0;
    VK_CHECK(m_vk.vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain,
                                          &imageCount, nullptr));
    m_vkImages.resize(imageCount);
    VK_CHECK(m_vk.vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain,
                                          &imageCount, m_vkImages.data()));
    for (auto i = 0; i < m_vkImages.size(); i++) {
        VkImageViewCreateInfo imageViewCi = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_vkImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = k_vkFormat,
            .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                           .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                           .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                           .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel = 0,
                                 .levelCount = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount = 1}};
        VkImageView vkImageView;
        VK_CHECK(m_vk.vkCreateImageView(m_vkDevice, &imageViewCi, nullptr,
                                        &vkImageView));
        m_vkImageViews.push_back(vkImageView);
    }
}

SwapChainStateVk::~SwapChainStateVk() {
    for (auto imageView : m_vkImageViews) {
        m_vk.vkDestroyImageView(m_vkDevice, imageView, nullptr);
    }
    m_vk.vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
}

std::vector<const char *> SwapChainStateVk::getRequiredInstanceExtensions() {
    return {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef __APPLE__
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
    };
}

std::vector<const char *> SwapChainStateVk::getRequiredDeviceExtensions() {
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
}

bool SwapChainStateVk::validateQueueFamilyProperties(
    const goldfish_vk::VulkanDispatch &vk, VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface, uint32_t queueFamilyIndex) {
    VkBool32 presentSupport = VK_FALSE;
    VK_CHECK(vk.vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice, queueFamilyIndex, surface, &presentSupport));
    return presentSupport;
}

SwapChainStateVk::VkSwapchainCreateInfoKHRPtr
SwapChainStateVk::createSwapChainCi(
    const goldfish_vk::VulkanDispatch &vk, VkSurfaceKHR surface,
    VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
    const std::unordered_set<uint32_t> &queueFamilyIndices) {
    uint32_t formatCount = 0;
    VK_CHECK(vk.vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                                     &formatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    VK_CHECK(vk.vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice, surface, &formatCount, formats.data()));
    auto iSurfaceFormat = std::find_if(
        formats.begin(), formats.end(), [](const VkSurfaceFormatKHR &format) {
            return format.format == k_vkFormat &&
                   format.colorSpace == k_vkColorSpace;
        });
    if (iSurfaceFormat == formats.end()) {
        return nullptr;
    }

    uint32_t presentModeCount = 0;
    VK_CHECK(vk.vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &presentModeCount, nullptr));
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    auto iPresentMode =
        std::find_if(presentModes.begin(), presentModes.end(),
                     [](const VkPresentModeKHR &presentMode) {
                         return presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR;
                     });
    if (iPresentMode == presentModes.end()) {
        return nullptr;
    }
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice, surface, &surfaceCaps));
    std::optional<VkExtent2D> maybeExtent = std::nullopt;
    if (surfaceCaps.currentExtent.width != UINT32_MAX &&
        surfaceCaps.currentExtent.width == width &&
        surfaceCaps.currentExtent.height == height) {
        maybeExtent = surfaceCaps.currentExtent;
    } else if (width >= surfaceCaps.minImageExtent.width &&
               width <= surfaceCaps.maxImageExtent.width &&
               height >= surfaceCaps.minImageExtent.height &&
               height <= surfaceCaps.maxImageExtent.height) {
        maybeExtent = VkExtent2D({width, height});
    }
    if (!maybeExtent.has_value()) {
        return nullptr;
    }
    auto extent = maybeExtent.value();
    uint32_t imageCount = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount != 0 &&
        surfaceCaps.maxImageCount < imageCount) {
        imageCount = surfaceCaps.maxImageCount;
    }
    VkSwapchainCreateInfoKHRPtr swapChainCi(
        new VkSwapchainCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface,
            .minImageCount = imageCount,
            .imageFormat = iSurfaceFormat->format,
            .imageColorSpace = iSurfaceFormat->colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = surfaceCaps.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = *iPresentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE},
        [](VkSwapchainCreateInfoKHR *p) {
            if (p->pQueueFamilyIndices != nullptr) {
                delete[] p->pQueueFamilyIndices;
            }
        });
    if (queueFamilyIndices.empty()) {
        return nullptr;
    }
    if (queueFamilyIndices.size() == 1) {
        swapChainCi->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCi->queueFamilyIndexCount = 0;
        swapChainCi->pQueueFamilyIndices = nullptr;
    } else {
        swapChainCi->imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCi->queueFamilyIndexCount =
            static_cast<uint32_t>(queueFamilyIndices.size());
        uint32_t *pQueueFamilyIndices = new uint32_t[queueFamilyIndices.size()];
        std::copy(queueFamilyIndices.begin(), queueFamilyIndices.end(),
                  pQueueFamilyIndices);
        swapChainCi->pQueueFamilyIndices = pQueueFamilyIndices;
    }
    return swapChainCi;
}

VkFormat SwapChainStateVk::getFormat() { return k_vkFormat; }

const std::vector<VkImage> &SwapChainStateVk::getVkImages() const {
    return m_vkImages;
}

const std::vector<VkImageView> &SwapChainStateVk::getVkImageViews() const {
    return m_vkImageViews;
}

VkSwapchainKHR SwapChainStateVk::getSwapChain() const { return m_vkSwapChain; }