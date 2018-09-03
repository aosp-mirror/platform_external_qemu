# Copyright (c) 2018 The Android Open Source Project
# Copyright (c) 2018 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from copy import deepcopy

from .common.codegen import CodeGen
from .common.vulkantypes import \
        VulkanAPI, makeVulkanTypeSimple, iterateVulkanType

from .wrapperdefs import VulkanWrapperGenerator

# No real good way to automatically infer the most important Vulkan API
# functions as it relates to which getProcAddress function to use, plus
# we might want to control which function to use depending on our
# performance needs.
getProcAddrFuncs = [
    "vkCreateInstance",
    "vkDestroyInstance",
    "vkEnumeratePhysicalDevices",
    "vkGetPhysicalDeviceFeatures",
    "vkGetPhysicalDeviceFormatProperties",
    "vkGetPhysicalDeviceImageFormatProperties",
    "vkGetPhysicalDeviceProperties",
    "vkGetPhysicalDeviceQueueFamilyProperties",
    "vkGetPhysicalDeviceMemoryProperties",
    "vkGetInstanceProcAddr",
    "vkGetDeviceProcAddr",
    "vkCreateDevice",
    "vkDestroyDevice",
    "vkEnumerateInstanceExtensionProperties",
    "vkEnumerateDeviceExtensionProperties",
    "vkEnumerateInstanceLayerProperties",
    "vkEnumerateDeviceLayerProperties",
    "vkGetDeviceQueue",
    "vkQueueSubmit",
    "vkQueueWaitIdle",
    "vkDeviceWaitIdle",
]

getInstanceProcAddrFuncs = [
    "vkCreateSwapchainKHR",
    "vkDestroySwapchainKHR",
    "vkGetSwapchainImagesKHR",
    "vkAcquireNextImageKHR",
    "vkQueuePresentKHR",
    "vkCreateMacOSSurfaceMVK",
    "vkCreateWin32SurfaceKHR",
    "vkGetPhysicalDeviceWin32PresentationSupportKHR",
    "vkCreateXlibSurfaceKHR",
    "vkGetPhysicalDeviceXlibPresentationSupportKHR",
    "vkCreateXcbSurfaceKHR",
    "vkGetPhysicalDeviceXcbPresentationSupportKHR",
]

# Implicitly, everything else is going to be obtained
# with vkGetDeviceProcAddr,
# unless it has instance in the arg.

def isGetProcAddressAPI(vulkanApi):
    return vulkanApi.name in getProcAddrFuncs

def isGetInstanceProcAddressAPI(vulkanApi):
    if vulkanApi.name in getInstanceProcAddrFuncs:
        return True

    if vulkanApi.parameters[0].typeName == "VkInstance":
        return True

    return False

def isGetDeviceProcAddressAPI(vulkanApi):
    if isGetProcAddressAPI(vulkanApi):
        return False

    if isGetInstanceProcAddressAPI(vulkanApi):
        return False

    return True

def inferProcAddressFuncType(vulkanApi):
    if isGetProcAddressAPI(vulkanApi):
        return "global"
    if isGetInstanceProcAddressAPI(vulkanApi):
        return "instance"
    return "device"

class VulkanDispatch(VulkanWrapperGenerator):

    def __init__(self, module, typeInfo):
        VulkanWrapperGenerator.__init__(self, module, typeInfo)
        self.knownDefs = {}
        self.cgen = CodeGen()
        self.typeInfo = typeInfo

    def onBegin(self):
        self.cgen.line("struct VulkanDispatch {")
        self.cgen.line("VkInstance instance;")
        self.cgen.line("VkPhysicalDevice physicalDevice;")
        self.cgen.line("VkDevice device;")
        self.cgen.line("VkQueue graphicsQueue;")
        self.cgen.line("VkQueue computeQueue;")
        self.cgen.line("VkQueue presentQueue;")
        self.module.appendHeader(self.cgen.swapCode())

    def onEnd(self):
        self.cgen.line("};")
        self.module.appendHeader(self.cgen.swapCode())

    def onGenType(self, typeXml, name, alias):
        VulkanWrapperGenerator.onGenType(self, typeXml, name, alias)

    def onGenCmd(self, cmdinfo, name, alias):
        VulkanWrapperGenerator.onGenCmd(self, cmdinfo, name, alias)

        vulkanApi = self.typeInfo.apis[name]

        self.cgen.line("// %s proc addr by %s" % (name, inferProcAddressFuncType(vulkanApi)))
        self.cgen.stmt("PFN_%s %s" % (name, name));
        self.module.appendHeader(self.cgen.swapCode())
