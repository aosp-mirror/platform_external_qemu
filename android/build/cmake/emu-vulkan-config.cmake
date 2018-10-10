get_filename_component(
  PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/vulkan/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

if(ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  set(VULKAN_TEST_DEPENDENCIES "${PREBUILT_ROOT}/libvulkan.so>testlib64/libvulkan.so"
      "${PREBUILT_ROOT}/libVkICD_mock_icd.so>testlib64/libVkICD_mock_icd.so"
      "${PREBUILT_ROOT}/VkICD_mock_icd.json>testlib64/VkICD_mock_icd.json")
elseif(ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
  set(VULKAN_TEST_DEPENDENCIES
      "${PREBUILT_ROOT}/libvulkan.dylib>testlib64/libvulkan.dylib"
      "${PREBUILT_ROOT}/libVkICD_mock_icd.dylib>testlib64/libVkICD_mock_icd.dylib"
      "${PREBUILT_ROOT}/VkICD_mock_icd.json>testlib64/VkICD_mock_icd.json"
      # On mac we need these on our load path
      "${PREBUILT_ROOT}/libMoltenVK.dylib>lib64/libMoltenVK.dylib"
      "${PREBUILT_ROOT}/MoltenVK_icd.json>lib64/MoltenVK_icd.json"
      "${PREBUILT_ROOT}/libvulkan.dylib>lib64/libvulkan.dylib"
      )
elseif(ANDROID_TARGET_TAG MATCHES "windows.*")
  set(VULKAN_TEST_DEPENDENCIES "${PREBUILT_ROOT}/libvulkan.so>testlib64/libvulkan.so"
      "${PREBUILT_ROOT}/libVkICD_mock_icd.so>testlib64/libVkICD_mock_icd.so"
      "${PREBUILT_ROOT}/VkICD_mock_icd.json>testlib64/VkICD_mock_icd.json")
endif()
set(PACKAGE_EXPORT "VULKAN_TEST_DEPENDENCIES")
