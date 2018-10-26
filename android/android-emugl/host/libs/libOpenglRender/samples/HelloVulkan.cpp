// Copyright (C) 2018 The Android Open Source Project
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

#include "Standalone.h"

#include "VulkanDispatch.h"

#include "android/base/ArraySize.h"

#include <sstream>

#include <vulkan/vulkan.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using android::base::arraySize;

#define ASSERT_VK_RESULT(expected, result) \
    if (expected != (result)) abort();

namespace emugl {

static std::string deviceTypeToString(VkPhysicalDeviceType type) {
#define DO_ENUM_RETURN_STRING(e) \
    case e: \
        return #e; \

    switch (type) {
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_OTHER)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
    DO_ENUM_RETURN_STRING(VK_PHYSICAL_DEVICE_TYPE_CPU)
        default:
            return "Unknown";
    }
}

static std::string queueFlagsToString(VkQueueFlags queueFlags) {
    std::stringstream ss;

    if (queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        ss << "VK_QUEUE_GRAPHICS_BIT | ";
    }
    if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
        ss << "VK_QUEUE_COMPUTE_BIT | ";
    }
    if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
        ss << "VK_QUEUE_TRANSFER_BIT | ";
    }
    if (queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
        ss << "VK_QUEUE_SPARSE_BINDING_BIT | ";
    }
    if (queueFlags & VK_QUEUE_PROTECTED_BIT) {
        ss << "VK_QUEUE_PROTECTED_BIT";
    }

    return ss.str();
}

static std::string getPhysicalDevicePropertiesString(
    const goldfish_vk::VulkanDispatch* vk,
    VkPhysicalDevice physicalDevice,
    const VkPhysicalDeviceProperties& props) {

    std::stringstream ss;

    uint16_t apiMaj = (uint16_t)(props.apiVersion >> 22);
    uint16_t apiMin = (uint16_t)(0x000003ff & (props.apiVersion >> 12));
    uint16_t apiPatch = (uint16_t)(0x000007ff & (props.apiVersion));

    ss << "API version: " << apiMaj << "." << apiMin << "." << apiPatch << "\n";
    ss << "Driver version: " << std::hex << props.driverVersion << "\n";
    ss << "Vendor ID: " << std::hex << props.vendorID << "\n";
    ss << "Device ID: " << std::hex << props.deviceID << "\n";
    ss << "Device type: " << deviceTypeToString(props.deviceType) << "\n";
    ss << "Device name: " << props.deviceName << "\n";

    uint32_t deviceExtensionCount;
    std::vector<VkExtensionProperties> deviceExtensionProperties;
    vk->vkEnumerateDeviceExtensionProperties(
        physicalDevice,
        nullptr,
        &deviceExtensionCount,
        nullptr);

    deviceExtensionProperties.resize(deviceExtensionCount);
    vk->vkEnumerateDeviceExtensionProperties(
        physicalDevice,
        nullptr,
        &deviceExtensionCount,
        deviceExtensionProperties.data());

    for (uint32_t i = 0; i < deviceExtensionCount; ++i) {
        ss << "Device extension: " <<
            deviceExtensionProperties[i].extensionName << "\n";
    }

    return ss.str();
}

static void testInstanceCreation(const VulkanDispatch* vk,
                                 VkInstance* instance_out) {
    fprintf(stderr, "%s: getting exts %p\n", __func__, vk->vkEnumerateInstanceExtensionProperties);

    uint32_t count;
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

    fprintf(stderr, "%s: exts: %u\n", __func__, count);

    std::vector<VkExtensionProperties> props(count);
    vk->vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());

    for (uint32_t i = 0; i < count; i++) {
        fprintf(stderr, "%s: ext: %s\n", __func__, props[i].extensionName);
    }

    // Query the correct surface extensions, which we'll need.

    const char* exts[] = {
        "VK_KHR_surface",
#ifdef _WIN32
#define PLATFORM_SURFACE_INSTANCE_EXT "VK_KHR_win32_surface"
#else
#ifdef __APPLE__
#define PLATFORM_SURFACE_INSTANCE_EXT "VK_MVK_macos_surface"
#else
#define PLATFORM_SURFACE_INSTANCE_EXT "VK_KHR_xlib_surface"
#endif
#endif
        PLATFORM_SURFACE_INSTANCE_EXT,
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
        nullptr,
        0, nullptr,
        arraySize(exts), exts,
    };

    VkInstance instance;

    ASSERT_VK_RESULT(VK_SUCCESS,
        vk->vkCreateInstance(
            &instanceCreateInfo, nullptr, &instance));

    *instance_out = instance;
}

