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

from .common.codegen import VulkanWrapperGenerator
from .common.vulkantypes import makeVulkanTypeSimple

# Contains definitions for various Vulkan API wrappers. This information is
# shared to make it easier for one kind of wrapper to know how to call
# another one.

API_PREFIX_MARSHAL = "marshal_"
API_PREFIX_UNMARSHAL = "unmarshal_"

MARSHAL_INPUT_VAR_NAME = "forMarshaling"
UNMARSHAL_INPUT_VAR_NAME = "forUnmarshaling"

API_PREFIX_VALIDATE = "validate_"
API_PREFIX_FRONTEND = "goldfish_frontend_"

VULKAN_STREAM_TYPE = "VulkanStream"
VULKAN_STREAM_TYPE_GUEST = "VulkanStreamGuest"
VULKAN_STREAM_VAR_NAME = "vkStream"

VALIDATE_RESULT_TYPE = "VkResult"
VALIDATE_VAR_NAME = "validateResult"
VALIDATE_GOOD_RESULT = "VK_SUCCESS"

PARAMETERS_MARSHALING = [
    makeVulkanTypeSimple(False, VULKAN_STREAM_TYPE, 1, VULKAN_STREAM_VAR_NAME)
]
PARAMETERS_MARSHALING_GUEST = [
    makeVulkanTypeSimple(False, VULKAN_STREAM_TYPE_GUEST, 1, VULKAN_STREAM_VAR_NAME)
]
PARAMETERS_VALIDATE = [
    makeVulkanTypeSimple(False, VALIDATE_RESULT_TYPE, 1, VALIDATE_VAR_NAME)
]

STRUCT_EXTENSION_PARAM = \
    makeVulkanTypeSimple(True, "void", 1, "structExtension")

STRUCT_EXTENSION_PARAM2 = \
    makeVulkanTypeSimple(True, "void", 1, "structExtension2")

STRUCT_EXTENSION_PARAM_FOR_WRITE = \
    makeVulkanTypeSimple(False, "void", 1, "structExtension_out")

STRUCT_TYPE_API_NAME = "goldfish_vk_struct_type"
EXTENSION_SIZE_API_NAME = "goldfish_vk_extension_struct_size"

VOID_TYPE = makeVulkanTypeSimple(False, "void", 0)
STREAM_RET_TYPE = makeVulkanTypeSimple(False, "void", 0)

API_PREFIX_EQUALITY = "checkEqual_"
EQUALITY_VAR_NAMES = ["a", "b"]
EQUALITY_ON_FAIL_VAR = "onFail"
EQUALITY_ON_FAIL_VAR_TYPE = makeVulkanTypeSimple(False, "OnFailCompareFunc", 0,
                                                 EQUALITY_ON_FAIL_VAR)
EQUALITY_RET_TYPE = makeVulkanTypeSimple(False, "void", 0)
