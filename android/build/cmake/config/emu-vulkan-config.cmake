# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

get_filename_component(
  PREBUILT_ROOT
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/vulkan/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

set(VULKAN_FOUND TRUE)

if(LINUX_X86_64)
  set(VULKAN_DEPENDENCIES
      # Loader (for real)
      "${PREBUILT_ROOT}/libvulkan.so>lib64/vulkan/libvulkan.so"
      "${PREBUILT_ROOT}/libvulkan.so>lib64/vulkan/libvulkan.so.1"
      # Swiftshader
      "${PREBUILT_ROOT}/icds/libvk_swiftshader.so>lib64/vulkan/libvk_swiftshader.so"
      "${PREBUILT_ROOT}/icds/vk_swiftshader_icd.json>lib64/vulkan/vk_swiftshader_icd.json"
      # for translating shaders to SPIRV
      "${PREBUILT_ROOT}/glslangValidator>lib64/vulkan/glslangValidator"
      ${VULKAN_COMMON_DEPENDENCIES})
  set(VULKAN_TEST_DEPENDENCIES
      # Loader (for testing)
      "${PREBUILT_ROOT}/libvulkan.so>testlib64/libvulkan.so"
      # Mock ICD
      "${PREBUILT_ROOT}/icds/libVkICD_mock_icd.so>testlib64/libVkICD_mock_icd.so"
      "${PREBUILT_ROOT}/icds/VkICD_mock_icd.json>testlib64/VkICD_mock_icd.json"
      # Debug / validation layers
      "${PREBUILT_ROOT}/layers/libVkLayer_api_dump.so>testlib64/layers/libVkLayer_api_dump.so"
      "${PREBUILT_ROOT}/layers/libVkLayer_device_simulation.so>testlib64/layers/libVkLayer_device_simulation.so"
      "${PREBUILT_ROOT}/layers/libVkLayer_khronos_validation.so>testlib64/layers/libVkLayer_khronos_validation.so"
      "${PREBUILT_ROOT}/layers/libVkLayer_monitor.so>testlib64/layers/libVkLayer_monitor.so"
      "${PREBUILT_ROOT}/layers/libVkLayer_screenshot.so>testlib64/layers/libVkLayer_screenshot.so"
      "${PREBUILT_ROOT}/layers/VkLayer_api_dump.json>testlib64/layers/VkLayer_api_dump.json"
      "${PREBUILT_ROOT}/layers/VkLayer_device_simulation.json>testlib64/layers/VkLayer_device_simulation.json"
      "${PREBUILT_ROOT}/layers/VkLayer_khronos_validation.json>testlib64/layers/VkLayer_khronos_validation.json"
      "${PREBUILT_ROOT}/layers/VkLayer_monitor.json>testlib64/layers/VkLayer_monitor.json"
      "${PREBUILT_ROOT}/layers/VkLayer_screenshot.json>testlib64/layers/VkLayer_screenshot.json"
      # Shaders
      ${VULKAN_COMMON_DEPENDENCIES})
elseif(DARWIN_X86_64)
  set(VULKAN_DEPENDENCIES
      # Swiftshader
      "${PREBUILT_ROOT}/icds/libvk_swiftshader.dylib>lib64/vulkan/libvk_swiftshader.dylib"
      "${PREBUILT_ROOT}/icds/vk_swiftshader_icd.json>lib64/vulkan/vk_swiftshader_icd.json"
      # for translating shaders to SPIRV
      "${PREBUILT_ROOT}/glslangValidator>lib64/vulkan/glslangValidator"
      # On mac we need these on our load path MoltenVK
      "${PREBUILT_ROOT}/libvulkan.dylib>lib64/vulkan/libvulkan.dylib"
      "${PREBUILT_ROOT}/icds/libMoltenVK.dylib>lib64/vulkan/libMoltenVK.dylib"
      "${PREBUILT_ROOT}/icds/MoltenVK_icd.json>lib64/vulkan/MoltenVK_icd.json"
      # Shaders
      ${VULKAN_COMMON_DEPENDENCIES})
  set(VULKAN_TEST_DEPENDENCIES
      # Loader (for testing)
      "${PREBUILT_ROOT}/libvulkan.dylib>testlib64/libvulkan.dylib"
      # Mock ICD
      "${PREBUILT_ROOT}/icds/libVkICD_mock_icd.dylib>testlib64/libVkICD_mock_icd.dylib"
      "${PREBUILT_ROOT}/icds/VkICD_mock_icd.json>testlib64/VkICD_mock_icd.json"
      # Debug / validation layers
      "${PREBUILT_ROOT}/layers/libVkLayer_api_dump.dylib>testlib64/layers/libVkLayer_api_dump.dylib"
      "${PREBUILT_ROOT}/layers/libVkLayer_device_simulation.dylib>testlib64/layers/libVkLayer_device_simulation.dylib"
      "${PREBUILT_ROOT}/layers/libVkLayer_khronos_validation.dylib>testlib64/layers/libVkLayer_khronos_validation.dylib"
      "${PREBUILT_ROOT}/layers/VkLayer_api_dump.json>testlib64/layers/VkLayer_api_dump.json"
      "${PREBUILT_ROOT}/layers/VkLayer_device_simulation.json>testlib64/layers/VkLayer_device_simulation.json"
      "${PREBUILT_ROOT}/layers/VkLayer_khronos_validation.json>testlib64/layers/VkLayer_khronos_validation.json"
      # shaders
      ${VULKAN_COMMON_DEPENDENCIES})
elseif(DARWIN_AARCH64)
  set(VULKAN_DEPENDENCIES
      # Loader
      "${PREBUILT_ROOT}/libvulkan.dylib>lib64/vulkan/libvulkan.dylib"
      # Swiftshader
      "${PREBUILT_ROOT}/icds/libvk_swiftshader.dylib>lib64/vulkan/libvk_swiftshader.dylib"
      "${PREBUILT_ROOT}/icds/vk_swiftshader_icd.json>lib64/vulkan/vk_swiftshader_icd.json"
      # MoltenVK
      "${PREBUILT_ROOT}/icds/libMoltenVK.dylib>lib64/vulkan/libMoltenVK.dylib"
      "${PREBUILT_ROOT}/icds/MoltenVK_icd.json>lib64/vulkan/MoltenVK_icd.json")
elseif(WINDOWS)
  get_filename_component(
    PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/vulkan/windows-x86_64"
    ABSOLUTE)
  set(VULKAN_DEPENDENCIES
      # Swiftshader
      "${PREBUILT_ROOT}/icds/vk_swiftshader.dll>lib64/vulkan/vk_swiftshader.dll"
      "${PREBUILT_ROOT}/icds/vk_swiftshader_icd.json>lib64/vulkan/vk_swiftshader_icd.json"
      # for translating shaders to SPIRV
      "${PREBUILT_ROOT}/glslangValidator.exe>lib64/vulkan/glslangValidator.exe"
      ${VULKAN_COMMON_DEPENDENCIES})
  set(VULKAN_TEST_DEPENDENCIES
      # Loader (for testing)
      "${PREBUILT_ROOT}/vulkan-1.dll>testlib64/vulkan-1.dll"
      # Mock ICD
      "${PREBUILT_ROOT}/icds/VkICD_mock_icd.dll>testlib64/VkICD_mock_icd.dll"
      "${PREBUILT_ROOT}/icds/VkICD_mock_icd.json>testlib64/VkICD_mock_icd.json"
      # Debug / validation layers
      "${PREBUILT_ROOT}/layers/VkLayer_api_dump.dll>testlib64/layers/VkLayer_api_dump.dll"
      "${PREBUILT_ROOT}/layers/VkLayer_api_dump.json>testlib64/layers/VkLayer_api_dump.json"
      "${PREBUILT_ROOT}/layers/VkLayer_device_simulation.dll>testlib64/layers/VkLayer_device_simulation.dll"
      "${PREBUILT_ROOT}/layers/VkLayer_device_simulation.json>testlib64/layers/VkLayer_device_simulation.json"
      "${PREBUILT_ROOT}/layers/VkLayer_gfxreconstruct.dll>testlib64/layers/VkLayer_gfxreconstruct.dll"
      "${PREBUILT_ROOT}/layers/VkLayer_gfxreconstruct.json>testlib64/layers/VkLayer_gfxreconstruct.json"
      "${PREBUILT_ROOT}/layers/VkLayer_khronos_validation.dll>testlib64/layers/VkLayer_khronos_validation.dll"
      "${PREBUILT_ROOT}/layers/VkLayer_khronos_validation.json>testlib64/layers/VkLayer_khronos_validation.json"
      "${PREBUILT_ROOT}/layers/VkLayer_khronos_validation.pdb>testlib64/layers/VkLayer_khronos_validation.pdb"
      "${PREBUILT_ROOT}/layers/VkLayer_monitor.dll>testlib64/layers/VkLayer_monitor.dll"
      "${PREBUILT_ROOT}/layers/VkLayer_monitor.json>testlib64/layers/VkLayer_monitor.json"
      "${PREBUILT_ROOT}/layers/VkLayer_screenshot.dll>testlib64/layers/VkLayer_screenshot.dll"
      "${PREBUILT_ROOT}/layers/VkLayer_screenshot.json>testlib64/layers/VkLayer_screenshot.json"
      # Shaders
      ${VULKAN_COMMON_DEPENDENCIES})
endif()
set(PACKAGE_EXPORT "VULKAN_DEPENDENCIES;VULKAN_TEST_DEPENDENCIES;VULKAN_FOUND")
android_license(
  TARGET VULKAN_DEPENDENCIES
  LIBNAME "vulkan-sdk"
  URL "https://vulkan.lunarg.com/sdk/home"
  SPDX "Apache-2.0"
  LICENSE
    "https://vulkan.lunarg.com/software/license/vulkan-1.2.162.1-linux-license-summary.txt"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.VULKAN")