static void testDeviceCreation(const VulkanDispatch* vk,
                               VkInstance instance,
                               VkPhysicalDevice* physDevice_out,
                               VkDevice* device_out) {

    fprintf(stderr, "%s: call\n", __func__);

    uint32_t physicalDeviceCount;
    std::vector<VkPhysicalDevice> physicalDevices;

    ASSERT_VK_RESULT(VK_SUCCESS,
        vk->vkEnumeratePhysicalDevices(
            instance, &physicalDeviceCount, nullptr));

    physicalDevices.resize(physicalDeviceCount);

    ASSERT_VK_RESULT(VK_SUCCESS,
        vk->vkEnumeratePhysicalDevices(
            instance, &physicalDeviceCount, physicalDevices.data()));

    std::vector<VkPhysicalDeviceProperties> physicalDeviceProps(physicalDeviceCount);

    // at the end of the day, we need to pick a physical device.
    // Pick one that has graphics + compute if possible, otherwise settle for a device
    // that has at least one queue family capable of graphics.
    // TODO: Pick the device that has present capability for that queue if
    // we are not running in no-window mode.

    bool bestPhysicalDeviceFound = false;
    uint32_t bestPhysicalDeviceIndex = 0;

    std::vector<uint32_t> physDevsWithBothGraphicsAndCompute;
    std::vector<uint32_t> physDevsWithGraphicsOnly;

    for (uint32_t i = 0; i < physicalDeviceCount; i++) {
        uint32_t deviceExtensionCount;
        std::vector<VkExtensionProperties> deviceExtensionProperties;
        vk->vkEnumerateDeviceExtensionProperties(
            physicalDevices[i],
            nullptr,
            &deviceExtensionCount,
            nullptr);

        deviceExtensionProperties.resize(deviceExtensionCount);
        vk->vkEnumerateDeviceExtensionProperties(
            physicalDevices[i],
            nullptr,
            &deviceExtensionCount,
            deviceExtensionProperties.data());

        bool hasSwapchainExtension = false;

        fprintf(stderr, "%s: check swapchain ext\n", __func__);
        for (uint32_t j = 0; j < deviceExtensionCount; j++) {
            std::string ext = deviceExtensionProperties[j].extensionName;
            if (ext == "VK_KHR_swapchain") {
                hasSwapchainExtension = true;
            }
        }

        if (!hasSwapchainExtension) continue;

        vk->vkGetPhysicalDeviceProperties(
            physicalDevices[i],
            physicalDeviceProps.data() + i);

        auto str = getPhysicalDevicePropertiesString(vk, physicalDevices[i], physicalDeviceProps[i]);
        fprintf(stderr, "device %u: %s\n", i, str.c_str());

        uint32_t queueFamilyCount;
        vk->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vk->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, queueFamilies.data());

        bool hasGraphicsQueue = false;
        bool hasComputeQueue = false;

        for (uint32_t j = 0; j < queueFamilyCount; j++) {
            if (queueFamilies[j].queueCount > 0) {

                auto flags = queueFamilies[j].queueFlags;
                auto flagsAsString =
                    queueFlagsToString(flags);

                fprintf(stderr, "%s: found %u @ family %u with caps: %s\n",
                        __func__,
                        queueFamilies[j].queueCount, j,
                        flagsAsString.c_str());

                if ((flags & VK_QUEUE_GRAPHICS_BIT) &&
                    (flags & VK_QUEUE_COMPUTE_BIT)) {
                    hasGraphicsQueue = true;
                    hasComputeQueue = true;
                    bestPhysicalDeviceFound = true;
                    break;
                }

                if (flags & VK_QUEUE_GRAPHICS_BIT) {
                    hasGraphicsQueue = true;
                    bestPhysicalDeviceFound = true;
                }

                if (flags & VK_QUEUE_COMPUTE_BIT) {
                    hasComputeQueue = true;
                    bestPhysicalDeviceFound = true;
                }
            }
        }

        if (hasGraphicsQueue && hasComputeQueue) {
            physDevsWithBothGraphicsAndCompute.push_back(i);
            break;
        }

        if (hasGraphicsQueue) {
            physDevsWithGraphicsOnly.push_back(i);
        }
    }

    if (physDevsWithBothGraphicsAndCompute.size() > 0) {
        bestPhysicalDeviceIndex = physDevsWithBothGraphicsAndCompute[0];
    } else if (physDevsWithGraphicsOnly.size() > 0) {
        bestPhysicalDeviceIndex = physDevsWithGraphicsOnly[0];
    } else {
        fprintf(stderr, "%s: Could not find best physical device\n", __func__);
        abort();
        return;
    } 

    // Now we got our device; select it
    VkPhysicalDevice bestPhysicalDevice = physicalDevices[bestPhysicalDeviceIndex];

    uint32_t queueFamilyCount;
    vk->vkGetPhysicalDeviceQueueFamilyProperties(
        bestPhysicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vk->vkGetPhysicalDeviceQueueFamilyProperties(
        bestPhysicalDevice, &queueFamilyCount, queueFamilies.data());

    std::vector<uint32_t> wantedQueueFamilies;
    std::vector<uint32_t> wantedQueueFamilyCounts;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueCount > 0) {
            auto flags = queueFamilies[i].queueFlags;
            if ((flags & VK_QUEUE_GRAPHICS_BIT) &&
                    (flags & VK_QUEUE_COMPUTE_BIT)) {
                wantedQueueFamilies.push_back(i);
                wantedQueueFamilyCounts.push_back(queueFamilies[i].queueCount);
                break;
            }

            if ((flags & VK_QUEUE_GRAPHICS_BIT) ||
                    (flags & VK_QUEUE_COMPUTE_BIT)) {
                wantedQueueFamilies.push_back(i);
                wantedQueueFamilyCounts.push_back(queueFamilies[i].queueCount);
            }
        }
    }

    std::vector<VkDeviceQueueCreateInfo> queueCis;

    for (uint32_t i = 0; i < wantedQueueFamilies.size(); ++i) {
        auto familyIndex = wantedQueueFamilies[i];
        auto queueCount = wantedQueueFamilyCounts[i];

        std::vector<float> priorities;

        for (uint32_t j = 0; j < queueCount; ++j) {
            priorities.push_back(1.0f);
        }

        VkDeviceQueueCreateInfo dqci = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
            familyIndex,
            queueCount,
            priorities.data(),
        };

        queueCis.push_back(dqci);
    }

    const char* exts[] = {
        "VK_KHR_swapchain",
        PLATFORM_SURFACE_INSTANCE_EXT,
    };

    VkDeviceCreateInfo ci = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 0, 0,
        (uint32_t)queueCis.size(),
        queueCis.data(),
        0, nullptr,
        arraySize(exts), exts,
        nullptr,
    };

    VkDevice device;
    ASSERT_VK_RESULT(VK_SUCCESS,
        vk->vkCreateDevice(bestPhysicalDevice, &ci, nullptr, &device));

    *physDevice_out = bestPhysicalDevice;
    *device_out = device;
}

