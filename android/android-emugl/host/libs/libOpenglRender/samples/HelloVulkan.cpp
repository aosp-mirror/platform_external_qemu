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
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/system/System.h"

#include <sstream>

#include <vulkan/vulkan.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using android::base::arraySize;

#define ASSERT_VK_RESULT(expected, result) \
    if (expected != (result)) { fprintf(stderr, "%s: failed: %s\n", __func__, #result); abort(); }

namespace emugl {

struct VertexAttributes {
    float position[2];
    float texcoord[2];
};

static VertexAttributes vertexAttrs[] = {
    { { -1.0f, -1.0f,}, { 0.0, 0.0 } },
    { {  1.0f, -1.0f,}, { 1.0, 0.0 } },
    { { -1.0f,  1.0f,}, { 0.0, 1.0 } },
    { {  1.0f, -1.0f,}, { 1.0, 0.0 } },
    { {  1.0f,  1.0f,}, { 1.0, 1.0 } },
    { { -1.0f,  1.0f,}, { 0.0, 1.0 } },
};

class HelloVulkan : public SampleApplication {
protected:

    void initialize() override {
        constexpr char vshaderSrc[] = R"(#version 300 es
        precision highp float;

        layout (location = 0) in vec2 pos;
        layout (location = 1) in vec2 texcoord;

        out vec2 texcoord_varying;

        void main() {
            gl_Position = vec4(pos, 0.0, 1.0);
            texcoord_varying = texcoord;
        }
        )";

        constexpr char fshaderSrc[] = R"(#version 300 es
        precision highp float;

        uniform sampler2D tex;
        in vec2 texcoord_varying;
        out vec4 fragColor;

        void main() {
            fragColor = texture(tex, texcoord_varying);
        }
        )";

        GLint program = emugl::compileAndLinkShaderProgram(vshaderSrc, fshaderSrc);

        auto gl = getGlDispatch();

        mSamplerLoc = gl->glGetUniformLocation(program, "tex");

        gl->glEnableVertexAttribArray(0);
        gl->glEnableVertexAttribArray(1);

        gl->glActiveTexture(GL_TEXTURE0);
        gl->glGenTextures(1, &mTexture);
        gl->glBindTexture(GL_TEXTURE_2D, mTexture);
        gl->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, nullptr);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        gl->glGenBuffers(1, &mBuffer);
        gl->glBindBuffer(GL_ARRAY_BUFFER, mBuffer);

        gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertexAttrs), vertexAttrs,
                         GL_STATIC_DRAW);

        gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(VertexAttributes), 0);
        gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                                  sizeof(VertexAttributes),
                                  (GLvoid*)offsetof(VertexAttributes, texcoord));

        gl->glUseProgram(program);
        gl->glUniform1i(mSamplerLoc, 0);

        gl->glClearColor(0.2f, 0.2f, 0.3f, 0.0f);

        // Initialize Vulkan
        mVk = vkDispatch();

        auto vk = mVk;
        // creating instance
        std::vector<VkExtensionProperties> extProps;
        uint32_t extPropCount = 0;
        vk->vkEnumerateInstanceExtensionProperties(nullptr, &extPropCount,
                                                   nullptr);
        extProps.resize(extPropCount);
        vk->vkEnumerateInstanceExtensionProperties(nullptr, &extPropCount,
                                                   extProps.data());
        // look for swapchain in there
        bool hasSurfaceCapability = false;
        bool hasPlatformSurfaceCapability = false;
    #ifdef _WIN32
    #define PLATFORM_SURFACE_INSTANCE_EXT "VK_KHR_win32_surface"
    #else
    #ifdef __APPLE__
    #define PLATFORM_SURFACE_INSTANCE_EXT "VK_MVK_macos_surface"
    #else
    #define PLATFORM_SURFACE_INSTANCE_EXT "VK_KHR_xcb_surface"
    #endif
    #endif
        for (int i = 0; i < extProps.size(); i++) {
            const VkExtensionProperties& prop = extProps[i];
            if (!strcmp(prop.extensionName, "VK_KHR_surface")) {
                hasSurfaceCapability = true;
            }
            if (!strcmp(prop.extensionName, PLATFORM_SURFACE_INSTANCE_EXT)) {
                hasPlatformSurfaceCapability = true;
            }
        }
        if (!hasSurfaceCapability || !hasPlatformSurfaceCapability) {
            fprintf(stderr,
                    "%s: doesn't have needed surface capabilities; abort\n",
                    __func__);
            abort();
        }
        fprintf(stderr, "%s: instance ext query all good\n", __func__);
        VkApplicationInfo appInfo = {
            VK_STRUCTURE_TYPE_APPLICATION_INFO, 0,
            "HelloVulkan", VK_MAKE_VERSION(1, 0, 0),
            "HelloVulkanEngine", VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_0,
        };

        static const char* const wantedExts[] = {
            "VK_KHR_surface",
            PLATFORM_SURFACE_INSTANCE_EXT,
        };

        VkInstanceCreateInfo instanceCi = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, 0, 0,
            &appInfo,
            0, nullptr, /* no enabled layers */
            arraySize(wantedExts), wantedExts,
        };
        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkCreateInstance(&instanceCi, nullptr, &mInstance));
        // enumerate physical devices
        uint32_t physdevCount;
        std::vector<VkPhysicalDevice> physdevs;
        vk->vkEnumeratePhysicalDevices(mInstance, &physdevCount, nullptr);
        physdevs.resize(physdevCount);
        vk->vkEnumeratePhysicalDevices(mInstance, &physdevCount,
                                       physdevs.data());

        fprintf(stderr, "%s: found %u physical devices\n", __func__,
                physdevCount);

        bool foundWantedPhysicalDevice = false;
        uint32_t bestPhysicalDeviceIndex = 0;
        uint32_t transferQueueFamilyIndex = 0;
        uint32_t graphicsQueueFamilyIndex = 0;
        uint32_t transferQueueFamilyCount = 0;
        uint32_t graphicsQueueFamilyCount = 0;

        (void)graphicsQueueFamilyCount;
        (void)graphicsQueueFamilyIndex;
        (void)transferQueueFamilyCount;
        (void)transferQueueFamilyIndex;

        for (int i = 0; i < physdevCount; i++) {
            VkPhysicalDeviceProperties props;
            vk->vkGetPhysicalDeviceProperties(physdevs[i], &props);
            fprintf(stderr, "%s: physical device: %s\n", __func__,
                    props.deviceName);

            // Check device extensions;
            uint32_t devExtCount = 0;
            std::vector<VkExtensionProperties> devExts;
            vk->vkEnumerateDeviceExtensionProperties(physdevs[i], nullptr,
                                                     &devExtCount, nullptr);
            devExts.resize(devExtCount);
            vk->vkEnumerateDeviceExtensionProperties(
                    physdevs[i], nullptr, &devExtCount, devExts.data());

            bool hasSwapchainExt = false;
            bool extensionsGood = false;

            for (int j = 0; j < devExts.size(); j++) {
                fprintf(stderr, "%s: name: %s\n", __func__,
                        devExts[j].extensionName);
                if (!strcmp(devExts[j].extensionName, "VK_KHR_swapchain")) {
                    hasSwapchainExt = true;
                }
            }

            if (hasSwapchainExt) {
                extensionsGood = true;
            }

            uint32_t queueFamilyCount = 0;
            std::vector<VkQueueFamilyProperties> queueFamilyProps;
            vk->vkGetPhysicalDeviceQueueFamilyProperties(
                    physdevs[i], &queueFamilyCount, nullptr);
            queueFamilyProps.resize(queueFamilyCount);
            vk->vkGetPhysicalDeviceQueueFamilyProperties(
                    physdevs[i], &queueFamilyCount, queueFamilyProps.data());

            bool hasTransferQueueCapability = false;
            bool hasGraphicsQueueCapability = false;
            bool queuesGood = false;
            if (queueFamilyProps.size() == 0) {
                fprintf(stderr, "%s: welp, no queues\n", __func__);
                abort();
            } else {
                fprintf(stderr, "%s: queue family count: %zu\n", __func__,
                        queueFamilyProps.size());
            }
            for (int j = 0; j < queueFamilyProps.size(); j++) {
                const auto& props = queueFamilyProps[j];
                if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    fprintf(stderr, "%s: found graphics queue family at %d\n",
                            __func__, graphicsQueueFamilyIndex);
                    hasGraphicsQueueCapability = true;
                    graphicsQueueFamilyIndex = j;
                    graphicsQueueFamilyCount = props.queueCount;
                    hasTransferQueueCapability = true;
                    transferQueueFamilyIndex = j;
                    transferQueueFamilyCount = props.queueCount;
                }
                // Transfer is 'implied' by graphics according to the spec.
                // Keep this code around to show how to query
                // transfer queues in particular.
                // if ((props.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
                //     hasTransferQueueCapability = true;
                //     transferQueueFamilyIndex = j;
                //     transferQueueFamilyCount = props.queueCount;
                // }
                if (hasTransferQueueCapability && hasGraphicsQueueCapability) {
                    fprintf(stderr, "%s: queues check out\n", __func__);
                    queuesGood = true;
                }
            }
            if (queuesGood && extensionsGood) {
                foundWantedPhysicalDevice = true;
                bestPhysicalDeviceIndex = i;
                fprintf(stderr,
                        "%s: found suitable physical device. index: %d\n",
                        __func__, i);
                break;
            }
        }
        if (!foundWantedPhysicalDevice) {
            fprintf(stderr, "%s: FATAL: could not find a physical device\n",
                    __func__);
            abort();
        }
        mPhysicalDevice = physdevs[bestPhysicalDeviceIndex];

        static const char* const wantedDeviceExts[] = {
            "VK_KHR_swapchain",
        };

        if (graphicsQueueFamilyIndex == transferQueueFamilyIndex) {
            float priority = 1.0f;
            VkDeviceQueueCreateInfo graphicsDQCi = {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
                graphicsQueueFamilyIndex, 1, &priority,
            };

            VkDeviceCreateInfo dCi = {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 0, 0, 1,
                &graphicsDQCi, 0, nullptr,
                arraySize(wantedDeviceExts), wantedDeviceExts,
                nullptr, /* no features */
            };
            ASSERT_VK_RESULT(VK_SUCCESS,
                             vk->vkCreateDevice(mPhysicalDevice, &dCi, nullptr,
                                                &mDevice));
            vk->vkGetDeviceQueue(mDevice, graphicsQueueFamilyIndex, 0, &mQueue);
            mTransferQueue = mQueue;
        } else {
            float priority = 1.0f;
            VkDeviceQueueCreateInfo queueCis[] = {
                {
                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
                    graphicsQueueFamilyIndex, 1, &priority,
                },
                {
                    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 0, 0,
                    transferQueueFamilyIndex, 1, &priority,
                },
            };

            VkDeviceCreateInfo dCi = {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 0, 0,
                2, queueCis, 0, nullptr,
                arraySize(wantedDeviceExts), wantedDeviceExts,
                nullptr, /* no features */
            };
            ASSERT_VK_RESULT(VK_SUCCESS,
                             vk->vkCreateDevice(mPhysicalDevice, &dCi, nullptr,
                                                &mDevice));
            vk->vkGetDeviceQueue(mDevice, graphicsQueueFamilyIndex, 0, &mQueue);
            vk->vkGetDeviceQueue(mDevice, transferQueueFamilyIndex, 0,
                                 &mTransferQueue);
        }

        VkPhysicalDeviceMemoryProperties memProps;
        vk->vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProps);

        bool foundStaging = false;
        bool foundDevice = false;
        uint32_t bestStagingMemoryTypeIndex = 0;
        uint32_t bestDeviceMemoryTypeIndex = 0;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            const auto& type = memProps.memoryTypes[i];
            if (!foundStaging &&
                (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
                foundStaging = true;
                bestStagingMemoryTypeIndex = i;
                fprintf(stderr, "%s: best staging memory type index: %u\n",
                        __func__, bestStagingMemoryTypeIndex);
            }
            if (!foundDevice &&
                (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                foundDevice = true;
                bestDeviceMemoryTypeIndex = i;
                fprintf(stderr, "%s: best device memory type index: %u\n",
                        __func__, bestDeviceMemoryTypeIndex);
            }
        }

        VkMemoryAllocateInfo stagingAllocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
            kAllocSize, bestStagingMemoryTypeIndex,
        };

        VkMemoryAllocateInfo deviceAllocInfo = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 0,
            kAllocSize, bestDeviceMemoryTypeIndex,
        };

        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkAllocateMemory(mDevice, &stagingAllocInfo,
                                              nullptr, &mStagingMemory));
        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkAllocateMemory(mDevice, &deviceAllocInfo,
                                              nullptr, &mDeviceBufferMemory));
        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkAllocateMemory(mDevice, &deviceAllocInfo,
                                              nullptr, &mDeviceImageMemory));

        fprintf(stderr,
                "%s: allocated staging and device memory. time to map\n",
                __func__);

        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkMapMemory(mDevice, mStagingMemory, 0, kAllocSize,
                                         0, &mMappedStagingMemory));

        memset(mMappedStagingMemory, 0x0, kAllocSize);

        fprintf(stderr, "%s: successfully mapped memory\n", __func__);

        // set up shaders
        constexpr char kVkVShaderSrc[] = R"(
        #version 450
        #extension GL_ARB_separate_shader_objects : enable
        out gl_PerVertex {
            vec4 gl_Position;
        };
        layout (push_constant) uniform TransformPC {
            mat4 mvp;
        } transformPC;
        layout (std140, binding = 0) uniform Transform {
            mat4 mvp;
        } transform;
        layout (location = 0) in vec2 pos;
        layout (location = 1) in vec2 color;
        layout (location = 0) out vec2 color_varying;
        void main() {
            gl_Position = vec4(pos, 0.0, 1.0);
            color_varying = (transformPC.mvp * vec4(pos, 0.0, 1.0)).xy;
        }
        )";
        constexpr char kVkFShaderSrc[] = R"(
        #version 450
        #extension GL_ARB_separate_shader_objects : enable
        layout (location = 0) in vec2 color_varying;
        layout (location = 0) out vec4 fragcolor;
        void main() {
            fragcolor = vec4(color_varying.x, 0.2, color_varying.y, 1.0);
        }
        )";

        auto vertSpvBuff = compileSpirvFromGLSL("vert", kVkVShaderSrc);
        auto fragSpvBuff = compileSpirvFromGLSL("frag", kVkFShaderSrc);

        if (!vertSpvBuff || vertSpvBuff->size() == 0) {
            fprintf(stderr, "%s: failed to compile vert shader.\n", __func__);
            abort();
        }

        if (!fragSpvBuff || fragSpvBuff->size() == 0) {
            fprintf(stderr, "%s: failed to compile frag shader.\n", __func__);
            abort();
        }

        fprintf(stderr,
                "%s: compiled vert and frag shaders. creating shader modules\n",
                __func__);

        VkShaderModuleCreateInfo vertModuleCi = {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, 0, 0,
            vertSpvBuff->size(),
            (uint32_t*)vertSpvBuff->data(),
        };
        VkShaderModuleCreateInfo fragModuleCi = {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, 0, 0,
            fragSpvBuff->size(),
            (uint32_t*)fragSpvBuff->data(),
        };

        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkCreateShaderModule(mDevice, &vertModuleCi,
                                                  nullptr, &mVshaderModule));
        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkCreateShaderModule(mDevice, &fragModuleCi,
                                                  nullptr, &mFshaderModule));
        // Create staging buffer
        VkBufferCreateInfo stagingBufCi = {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
            (VkDeviceSize)kAllocSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_SHARING_MODE_EXCLUSIVE, 0, nullptr,
        };
        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkCreateBuffer(mDevice, &stagingBufCi,
                                                        nullptr, &mVkStagingBuffer));

        VkMemoryRequirements stagingBufMemReqs;
        vk->vkGetBufferMemoryRequirements(mDevice, mVkStagingBuffer,
                                          &stagingBufMemReqs);

        if (stagingBufMemReqs.size > kAllocSize) {
            fprintf(stderr,
                    "%s: fatal oops: staging buf actually required greater "
                    "size (0x%llx) "
                    "than expected (0x%llx)\n",
                    __func__, (unsigned long long)stagingBufMemReqs.size,
                    (unsigned long long)kAllocSize);
            abort();
        }

        if (stagingBufMemReqs.memoryTypeBits &
            (1 << bestStagingMemoryTypeIndex)) {
            fprintf(stderr,
                    "%s: staging buffer works wiht staging memory type index\n",
                    __func__);
        } else {
            abort();
        }

        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkBindBufferMemory(mDevice, mVkStagingBuffer,
                                                mStagingMemory, 0));
        // Create vertex buffer
        fprintf(stderr, "%s: vbo size: %zu\n", __func__, sizeof(vertexAttrs));

        VkBufferCreateInfo vboCi = {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 0, 0,
            (VkDeviceSize)sizeof(vertexAttrs),
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_SHARING_MODE_EXCLUSIVE, 0, nullptr,
        };

        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkCreateBuffer(mDevice, &vboCi, nullptr,
                                                        &mVkVertexBuffer));
