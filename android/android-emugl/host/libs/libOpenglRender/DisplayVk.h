#ifndef DISPLAY_VK_H
#define DISPLAY_VK_H

#include <functional>
#include <memory>
#include <optional>

#include "ColorBuffer.h"
#include "CompositorVk.h"
#include "RenderContext.h"
#include "SwapChainStateVk.h"
#include "vulkan/cereal/common/goldfish_vk_dispatch.h"

// The DisplayVk class holds the Vulkan and other states required to draw a
// frame in a host window.

class DisplayVk {
   public:
    DisplayVk(const goldfish_vk::VulkanDispatch &, VkPhysicalDevice,
              uint32_t swapChainQueueFamilyIndex,
              uint32_t compositorQueueFamilyIndex, VkDevice,
              VkQueue compositorVkQueue, VkQueue swapChainVkQueue);
    ~DisplayVk();
    void bindToSurface(VkSurfaceKHR, uint32_t width, uint32_t height);
    void importVkImage(HandleType id, VkImage, VkFormat, uint32_t width,
                       uint32_t height);
    void releaseImport(HandleType id);
    // To display a colorbuffer, first import it by calling importVkImage
    void post(HandleType id);

   private:
    bool canComposite(VkFormat);

    const goldfish_vk::VulkanDispatch &m_vk;
    VkPhysicalDevice m_vkPhysicalDevice;
    VkFormatFeatureFlags m_swapChainImageFeatures;
    uint32_t m_swapChainQueueFamilyIndex;
    uint32_t m_compositorQueueFamilyIndex;
    VkDevice m_vkDevice;
    VkQueue m_compositorVkQueue;
    VkQueue m_swapChainVkQueue;
    VkCommandPool m_vkCommandPool;
    VkSampler m_compositionVkSampler;
    VkFence m_frameDrawCompleteFence;
    VkSemaphore m_imageReadySem;
    VkSemaphore m_frameDrawCompleteSem;

    struct ColorBufferInfo {
        uint32_t m_width;
        uint32_t m_height;
        VkFormat m_vkFormat;
        VkImageView m_vkImageView;
    };
    // This map won't automatically relaase all the created VkImageView. The
    // client is responsible for calling releaseImport on all imported ids
    // before this object is released.
    std::unordered_map<HandleType, ColorBufferInfo> m_colorBuffers;

    std::unique_ptr<SwapChainStateVk> m_swapChainStateVk;
    std::unique_ptr<CompositorVk> m_compositorVk;
    struct SurfaceState {
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        std::optional<HandleType> m_prevColorBuffer = std::nullopt;
    };
    std::unique_ptr<SurfaceState> m_surfaceState;
    std::unordered_map<VkFormat, bool> m_canComposite;
};

#endif