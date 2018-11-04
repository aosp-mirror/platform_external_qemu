# Unit tests
set(OpenglRender_vulkan_unittests_src VulkanStream_unittest.cpp)
android_add_test(OpenglRender_vulkan_unittests)
target_link_libraries(OpenglRender_vulkan_unittests PRIVATE emugl_base)
target_link_libraries(OpenglRender_vulkan_unittests
                              PUBLIC
                              OpenglRender_standalone_common
                              OpenglRender_vulkan_cereal
                              gmock_main)
target_include_directories(OpenglRender_vulkan_unittests
                                   PRIVATE
                                   .
                                   cereal
                                   ${ANDROID_EMUGL_DIR}/host/include/OpenglRender
                                   ${ANDROID_EMUGL_DIR}/shared/OpenglCodecCommon
                                   ${ANDROID_EMUGL_DIR}/host/include/vulkan)
