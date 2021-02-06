#include <gtest/gtest.h>

#include "DisplayVk.h"

#include "Standalone.h"
#include "tests/VkTestUtils.h"
#include "vulkan/VulkanDispatch.h"

class DisplayVkTest : public ::testing::Test {
   protected:
    static void SetUpTestCase() { k_vk = emugl::vkDispatch(false); }

    void SetUp() override {
        // skip the test when testing without a window
        if (!emugl::shouldUseWindow()) {
            GTEST_SKIP();
        }
        ASSERT_NE(k_vk, nullptr);

        createInstance();
        createWindowAndSurface();
        m_window = emugl::createOrGetTestWindow(0, 0, k_width, k_height);
        pickPhysicalDevice();
        createLogicalDevice();
        k_vk->vkGetDeviceQueue(m_vkDevice, m_compositorQueueFamilyIndex, 0,
                               &m_compositorVkQueue);
        k_vk->vkGetDeviceQueue(m_vkDevice, m_swapChainQueueFamilyIndex, 0,
                               &m_swapChainVkQueue);
        VkCommandPoolCreateInfo commandPoolCi = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = m_compositorQueueFamilyIndex};
        ASSERT_EQ(k_vk->vkCreateCommandPool(m_vkDevice, &commandPoolCi, nullptr,
                                            &m_vkCommandPool),
                  VK_SUCCESS);
        m_displayVk = std::make_unique<DisplayVk>(
            *k_vk, m_vkPhysicalDevice, m_swapChainQueueFamilyIndex,
            m_compositorQueueFamilyIndex, m_vkDevice, m_compositorVkQueue,
            m_swapChainVkQueue);
        m_displayVk->bindToSurface(m_vkSurface, k_width, k_height);
    }

    void TearDown() override {
        if (emugl::shouldUseWindow()) {
            ASSERT_EQ(k_vk->vkQueueWaitIdle(m_compositorVkQueue), VK_SUCCESS);
            ASSERT_EQ(k_vk->vkQueueWaitIdle(m_swapChainVkQueue), VK_SUCCESS);

            m_displayVk.reset();
            k_vk->vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
            k_vk->vkDestroyDevice(m_vkDevice, nullptr);
            k_vk->vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
            k_vk->vkDestroyInstance(m_vkInstance, nullptr);
        }
    }

    using RenderTexture = emugl::RenderTextureVk;

    static const goldfish_vk::VulkanDispatch *k_vk;
    static const uint32_t k_width = 0x100;
    static const uint32_t k_height = 0x100;

    OSWindow *m_window;
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    VkSurfaceKHR m_vkSurface = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    uint32_t m_swapChainQueueFamilyIndex = 0;
    uint32_t m_compositorQueueFamilyIndex = 0;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VkQueue m_compositorVkQueue = VK_NULL_HANDLE;
    VkQueue m_swapChainVkQueue = VK_NULL_HANDLE;
    VkCommandPool m_vkCommandPool = VK_NULL_HANDLE;
    std::unique_ptr<DisplayVk> m_displayVk = nullptr;

   private:
    void createInstance() {
        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "emulator SwapChainStateVk unittest",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_1};
        auto extensions = SwapChainStateVk::getRequiredInstanceExtensions();
        VkInstanceCreateInfo instanceCi = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()};
        ASSERT_EQ(k_vk->vkCreateInstance(&instanceCi, nullptr, &m_vkInstance),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkInstance != VK_NULL_HANDLE);
    }

    void createWindowAndSurface() {
        m_window = emugl::createOrGetTestWindow(0, 0, k_width, k_height);
        ASSERT_NE(m_window, nullptr);
        // TODO(kaiyili, b/179477624): add support for other platforms
#ifdef _WIN32
        VkWin32SurfaceCreateInfoKHR surfaceCi = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd = m_window->getNativeWindow()};
        ASSERT_EQ(k_vk->vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCi,
                                                nullptr, &m_vkSurface),
                  VK_SUCCESS);
