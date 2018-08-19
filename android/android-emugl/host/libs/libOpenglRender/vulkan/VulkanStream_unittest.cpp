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
#include "VulkanStream.h"

#include "common/goldfish_vk_marshaling.h"

#include "android/base/ArraySize.h"

#include <gtest/gtest.h>
#include <string.h>
#include <vulkan.h>

using android::base::arraySize;

namespace goldfish_vk {

class VulkanStreamForTesting : public VulkanStream {
public:
    ssize_t read(void* buffer, size_t size) override {
        EXPECT_LE(mReadCursor + size, mBuffer.size());
        memcpy(buffer, mBuffer.data() + mReadCursor, size);

        mReadCursor += size;

        if (mReadCursor == mWriteCursor) {
            clear();
        }
        return size;
    }

    ssize_t write(const void* buffer, size_t size) override {
        if (mBuffer.size() < mWriteCursor + size) {
            mBuffer.resize(mWriteCursor + size);
        }

        memcpy(mBuffer.data() + mWriteCursor, buffer, size);

        mWriteCursor += size;

        if (mReadCursor == mWriteCursor) {
            clear();
        }
        return size;
    }

private:
    void clear() {
        mBuffer.clear();
        mReadCursor = 0;
        mWriteCursor = 0;
    }

    size_t mReadCursor = 0;
    size_t mWriteCursor = 0;
    std::vector<char> mBuffer;
};

// Just see whether the test class is OK
TEST(VulkanStream, Basic) {
    VulkanStreamForTesting stream;

    const uint32_t testInt = 6;
    stream.putBe32(testInt);
    EXPECT_EQ(testInt, stream.getBe32());

    const std::string testString = "Hello World";
    stream.putString(testString);
    EXPECT_EQ(testString, stream.getString());
}

// Write / read arrays of strings
TEST(VulkanStream, StringArray) {
    VulkanStreamForTesting stream;

    const char* const testStrings[] = {
        "Hello World",
        "asdf",
        "",
        "https://android.googlesource.com",
    };

    stream.putStringArray(testStrings, arraySize(testStrings));

    auto stringsReturned = stream.getStringArray();

    for (int i = 0; i < stringsReturned.size(); i++) {
        EXPECT_EQ(stringsReturned[i], testStrings[i]);
    }
}

// Try a "basic" Vulkan struct (VkInstanceCreateInfo)
TEST(VulkanStream, testMarshalVulkanStruct) {
    VulkanStreamForTesting stream;

    VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        0, // pNext
        "VulkanStreamTest", // application name
        6, // application version
        "VulkanStreamTestEngine", //engine name
        4, // engine version,
        VK_API_VERSION_1_0,
    };

    const char* const layerNames[] = {
        "layer0",
        "layer1: test layer",
    };

    const char* const extensionNames[] = {
        "VK_KHR_8bit_storage",
        "VK_KHR_android_surface",
        "VK_MVK_macos_surface",
    };

    VkInstanceCreateInfo forMarshaling = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        0, // pNext
        0, // flags,
        &appInfo, // pApplicationInfo,
        arraySize(layerNames),
        layerNames,
        arraySize(extensionNames),
        extensionNames
    };

    marshal_VkInstanceCreateInfo(&stream, &forMarshaling);

    VkApplicationInfo forUnmarshalingAppInfo;
    VkInstanceCreateInfo forUnmarshaling;
    forUnmarshaling.pApplicationInfo = &forUnmarshalingAppInfo;

    unmarshal_VkInstanceCreateInfo(&stream, &forUnmarshaling);

    EXPECT_EQ(forMarshaling.sType, forUnmarshaling.sType);
    EXPECT_EQ(forMarshaling.pNext, forUnmarshaling.pNext);
    EXPECT_EQ(forMarshaling.flags, forUnmarshaling.flags);
    EXPECT_EQ(forMarshaling.pApplicationInfo->sType,
              forUnmarshaling.pApplicationInfo->sType);
    EXPECT_EQ(forMarshaling.pApplicationInfo->apiVersion,
              forUnmarshaling.pApplicationInfo->apiVersion);

    // TODO: Collect the strings
}

