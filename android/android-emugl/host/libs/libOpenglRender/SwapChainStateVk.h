#ifndef SWAP_CHAIN_STATE_VK_H
#define SWAP_CHAIN_STATE_VK_H

#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "vulkan/cereal/common/goldfish_vk_dispatch.h"

class SwapChainStateVk {
   public:
    static std::vector<const char *> getRequiredInstanceExtensions();
    static std::vector<const char *> getRequiredDeviceExtensions();
    static bool validateQueueFamilyProperties(
        const goldfish_vk::VulkanDispatch &, VkPhysicalDevice, VkSurfaceKHR,
        uint32_t queueFamilyIndex);
    using VkSwapchainCreateInfoKHRPtr =
        std::unique_ptr<VkSwapchainCreateInfoKHR,
                        std::function<void(VkSwapchainCreateInfoKHR *)>>;
    static VkSwapchainCreateInfoKHRPtr createSwapChainCi(
        const goldfish_vk::VulkanDispatch &, VkSurfaceKHR, VkPhysicalDevice,
        uint32_t width, uint32_t height,
        const std::unordered_set<uint32_t> &queueFamilyIndices);

    explicit SwapChainStateVk(const goldfish_vk::VulkanDispatch &, VkDevice,
                     const VkSwapchainCreateInfoKHR &);
    ~SwapChainStateVk();
    VkFormat getFormat();
    const std::vector<VkImage>& getVkImages() const;
    const std::vector<VkImageView>& getVkImageViews() const;
    VkSwapchainKHR getSwapChain() const;

   private:
    const static VkFormat k_vkFormat = VK_FORMAT_B8G8R8A8_UNORM;
    const static VkColorSpaceKHR k_vkColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    const goldfish_vk::VulkanDispatch &m_vk;
    VkDevice m_vkDevice;
    VkSwapchainKHR m_vkSwapChain;
    std::vector<VkImage> m_vkImages;
    std::vector<VkImageView> m_vkImageViews;
};

#endif