#ifndef ROUND_UP
#define ROUND_UP(n, d) (((n) + (d) - 1) & -(0 ? (n) : (d)))
#endif
        VkDeviceSize currentDeviceMemoryOffset = 0;
        VkMemoryRequirements bufferMemReqs;
        vk->vkGetBufferMemoryRequirements(mDevice, mVkVertexBuffer, &bufferMemReqs);

        if (bufferMemReqs.memoryTypeBits & (1 << bestDeviceMemoryTypeIndex)) {
            fprintf(stderr, "%s: buffer usable with device memory type index\n",
                    __func__);
        } else {
            fprintf(stderr,
                    "%s: buffer NOT usable with device memory type index. "
                    "wanted to use index: %d type bits: 0x%x\n",
                    __func__, bestDeviceMemoryTypeIndex,
                    bufferMemReqs.memoryTypeBits);
            abort();
        }

        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkBindBufferMemory(mDevice, mVkVertexBuffer,
                                                mDeviceBufferMemory, 0));

        currentDeviceMemoryOffset +=
                ROUND_UP(bufferMemReqs.size, bufferMemReqs.alignment);

        fprintf(stderr,
                "%s: vertex buffer: size 0x%llx align 0x%llx range: [0x%llx "
                "0x%llx]\n",
                __func__,
                (unsigned long long)bufferMemReqs.size,
                (unsigned long long)bufferMemReqs.alignment,
                (unsigned long long)0x0,
                (unsigned long long)currentDeviceMemoryOffset);

        // Create image for testing
        VkImageCreateInfo imgCi = {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, 0, 0,
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            { 64, 64, 1 },
            1, 1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            VK_IMAGE_LAYOUT_UNDEFINED,
        };
        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkCreateImage(mDevice, &imgCi, nullptr,
                                                       &mVkImageToPaint));

        VkMemoryRequirements imageMemReqs;
        vk->vkGetImageMemoryRequirements(mDevice, mVkImageToPaint,
                                         &imageMemReqs);
        if (imageMemReqs.memoryTypeBits & (1 << bestDeviceMemoryTypeIndex)) {
            fprintf(stderr, "%s: image usable with device memory type index\n",
                    __func__);
        }

        fprintf(stderr, "%s: image: requires 0x%llx alignment\n", __func__,
                (unsigned long long)imageMemReqs.alignment);

        currentDeviceMemoryOffset = 0;
        ASSERT_VK_RESULT(
                VK_SUCCESS,
                vk->vkBindImageMemory(mDevice, mVkImageToPaint, mDeviceImageMemory,
                                      0));
        VkDeviceSize alignedImageSize = ROUND_UP(imageMemReqs.size, imageMemReqs.alignment);
        currentDeviceMemoryOffset += alignedImageSize;
        fprintf(stderr, "%s: image: occupies 0x%llx in range [0x%llx 0x%llx]\n",
                __func__, (unsigned long long)alignedImageSize,
                (unsigned long long)(currentDeviceMemoryOffset - alignedImageSize),
                (unsigned long long)currentDeviceMemoryOffset);

        // initialize descriptor pool
        VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
        VkDescriptorPoolCreateInfo dpCi = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0, 0,
            1 /* max sets */,
            1 /* pool sizes */, &poolSize,
        };
        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkCreateDescriptorPool(mDevice, &dpCi, nullptr,
                                                    &mDescriptorPool));

        // initialize descriptor set layout
        VkDescriptorSetLayoutBinding uniformBinding = {
            0 /* binding number */,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 /* # descriptors */,
            VK_SHADER_STAGE_VERTEX_BIT,
            nullptr, /* no immutable samplers */
        };
        VkDescriptorSetLayoutCreateInfo dslCi = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, 0, 0,
            1, &uniformBinding,
        };
        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkCreateDescriptorSetLayout(mDevice, &dslCi, nullptr,
                                                         &mDescriptorSetLayout));

        // Create push constant range and pipe
        VkPushConstantRange pushConstantRange = {
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            16 * sizeof(float) /* enough for our modelview matrix */,
        };

        VkPipelineLayoutCreateInfo pipelineLayoutCi = {
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, 0, 0,
            1, &mDescriptorSetLayout,
            1, &pushConstantRange,
        };

        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkCreatePipelineLayout(mDevice, &pipelineLayoutCi,
                                                    nullptr, &mPipelineLayout));

        VkDescriptorSetAllocateInfo dsai = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, 0,
            mDescriptorPool, 1, &mDescriptorSetLayout,
        };
        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkAllocateDescriptorSets(
                                             mDevice, &dsai, &mDescriptorSet));

        // Do the render pass.
        // 1. Create an image view for our color attachment
        VkImageViewCreateInfo imgViewCi = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 0, 0,
            mVkImageToPaint,
            VK_IMAGE_VIEW_TYPE_2D,
            VK_FORMAT_R8G8B8A8_UNORM,
            {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            {
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1, 0, 1,
            },
        };
        ASSERT_VK_RESULT(VK_SUCCESS,
                         vk->vkCreateImageView(mDevice, &imgViewCi, nullptr,
                                               &mVkImageToPaintView));

        // 2. Create render pass (attachment description, attachment reference,
        // subpass, render pass)
        VkAttachmentDescription colorDesc = {
            0 /* flags */,
            VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        };
        VkAttachmentReference colorRef = {
            0 /* index */,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        VkSubpassDescription colorSubpassDesc = {
            0 /* flags */,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            0, nullptr, /* no input attachments */
            1, /* color/depth attachemnt count */
            &colorRef, /* color attachment reference */
            nullptr, nullptr, /* no resolve or depthstencil attachent refs */
            0, nullptr, /* no preserve attachments */
        };
        VkRenderPassCreateInfo renderPassCi = {
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, 0, 0,
            1, &colorDesc,
            1, &colorSubpassDesc,
            0, nullptr, /* no subpass dependencies */
        };
        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkCreateRenderPass(mDevice, &renderPassCi,
                                                            nullptr, &mRenderPass));
        // 3. Create framebuffer
        VkFramebufferCreateInfo fbCi = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, 0, 0,
            mRenderPass,
            1, &mVkImageToPaintView, /* 1 attachment */
            64, 64, 1,
        };
        ASSERT_VK_RESULT(
                VK_SUCCESS,
                vk->vkCreateFramebuffer(mDevice, &fbCi, nullptr, &mVkFramebuffer));
        // Shader stages
        VkPipelineShaderStageCreateInfo shaderStageInfos[] = {
            {
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, 0, 0,
                VK_SHADER_STAGE_VERTEX_BIT, mVshaderModule, "main", nullptr,
            },
            {
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, 0, 0,
                VK_SHADER_STAGE_FRAGMENT_BIT, mFshaderModule, "main", nullptr,
            },
        };
        // Vertex input
        VkVertexInputBindingDescription vertexBindingDesc = {
            0 /* binding */,
            sizeof(VertexAttributes) /* stride */,
            VK_VERTEX_INPUT_RATE_VERTEX /* rate */,
        };
        VkVertexInputAttributeDescription vertexInputAttrs[] = {
            {
                0 /* location */, 0 /* binding */,
                VK_FORMAT_R32G32_SFLOAT /* format */,
                0 /* offset */,
            },
            {
                1 /* location */, 0 /* binding */,
                VK_FORMAT_R32G32_SFLOAT /* format */,
                offsetof(VertexAttributes, texcoord) /* offset */,
            },
        };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, 0, 0,
            1, &vertexBindingDesc,
            2, vertexInputAttrs,
        };
        // input assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, 0, 0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            VK_FALSE /* primitive restart disabled */,
        };
        // tessellation state: null
        // viewport
        VkViewport viewport = {
            0.0f, 0.0f, 64.0f, 64.0f,
            0.0f, 1.0f,
        };
        VkRect2D scissor = {
            { 0, 0, },
            { 64, 64 },
        };
        VkPipelineViewportStateCreateInfo viewportInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, 0, 0,
            1, &viewport,
            1, &scissor,
        };
        // rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizationInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, 0, 0,
            VK_FALSE /* enable depth clamp */,
            VK_FALSE /* enable rasterizer discard */,
            VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE,
            VK_FALSE /* disable depth bias */,
            0.0f, 0.0f, 0.0f, /* depth bias constant, clamp, slope */
            1.0f /* line width */,
        };
        // multisample state
        VkPipelineMultisampleStateCreateInfo multisampleInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, 0, 0,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FALSE /* no sample shading */,
            0.0f,
            nullptr,
            VK_FALSE /* alpha to coverage */,
            VK_FALSE /* alpha to 1 */,
        };
        // depth stencil state
        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, 0, 0,
            VK_FALSE, VK_FALSE, /* disable depth test and depth write */
            VK_COMPARE_OP_LESS, /* depth test function */
            VK_FALSE, /* disable depth bounds test */
            VK_FALSE, /* disable stencil test */
            /* front/back stencil ops */
            { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
              VK_COMPARE_OP_EQUAL, 0x0, 0x0, 0x0 },
            { VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
              VK_COMPARE_OP_EQUAL, 0x0, 0x0, 0x0 },
            0.0f, 1.0f, /* depth bounds test's bounds */
        };
        // color blend state
        VkPipelineColorBlendAttachmentState colorAttachmentBlendState = {
            VK_FALSE /* blending disabled */,
            VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD,
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, 0, 0,
            VK_FALSE, VK_LOGIC_OP_CLEAR, /* disable logic op, use clear op */
            1, &colorAttachmentBlendState,
            { 1.0f, 1.0f, 1.0f, 1.0f },
        };
        // dynamic state - skip
        // create graphics pipeline here
        VkGraphicsPipelineCreateInfo graphicsPipelineCi = {
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, 0, 0,
            2, shaderStageInfos,
            &vertexInputInfo,
            &inputAssemblyInfo,
            nullptr /* no tessellation info */,
            &viewportInfo,
            &rasterizationInfo,
            &multisampleInfo,
            &depthStencilInfo,
            &colorBlendInfo,
            nullptr /* no dynamic state */,
            mPipelineLayout,
            mRenderPass,
            0 /* first subpass? */,
            nullptr, 0, /* no base pipeline */
        };

        auto res = vk->vkCreateGraphicsPipelines(
                mDevice, nullptr /* no cache */, 1, &graphicsPipelineCi,
                nullptr, &mPipeline);

        if (res == VK_SUCCESS) {
            fprintf(stderr, "%s: created graphics pipeline\n", __func__);
        } else {
            fprintf(stderr,
                    "%s: could not create graphics pipeline. error 0x%x\n",
                    __func__, res);
        }

        // Initialize command pool and command buffers
        VkCommandPoolCreateInfo commandPoolCi_graphics = {
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            graphicsQueueFamilyIndex,
        };

        VkCommandPoolCreateInfo commandPoolCi_transfer = {
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, 0, 0,
            transferQueueFamilyIndex,
        };

        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkCreateCommandPool(
                                             mDevice, &commandPoolCi_graphics,
                                             nullptr, &mCommandPool_graphics));
        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkCreateCommandPool(
                                             mDevice, &commandPoolCi_transfer,
                                             nullptr, &mCommandPool_transfer));

        VkCommandBufferAllocateInfo commandBufferCi_graphics = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
            mCommandPool_graphics, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
        };

        VkCommandBufferAllocateInfo commandBufferCi_transfer = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 0,
            mCommandPool_transfer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1,
        };

        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkAllocateCommandBuffers(
                                             mDevice, &commandBufferCi_graphics,
                                             &mCommandBuffer_graphics));
        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkAllocateCommandBuffers(
                                             mDevice, &commandBufferCi_transfer,
                                             &mCommandBuffer_transfer));

        fprintf(stderr, "%s: created command buffers\n", __func__);

        // Transfer vertex attributes
        memcpy(mMappedStagingMemory, vertexAttrs, sizeof(vertexAttrs));
        VkMappedMemoryRange range = {
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0, mStagingMemory,
            0, sizeof(vertexAttrs),
        };
        vk->vkFlushMappedMemoryRanges(mDevice, 1, &range);

        VkCommandBufferBeginInfo beginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            nullptr,
        };

        vk->vkBeginCommandBuffer(mCommandBuffer_transfer, &beginInfo);
        VkBufferCopy region = {
            0, 0, sizeof(vertexAttrs),
        };
        vk->vkCmdCopyBuffer(mCommandBuffer_transfer, mVkStagingBuffer,
                            mVkVertexBuffer, 1, &region);
        vk->vkEndCommandBuffer(mCommandBuffer_transfer);

        VkSubmitInfo vboCopySi = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
            0, nullptr, nullptr, /* no wait semaphores */
            1, &mCommandBuffer_transfer, /* 1 command buffer */
            0, nullptr, /* no signal semaphores */
        };

        VkFenceCreateInfo fenceCi = {
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 0,
        };
        vk->vkCreateFence(mDevice, &fenceCi, nullptr, &mFence);

        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkQueueSubmit(mTransferQueue, 1,
                                                       &vboCopySi, mFence));

        uint64_t secondsToNanos = 1000ULL * 1000ULL * 1000ULL;
        ASSERT_VK_RESULT(
                VK_SUCCESS,
                vk->vkWaitForFences(mDevice, 1, &mFence, VK_TRUE /* wait all */,
                                    5ULL * secondsToNanos));
        vk->vkResetFences(mDevice, 1, &mFence);

        fprintf(stderr, "%s: uploaded vertex data\n", __func__);
    }

    void draw() override {
        auto vk = mVk;

        // Vk part: repeatedly:
        // 1. render to the color attachment with modified rotation
        // 2. copy the data back out to the staging buffer

        VkCommandBufferBeginInfo beginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 0,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            nullptr,
        };
        vk->vkBeginCommandBuffer(mCommandBuffer_graphics, &beginInfo);

        vk->vkCmdBindPipeline(mCommandBuffer_graphics,
                              VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

        VkClearColorValue clearColorValue;
        clearColorValue.float32[0] = 0.2f;
        clearColorValue.float32[1] = 0.4f;
        clearColorValue.float32[2] = 0.6f;
        clearColorValue.float32[3] = 0.0f;

        VkClearValue clearValue = {
            clearColorValue,
        };

        VkRenderPassBeginInfo rpBi = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, 0,
            mRenderPass,
            mVkFramebuffer,
            {{0, 0}, {64, 64}},
            1,
            &clearValue };
        vk->vkCmdBeginRenderPass(mCommandBuffer_graphics, &rpBi, VK_SUBPASS_CONTENTS_INLINE);

        glm::mat4 rot = 
            glm::rotate(glm::mat4(), mTime, glm::vec3(0.0f, 0.0f, 1.0f));

        vk->vkCmdPushConstants(mCommandBuffer_graphics, mPipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT, 0,
                               16 * sizeof(float), glm::value_ptr(rot));

        VkDeviceSize vboOffset = 0;
        vk->vkCmdBindVertexBuffers(mCommandBuffer_graphics, 0, 1,
                                   &mVkVertexBuffer, &vboOffset);
        // vk->vkCmdBindDescriptorSets(
        //         mCommandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS,
        //         mPipelineLayout, 0, 1, &mDescriptorSet, 0, nullptr);
        vk->vkCmdDraw(mCommandBuffer_graphics, 6, 1, 0, 0);

        vk->vkCmdEndRenderPass(mCommandBuffer_graphics);

        // image should be transferred to TRANSFER_SRC_OPTIMAL at end of render pass,
        // so no need for an explicit barrier
        VkBufferImageCopy imgCopy = {
            0, 64, 64,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            { 0, 0, 0 },
            { 64, 64, 1 },
        };
        vk->vkCmdCopyImageToBuffer(mCommandBuffer_graphics, mVkImageToPaint,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   mVkStagingBuffer, 1, &imgCopy);

        vk->vkEndCommandBuffer(mCommandBuffer_graphics);

        VkSubmitInfo drawSi = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, 0,
            0, nullptr, nullptr, /* no wait semaphores */
            1, &mCommandBuffer_graphics, /* 1 command buffer */
            0, nullptr, /* no signal semaphores */
        };
        ASSERT_VK_RESULT(VK_SUCCESS, vk->vkQueueSubmit(mQueue, 1, &drawSi, mFence));

        uint64_t secondsToNanos = 1000ULL * 1000ULL * 1000ULL;
        ASSERT_VK_RESULT(
                VK_SUCCESS,
                vk->vkWaitForFences(mDevice, 1, &mFence, VK_TRUE /* wait all */,
                                    5ULL * secondsToNanos));
        vk->vkResetFences(mDevice, 1, &mFence);
        VkMappedMemoryRange invalidateRange = {
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, 0,
            mStagingMemory, 0, 64 * 64 * 4,
        };
        vk->vkInvalidateMappedMemoryRanges(mDevice, 1, &invalidateRange);

        // GL part
        {
            auto gl = getGlDispatch();
            gl->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGBA,
                                GL_UNSIGNED_BYTE, mMappedStagingMemory);
            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            gl->glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        mTime += 0.05f;
    }

    float mTime = 0.0f;

    GLint mSamplerLoc;

    GLuint mTexture;
    GLuint mBuffer;

    VulkanDispatch* mVk = nullptr;

    VkInstance mInstance = nullptr;
    VkPhysicalDevice mPhysicalDevice = nullptr;
    VkDevice mDevice = nullptr;
    VkQueue mQueue = nullptr;
    VkQueue mTransferQueue = nullptr;

    static constexpr VkDeviceSize kAllocSize =
            64 * 1024;  // 64 kb is much more than enough.

    VkDeviceMemory mStagingMemory = nullptr;
    void* mMappedStagingMemory = nullptr;
    VkDeviceMemory mDeviceBufferMemory = nullptr;
    VkDeviceMemory mDeviceImageMemory = nullptr;

    VkShaderModule mVshaderModule;
    VkShaderModule mFshaderModule;

    VkBuffer mVkStagingBuffer;
    VkBuffer mVkVertexBuffer;
    VkImage mVkImageToPaint;
    VkImageView mVkImageToPaintView;

    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorSet mDescriptorSet;

    VkRenderPass mRenderPass;
    VkFramebuffer mVkFramebuffer;

    VkPipelineLayout  mPipelineLayout;
    VkPipeline  mPipeline;

    VkCommandPool mCommandPool_graphics;
    VkCommandBuffer mCommandBuffer_graphics;

    VkCommandPool mCommandPool_transfer;
    VkCommandBuffer mCommandBuffer_transfer;

    VkFence mFence;
};

} // namespace emugl

int main(int argc, char** argv) {
    emugl::HelloVulkan app;
    app.drawLoop();
    return 0;
}