// Try a Vulkan struct that has non-ptr structs in it
TEST(VulkanStream, testMarshalVulkanStructWithNonPtrStruct) {
    VulkanStreamForTesting stream;

    VkPhysicalDeviceProperties forMarshaling = {
        VK_API_VERSION_1_0,
        0,
        0x8086,
        0x7800,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        "Intel740",
        "123456789abcdef",
        {
            0x00, // maxImageDimension1D;
            0x01, // maxImageDimension2D;
            0x02, // maxImageDimension3D;
            0x03, // maxImageDimensionCube;
            0x04, // maxImageArrayLayers;
            0x05, // maxTexelBufferElements;
            0x06, // maxUniformBufferRange;
            0x07, // maxStorageBufferRange;
            0x08, // maxPushConstantsSize;
            0x09, // maxMemoryAllocationCount;
            0x0a, // maxSamplerAllocationCount;
            0x0b, // bufferImageGranularity;
            0x0c, // sparseAddressSpaceSize;
            0x0d, // maxBoundDescriptorSets;
            0x0e, // maxPerStageDescriptorSamplers;
            0x0f, // maxPerStageDescriptorUniformBuffers;
            0x10, // maxPerStageDescriptorStorageBuffers;
            0x11, // maxPerStageDescriptorSampledImages;
            0x12, // maxPerStageDescriptorStorageImages;
            0x13, // maxPerStageDescriptorInputAttachments;
            0x14, // maxPerStageResources;
            0x15, // maxDescriptorSetSamplers;
            0x16, // maxDescriptorSetUniformBuffers;
            0x17, // maxDescriptorSetUniformBuffersDynamic;
            0x18, // maxDescriptorSetStorageBuffers;
            0x19, // maxDescriptorSetStorageBuffersDynamic;
            0x1a, // maxDescriptorSetSampledImages;
            0x1b, // maxDescriptorSetStorageImages;
            0x1c, // maxDescriptorSetInputAttachments;
            0x1d, // maxVertexInputAttributes;
            0x1e, // maxVertexInputBindings;
            0x1f, // maxVertexInputAttributeOffset;
            0x20, // maxVertexInputBindingStride;
            0x21, // maxVertexOutputComponents;
            0x22, // maxTessellationGenerationLevel;
            0x23, // maxTessellationPatchSize;
            0x24, // maxTessellationControlPerVertexInputComponents;
            0x25, // maxTessellationControlPerVertexOutputComponents;
            0x26, // maxTessellationControlPerPatchOutputComponents;
            0x27, // maxTessellationControlTotalOutputComponents;
            0x28, // maxTessellationEvaluationInputComponents;
            0x29, // maxTessellationEvaluationOutputComponents;
            0x2a, // maxGeometryShaderInvocations;
            0x2b, // maxGeometryInputComponents;
            0x2c, // maxGeometryOutputComponents;
            0x2d, // maxGeometryOutputVertices;
            0x2e, // maxGeometryTotalOutputComponents;
            0x2f, // maxFragmentInputComponents;
            0x30, // maxFragmentOutputAttachments;
            0x31, // maxFragmentDualSrcAttachments;
            0x32, // maxFragmentCombinedOutputResources;
            0x33, // maxComputeSharedMemorySize;
            { 0x1, 0x2, 0x3 }, // maxComputeWorkGroupCount[3];
            0x35, // maxComputeWorkGroupInvocations;
            { 0x4, 0x5, 0x6 }, // maxComputeWorkGroupSize[3];
            0x37, // subPixelPrecisionBits;
            0x38, // subTexelPrecisionBits;
            0x39, // mipmapPrecisionBits;
            0x3a, // maxDrawIndexedIndexValue;
            0x3b, // maxDrawIndirectCount;
            1.0f, // maxSamplerLodBias;
            1.0f, // maxSamplerAnisotropy;
            0x3e, // maxViewports;
            { 0x7, 0x8 }, // maxViewportDimensions[2];
            { 0.4f, 0.5f }, // viewportBoundsRange[2];
            0x41, // viewportSubPixelBits;
            0x42, // minMemoryMapAlignment;
            0x43, // minTexelBufferOffsetAlignment;
            0x44, // minUniformBufferOffsetAlignment;
            0x45, // minStorageBufferOffsetAlignment;
            0x46, // minTexelOffset;
            0x47, // maxTexelOffset;
            0x48, // minTexelGatherOffset;
            0x49, // maxTexelGatherOffset;
            10.0f, // minInterpolationOffset;
            11.0f, // maxInterpolationOffset;
            0x4c, // subPixelInterpolationOffsetBits;
            0x4d, // maxFramebufferWidth;
            0x4e, // maxFramebufferHeight;
            0x4f, // maxFramebufferLayers;
            0x50, // framebufferColorSampleCounts;
            0x51, // framebufferDepthSampleCounts;
            0x52, // framebufferStencilSampleCounts;
            0x53, // framebufferNoAttachmentsSampleCounts;
            0x54, // maxColorAttachments;
            0x55, // sampledImageColorSampleCounts;
            0x56, // sampledImageIntegerSampleCounts;
            0x57, // sampledImageDepthSampleCounts;
            0x58, // sampledImageStencilSampleCounts;
            0x59, // storageImageSampleCounts;
            0x5a, // maxSampleMaskWords;
            0x5b, // timestampComputeAndGraphics;
            100.0f, // timestampPeriod;
            0x5d, // maxClipDistances;
            0x5e, // maxCullDistances;
            0x5f, // maxCombinedClipAndCullDistances;
            0x60, // discreteQueuePriorities;
            { 0.0f, 1.0f }, // pointSizeRange[2];
            { 1.0f, 2.0f }, // lineWidthRange[2];
            3.0f, // pointSizeGranularity;
            4.0f, // lineWidthGranularity;
            0x65, // strictLines;
            0x66, // standardSampleLocations;
            0x67, // optimalBufferCopyOffsetAlignment;
            0x68, // optimalBufferCopyRowPitchAlignment;
            0x69, // nonCoherentAtomSize;
        },
        {
            0xff, // residencyStandard2DBlockShape;
            0x00, // residencyStandard2DMultisampleBlockShape;
            0x11, // residencyStandard3DBlockShape;
            0x22, // residencyAlignedMipSize;
            0x33, // residencyNonResidentStrict;
        },
    };

    marshal_VkPhysicalDeviceProperties(&stream, &forMarshaling);

    VkPhysicalDeviceProperties forUnmarshaling;
    memset(&forUnmarshaling, 0x0, sizeof(VkPhysicalDeviceLimits));

    unmarshal_VkPhysicalDeviceProperties(&stream, &forUnmarshaling);

    fprintf(stderr, "%s: %d\n", __func__,
            memcmp(&forMarshaling.limits, &forUnmarshaling.limits, sizeof(VkPhysicalDeviceLimits)));
    fprintf(stderr, "%s: %d\n", __func__,
            memcmp(&forMarshaling.sparseProperties, &forUnmarshaling.sparseProperties, sizeof(VkPhysicalDeviceSparseProperties)));


    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxImageDimension1D);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxImageDimension2D);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxImageDimension3D);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxImageDimensionCube);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxImageArrayLayers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTexelBufferElements);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxUniformBufferRange);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxStorageBufferRange);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPushConstantsSize);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxMemoryAllocationCount);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxSamplerAllocationCount);
    fprintf(stderr, "0x%llx\n", forUnmarshaling.limits.bufferImageGranularity);
    fprintf(stderr, "0x%llx\n", forUnmarshaling.limits.sparseAddressSpaceSize);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxBoundDescriptorSets);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPerStageDescriptorSamplers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPerStageDescriptorUniformBuffers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPerStageDescriptorStorageBuffers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPerStageDescriptorSampledImages);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPerStageDescriptorStorageImages);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPerStageDescriptorInputAttachments);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxPerStageResources);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetSamplers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetUniformBuffers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetUniformBuffersDynamic);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetStorageBuffers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetStorageBuffersDynamic);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetSampledImages);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetStorageImages);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDescriptorSetInputAttachments);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxVertexInputAttributes);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxVertexInputBindings);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxVertexInputAttributeOffset);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxVertexInputBindingStride);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxVertexOutputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationGenerationLevel);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationPatchSize);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationControlPerVertexInputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationControlPerVertexOutputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationControlPerPatchOutputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationControlTotalOutputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationEvaluationInputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTessellationEvaluationOutputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxGeometryShaderInvocations);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxGeometryInputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxGeometryOutputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxGeometryOutputVertices);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxGeometryTotalOutputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxFragmentInputComponents);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxFragmentOutputAttachments);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxFragmentDualSrcAttachments);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxFragmentCombinedOutputResources);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeSharedMemorySize);

    fprintf(stderr, "{\n");
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeWorkGroupCount[0]);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeWorkGroupCount[1]);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeWorkGroupCount[2]);
    fprintf(stderr, "}\n");

    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeWorkGroupInvocations);
    fprintf(stderr, "{\n");
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeWorkGroupSize[0]);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeWorkGroupSize[1]);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxComputeWorkGroupSize[2]);
    fprintf(stderr, "}\n");
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.subPixelPrecisionBits);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.subTexelPrecisionBits);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.mipmapPrecisionBits);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDrawIndexedIndexValue);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxDrawIndirectCount);
    fprintf(stderr, "%f\n", forUnmarshaling.limits.maxSamplerLodBias);
    fprintf(stderr, "%f\n", forUnmarshaling.limits.maxSamplerAnisotropy);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxViewports);

    fprintf(stderr, "{\n");
        fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxViewportDimensions[0]);
        fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxViewportDimensions[1]);
    fprintf(stderr, "}\n");

    fprintf(stderr, "{\n");
        fprintf(stderr, "%f\n", forUnmarshaling.limits.viewportBoundsRange[0]);
        fprintf(stderr, "%f\n", forUnmarshaling.limits.viewportBoundsRange[1]);
    fprintf(stderr, "}\n");

    // fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxViewportDimensions[2]);
    // fprintf(stderr, "0x%x\n", forUnmarshaling.limits.viewportBoundsRange[2]);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.viewportSubPixelBits);
    fprintf(stderr, "0x%llx\n", forUnmarshaling.limits.minMemoryMapAlignment);
    fprintf(stderr, "0x%llx\n", forUnmarshaling.limits.minTexelBufferOffsetAlignment);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.minUniformBufferOffsetAlignment);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.minStorageBufferOffsetAlignment);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.minTexelOffset);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTexelOffset);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.minTexelGatherOffset);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxTexelGatherOffset);
    fprintf(stderr, "%f\n", forUnmarshaling.limits.minInterpolationOffset);
    fprintf(stderr, "%f\n", forUnmarshaling.limits.maxInterpolationOffset);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.subPixelInterpolationOffsetBits);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxFramebufferWidth);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxFramebufferHeight);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxFramebufferLayers);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.framebufferColorSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.framebufferDepthSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.framebufferStencilSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.framebufferNoAttachmentsSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxColorAttachments);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.sampledImageColorSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.sampledImageIntegerSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.sampledImageDepthSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.sampledImageStencilSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.storageImageSampleCounts);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxSampleMaskWords);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.timestampComputeAndGraphics);
    fprintf(stderr, "%f\n", forUnmarshaling.limits.timestampPeriod);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxClipDistances);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxCullDistances);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.maxCombinedClipAndCullDistances);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.discreteQueuePriorities);
    // fprintf(stderr, "0x%x\n", forUnmarshaling.limits.pointSizeRange[2]);
    // fprintf(stderr, "0x%x\n", forUnmarshaling.limits.lineWidthRange[2]);
    //
    fprintf(stderr, "{\n");
        fprintf(stderr, "%f\n", forUnmarshaling.limits.pointSizeRange[0]);
        fprintf(stderr, "%f\n", forUnmarshaling.limits.pointSizeRange[1]);
    fprintf(stderr, "}\n");

    fprintf(stderr, "{\n");
        fprintf(stderr, "%f\n", forUnmarshaling.limits.lineWidthRange[0]);
        fprintf(stderr, "%f\n", forUnmarshaling.limits.lineWidthRange[1]);
    fprintf(stderr, "}\n");
    fprintf(stderr, "%f\n", forUnmarshaling.limits.pointSizeGranularity);
    fprintf(stderr, "%f\n", forUnmarshaling.limits.lineWidthGranularity);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.strictLines);
    fprintf(stderr, "0x%x\n", forUnmarshaling.limits.standardSampleLocations);
    fprintf(stderr, "0x%llx\n", forUnmarshaling.limits.optimalBufferCopyOffsetAlignment);
    fprintf(stderr, "0x%llx\n", forUnmarshaling.limits.optimalBufferCopyRowPitchAlignment);
    fprintf(stderr, "0x%llx\n", forUnmarshaling.limits.nonCoherentAtomSize);
}

} // namespace goldfish_vk
