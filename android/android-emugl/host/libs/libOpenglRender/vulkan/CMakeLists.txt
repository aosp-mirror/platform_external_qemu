set(OpenglRender_vulkan_src VulkanDispatch.cpp VulkanStream.cpp)
android_add_library(OpenglRender_vulkan)
target_link_libraries(OpenglRender_vulkan PRIVATE emugl_base)
target_link_libraries(OpenglRender_vulkan PUBLIC OpenglRender_vulkan_cereal android-emu)
target_include_directories(OpenglRender_vulkan
                                   PRIVATE
                                   .
                                   cereal
                                   ${ANDROID_EMUGL_DIR}/host/include/vulkan)

# Unit tests
set(OpenglRender_vulkan_unittests_src VulkanStream_unittest.cpp)
android_add_test(OpenglRender_vulkan_unittests)
target_link_libraries(OpenglRender_vulkan_unittests PRIVATE emugl_base)
target_link_libraries(OpenglRender_vulkan_unittests
                              PUBLIC
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              gmock_main)
target_include_directories(OpenglRender_vulkan_unittests
                                   PRIVATE
                                   .
                                   cereal
                                   ${ANDROID_EMUGL_DIR}/host/include/vulkan)
