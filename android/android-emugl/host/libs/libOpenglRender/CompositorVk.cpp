#include "CompositorVk.h"

#include "vulkan/vk_util.h"

namespace CompositorVkShader {
#include "vulkan/CompositorFragmentShader.h"
#include "vulkan/CompositorVertexShader.h"
}  // namespace CompositorVkShader

std::unique_ptr<CompositorVk> CompositorVk::create(
    const goldfish_vk::VulkanDispatch &vk, VkDevice vkDevice) {
    return std::unique_ptr<CompositorVk>(new CompositorVk(vk, vkDevice));
}

CompositorVk::CompositorVk(const goldfish_vk::VulkanDispatch &vk,
                           VkDevice vkDevice)
    : m_vk(vk), m_vkDevice(vkDevice) {}

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