#ifndef COMPOSITOR_VK_H
#define COMPOSITOR_VK_H

#include <memory>
#include <vector>

#include "vulkan/cereal/common/goldfish_vk_dispatch.h"

class CompositorVk {
   public:
    static std::unique_ptr<CompositorVk> create(
        const goldfish_vk::VulkanDispatch &vk, VkDevice);
    static bool validatePhysicalDeviceFeatures(
        const VkPhysicalDeviceFeatures2 &features);
    static bool validateQueueFamilyProperties(
        const VkQueueFamilyProperties &properties);
    static bool enablePhysicalDeviceFeatures(
        VkPhysicalDeviceFeatures2 &features);
    static std::vector<const char *> getRequiredDeviceExtensions();

   private:
    explicit CompositorVk(const goldfish_vk::VulkanDispatch &, VkDevice);

    const goldfish_vk::VulkanDispatch &m_vk;
    const VkDevice m_vkDevice;
};

#endif /* COMPOSITOR_VK_H */