static void teardownVulkanTest(const VulkanDispatch* vk,
                               VkDevice dev,
                               VkInstance instance) {
    vk->vkDestroyDevice(dev, nullptr);
    vk->vkDestroyInstance(instance, nullptr);
}



class HelloVulkan : public SampleApplication {
protected:
    struct VertexAttributes {
        float position[2];
        float color[3];
    };

    void initialize() override {
        constexpr char vshaderSrc[] = R"(#version 300 es
        precision highp float;

        layout (location = 0) in vec2 pos;
        layout (location = 1) in vec3 color;

        uniform mat4 transform;

        out vec3 color_varying;

        void main() {
            gl_Position = transform * vec4(pos, 0.0, 1.0);
            color_varying = (transform * vec4(color, 1.0)).xyz;
        }
        )";
        constexpr char fshaderSrc[] = R"(#version 300 es
        precision highp float;

        in vec3 color_varying;

        out vec4 fragColor;

        void main() {
            fragColor = vec4(color_varying, 1.0);
        }
        )";

        GLint program = emugl::compileAndLinkShaderProgram(vshaderSrc, fshaderSrc);

        auto gl = getGlDispatch();

        mTransformLoc = gl->glGetUniformLocation(program, "transform");

        gl->glEnableVertexAttribArray(0);
        gl->glEnableVertexAttribArray(1);

        const VertexAttributes vertexAttrs[] = {
            { { -0.5f, -0.5f,}, { 0.2, 0.1, 0.9, }, },
            { { 0.5f, -0.5f,}, { 0.8, 0.3, 0.1,}, },
            { { 0.0f, 0.5f,}, { 0.1, 0.9, 0.6,}, },
        };

        gl->glGenBuffers(1, &mBuffer);
        gl->glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
        gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs), vertexAttrs,
                         GL_STATIC_DRAW);

        gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(VertexAttributes), 0);
        gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(VertexAttributes),
                                  (GLvoid*)offsetof(VertexAttributes, color));

        gl->glUseProgram(program);

        gl->glClearColor(0.2f, 0.2f, 0.3f, 0.0f);

        // Initialize Vulkan
        mVk = vkDispatch();
        testInstanceCreation(mVk, &mInstance);
        testDeviceCreation(mVk, mInstance, &mPhysicalDevice, &mDevice);
    }

    void draw() override {
        glm::mat4 rot =
                glm::rotate(glm::mat4(), mTime, glm::vec3(0.0f, 0.0f, 1.0f));
    
        auto gl = getGlDispatch();
    
        gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl->glUniformMatrix4fv(mTransformLoc, 1, GL_FALSE, glm::value_ptr(rot));
        gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    
        mTime += 0.05f;
    }

    GLint mTransformLoc;
    GLuint mBuffer;
    float mTime = 0.0f;

    VulkanDispatch* mVk = nullptr;

    VkInstance mInstance = nullptr;
    VkPhysicalDevice mPhysicalDevice = nullptr;
    VkDevice mDevice = nullptr;
};

} // namespace emugl

int main(int argc, char** argv) {
    emugl::HelloVulkan app;
    app.drawLoop();
    return 0;
}