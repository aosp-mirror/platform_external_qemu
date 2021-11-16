// Copyright (C) 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "SwapChainStateVk.h"

#include <gtest/gtest.h>

#include "Standalone.h"
#include "vulkan/VulkanDispatch.h"

class SwapChainStateVkTest : public ::testing::Test {
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
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void TearDown() override {
        if (emugl::shouldUseWindow()) {
            k_vk->vkDestroyDevice(m_vkDevice, nullptr);
            k_vk->vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
            k_vk->vkDestroyInstance(m_vkInstance, nullptr);
        }
    }

    static goldfish_vk::VulkanDispatch *k_vk;
    static const uint32_t k_width = 0x100;
    static const uint32_t k_height = 0x100;

    OSWindow *m_window;
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    VkSurfaceKHR m_vkSurface = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    uint32_t m_swapChainQueueFamilyIndex = 0;
    VkDevice m_vkDevice = VK_NULL_HANDLE;

   private:
    void createInstance() {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = "emulator SwapChainStateVk unittest";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;
        auto extensions = SwapChainStateVk::getRequiredInstanceExtensions();
        VkInstanceCreateInfo instanceCi = {};
        instanceCi.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCi.ppEnabledExtensionNames = extensions.data();
        instanceCi.enabledExtensionCount =
            static_cast<uint32_t>(extensions.size());
        instanceCi.pApplicationInfo = &appInfo;
        ASSERT_EQ(k_vk->vkCreateInstance(&instanceCi, nullptr, &m_vkInstance),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkInstance != VK_NULL_HANDLE);
    }

    void createWindowAndSurface() {
        m_window = emugl::createOrGetTestWindow(0, 0, k_width, k_height);
        ASSERT_NE(m_window, nullptr);
        // TODO(kaiyili, b/179477624): add support for other platforms
#if defined(_WIN32)
        VkWin32SurfaceCreateInfoKHR surfaceCi = {};
        surfaceCi.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCi.hinstance = GetModuleHandle(nullptr);
        surfaceCi.hwnd = m_window->getNativeWindow();
        ASSERT_EQ(k_vk->vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCi,
                                                nullptr, &m_vkSurface),
                  VK_SUCCESS);
#elif defined(__linux__)
        VkXcbSurfaceCreateInfoKHR surfaceCi = {
            .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = {},
            .connection = reinterpret_cast<xcb_connection_t*>(
                m_window->getNativeDisplayConnection()),
            .window = static_cast<xcb_window_t>(m_window->getNativeWindow()),
        };
        ASSERT_EQ(k_vk->vkCreateXcbSurfaceKHR(m_vkInstance, &surfaceCi,
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
            uint32_t queueFamilyCount = 0;
            k_vk->vkGetPhysicalDeviceQueueFamilyProperties(
                device, &queueFamilyCount, nullptr);
            ASSERT_GT(queueFamilyCount, 0);
            uint32_t queueFamilyIndex = 0;
            for (; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
                if (!SwapChainStateVk::validateQueueFamilyProperties(
                        *k_vk, device, m_vkSurface, queueFamilyIndex)) {
                    continue;
                }
                if (SwapChainStateVk::createSwapChainCi(
                        *k_vk, m_vkSurface, device, k_width, k_height,
                        {queueFamilyIndex}) == nullptr) {
                    continue;
                }
                break;
            }
            if (queueFamilyIndex == queueFamilyCount) {
                continue;
            }

            m_swapChainQueueFamilyIndex = queueFamilyIndex;
            m_vkPhysicalDevice = device;
            return;
        }
        FAIL() << "Can't find a suitable VkPhysicalDevice.";
    }

    void createLogicalDevice() {
        const float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCi = {};
        queueCi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCi.queueFamilyIndex = m_swapChainQueueFamilyIndex;
        queueCi.queueCount = 1;
        queueCi.pQueuePriorities = &queuePriority;
        VkPhysicalDeviceFeatures features = {};
        const std::vector<const char *> enabledDeviceExtensions =
            SwapChainStateVk::getRequiredDeviceExtensions();
        VkDeviceCreateInfo deviceCi = {};
        deviceCi.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCi.pQueueCreateInfos = &queueCi;
        deviceCi.queueCreateInfoCount = 1;
        deviceCi.pEnabledFeatures = &features;
        deviceCi.enabledLayerCount = 0;
        deviceCi.enabledExtensionCount =
            static_cast<uint32_t>(enabledDeviceExtensions.size());
        deviceCi.ppEnabledExtensionNames = enabledDeviceExtensions.data();
        ASSERT_EQ(k_vk->vkCreateDevice(m_vkPhysicalDevice, &deviceCi, nullptr,
                                       &m_vkDevice),
                  VK_SUCCESS);
        ASSERT_TRUE(m_vkDevice != VK_NULL_HANDLE);
    }
};

goldfish_vk::VulkanDispatch *SwapChainStateVkTest::k_vk = nullptr;

TEST_F(SwapChainStateVkTest, init) {
    auto swapChainCi = SwapChainStateVk::createSwapChainCi(
        *k_vk, m_vkSurface, m_vkPhysicalDevice, k_width, k_height,
        {m_swapChainQueueFamilyIndex});
    ASSERT_NE(swapChainCi, nullptr);
    SwapChainStateVk swapChainState(*k_vk, m_vkDevice, *swapChainCi);
}