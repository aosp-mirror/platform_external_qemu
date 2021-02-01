#ifndef COMPOSITOR_VK_H
#define COMPOSITOR_VK_H

#include <memory>
#include <variant>
#include <vector>

#include "vulkan/cereal/common/goldfish_vk_dispatch.h"

class CompositorVk {
   public:
    static std::unique_ptr<CompositorVk> create(
        const goldfish_vk::VulkanDispatch &vk, VkDevice, VkFormat,
        VkImageLayout initialLayout, VkImageLayout finalLayout, uint32_t width,
        uint32_t height, const std::vector<VkImageView> &renderTargets,
        VkCommandPool);
    static bool validatePhysicalDeviceFeatures(
        const VkPhysicalDeviceFeatures2 &features);
    static bool validateQueueFamilyProperties(
        const VkQueueFamilyProperties &properties);
    static bool enablePhysicalDeviceFeatures(
        VkPhysicalDeviceFeatures2 &features);
    static std::vector<const char *> getRequiredDeviceExtensions();

    ~CompositorVk();
    VkCommandBuffer getCommandBuffer(uint32_t i) const;

   private:
    explicit CompositorVk(const goldfish_vk::VulkanDispatch &, VkDevice,
                          VkCommandPool);
    void setUpGraphicsPipeline(uint32_t width, uint32_t height,
                               VkFormat renderTargetFormat,
                               VkImageLayout initialLayout,
                               VkImageLayout finalLayout);
    void setUpFramebuffers(const std::vector<VkImageView> &, uint32_t width,
                           uint32_t height);
    void setUpCommandBuffers(uint32_t width, uint32_t height);

    const goldfish_vk::VulkanDispatch &m_vk;
    const VkDevice m_vkDevice;
    VkPipelineLayout m_vkPipelineLayout;
    VkRenderPass m_vkRenderPass;
    VkPipeline m_graphicsVkPipeline;
    std::vector<VkCommandBuffer> m_vkCommandBuffers;
    std::vector<VkFramebuffer> m_renderTargetVkFrameBuffers;

    VkCommandPool m_vkCommandPool;
};

#endif /* COMPOSITOR_VK_H */