#endif
    }

    void pickPhysicalDevice() {
        uint32_t physicalDeviceCount = 0;
        ASSERT_EQ(k_vk->vkEnumeratePhysicalDevices(
                      m_vkInstance, &physicalDeviceCount, nullptr),
                  VK_SUCCESS);
        ASSERT_GT(physicalDeviceCount, 0);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        ASSERT_EQ(
            k_vk->vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount,
                                             physicalDevices.data()),
            VK_SUCCESS);
        for (const auto &device : physicalDevices) {
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {
                .sType =
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
            VkPhysicalDeviceFeatures2 features = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &descIndexingFeatures};
            k_vk->vkGetPhysicalDeviceFeatures2(device, &features);
            if (!CompositorVk::validatePhysicalDeviceFeatures(features)) {
                continue;
            }
            uint32_t queueFamilyCount = 0;
            k_vk->vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, nullptr);
            ASSERT_GT(queueFamilyCount, 0);
            std::vector<VkQueueFamilyProperties> queueProps(queueFamilyCount);
            k_vk->vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, queueProps.data());
            std::optional<uint32_t> maybeSwapChainQueueFamilyIndex =
                std::nullopt;
            std::optional<uint32_t> maybeCompositorQueueFamilyIndex =
                std::nullopt;
            for (uint32_t queueFamilyIndex = 0;
                 queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
                if (!maybeSwapChainQueueFamilyIndex.has_value() &&
                    SwapChainStateVk::validateQueueFamilyProperties(
                        *k_vk, device, m_vkSurface, queueFamilyIndex) &&
                    SwapChainStateVk::createSwapChainCi(
                        *k_vk, m_vkSurface, device, k_width, k_height,
                        {queueFamilyIndex}) != nullptr) {
                    maybeSwapChainQueueFamilyIndex = queueFamilyIndex;
                }
                if (!maybeCompositorQueueFamilyIndex.has_value() &&
                    CompositorVk::validateQueueFamilyProperties(
                        queueProps[queueFamilyIndex])) {
                    maybeCompositorQueueFamilyIndex = queueFamilyIndex;
                }
            }
            if (!maybeSwapChainQueueFamilyIndex.has_value() ||
                !maybeCompositorQueueFamilyIndex.has_value()) {
                continue;
            }
            m_swapChainQueueFamilyIndex =
                maybeSwapChainQueueFamilyIndex.value();
            m_compositorQueueFamilyIndex =
                maybeCompositorQueueFamilyIndex.value();
            m_vkPhysicalDevice = device;
            return;
        }
        FAIL() << "Can't find a suitable VkPhysicalDevice.";
    }

    void createLogicalDevice() {
        const float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCis(0);
        for (auto queueFamilyIndex : std::unordered_set(
                 {m_swapChainQueueFamilyIndex, m_compositorQueueFamilyIndex})) {
            VkDeviceQueueCreateInfo queueCi = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority};
            queueCis.push_back(queueCi);
        }
        VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexingFeatures = {
            .sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
        VkPhysicalDeviceFeatures2 features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &descIndexingFeatures};
        ASSERT_TRUE(CompositorVk::enablePhysicalDeviceFeatures(features));
        auto extensions = SwapChainStateVk::getRequiredDeviceExtensions();
        const auto compositorExtensions =
            CompositorVk::getRequiredDeviceExtensions();
        extensions.insert(extensions.end(), compositorExtensions.begin(),
                          compositorExtensions.end());
        VkDeviceCreateInfo deviceCi = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &features,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCis.size()),
            .pQueueCreateInfos = queueCis.data(),
            .enabledLayerCount = 0,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
            .pEnabledFeatures = nullptr};
        ASSERT_EQ(k_vk->vkCreateDevice(m_vkPhysicalDevice, &deviceCi, nullptr,
                                       &m_vkDevice),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkDevice != VK_NULL_HANDLE);
    }
};

const goldfish_vk::VulkanDispatch *DisplayVkTest::k_vk = nullptr;

TEST_F(DisplayVkTest, Init) {}

TEST_F(DisplayVkTest, PostWithoutSurfaceShouldntCrash) {
    uint32_t textureWidth = 20;
    uint32_t textureHeight = 40;
    DisplayVk displayVk(*k_vk, m_vkPhysicalDevice, m_swapChainQueueFamilyIndex,
                        m_compositorQueueFamilyIndex, m_vkDevice,
                        m_compositorVkQueue, m_swapChainVkQueue);
    auto texture = RenderTexture::create(*k_vk, m_vkDevice, m_vkPhysicalDevice,
                                         m_compositorVkQueue, m_vkCommandPool,
                                         textureWidth, textureHeight);
    std::vector<uint32_t> pixels(textureWidth * textureHeight, 0);
    ASSERT_TRUE(texture->write(pixels));
    auto cbvk = displayVk.createDisplayBuffer(texture->m_vkImage,
                                              RenderTexture::k_vkFormat,
                                              textureWidth, textureHeight);
    displayVk.post(cbvk);
}

TEST_F(DisplayVkTest, SimplePost) {
    uint32_t textureWidth = 20;
    uint32_t textureHeight = 40;
    auto texture = RenderTexture::create(*k_vk, m_vkDevice, m_vkPhysicalDevice,
                                         m_compositorVkQueue, m_vkCommandPool,
                                         textureWidth, textureHeight);
    std::vector<uint32_t> pixels(textureWidth * textureHeight);
    for (int i = 0; i < textureHeight; i++) {
        for (int j = 0; j < textureWidth; j++) {
            uint8_t *pixel =
                reinterpret_cast<uint8_t *>(&pixels[i * textureWidth + j]);
            pixel[0] = static_cast<uint8_t>((i * 0xff / textureHeight) & 0xff);
            pixel[1] = static_cast<uint8_t>((j * 0xff / textureWidth) & 0xff);
            pixel[2] = 0;
            pixel[3] = 0xff;
        }
    }
    ASSERT_TRUE(texture->write(pixels));
    auto cbvk = m_displayVk->createDisplayBuffer(
        texture->m_vkImage, texture->k_vkFormat, texture->m_width,
        texture->m_height);
    for (uint32_t i = 0; i < 10; i++) {
        m_displayVk->post(cbvk);
    }
}

TEST_F(DisplayVkTest, PostTwoColorBuffers) {
    uint32_t textureWidth = 20;
    uint32_t textureHeight = 40;
    auto redTexture = RenderTexture::create(
        *k_vk, m_vkDevice, m_vkPhysicalDevice, m_compositorVkQueue,
        m_vkCommandPool, textureWidth, textureHeight);
    auto greenTexture = RenderTexture::create(
        *k_vk, m_vkDevice, m_vkPhysicalDevice, m_compositorVkQueue,
        m_vkCommandPool, textureWidth, textureHeight);
    uint32_t red = 0xff0000ff;
    uint32_t green = 0xff00ff00;
    std::vector<uint32_t> redPixels(textureWidth * textureHeight, red);
    std::vector<uint32_t> greenPixels(textureWidth * textureHeight, green);
    ASSERT_TRUE(redTexture->write(redPixels));
    ASSERT_TRUE(greenTexture->write(greenPixels));
    auto redCbvk = m_displayVk->createDisplayBuffer(
        redTexture->m_vkImage, redTexture->k_vkFormat, redTexture->m_width,
        redTexture->m_height);
    auto greenCbvk = m_displayVk->createDisplayBuffer(
        greenTexture->m_vkImage, greenTexture->k_vkFormat,
        greenTexture->m_width, greenTexture->m_height);
    for (uint32_t i = 0; i < 10; i++) {
        m_displayVk->post(redCbvk);
        m_displayVk->post(greenCbvk);
    }
}