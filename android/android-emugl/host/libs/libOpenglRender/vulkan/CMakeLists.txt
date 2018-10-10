set(OpenglRender_vulkan_src VulkanDispatch.cpp VulkanStream.cpp)
android_add_library(OpenglRender_vulkan)
android_target_link_libraries(OpenglRender_vulkan all PRIVATE emugl_base)
android_target_link_libraries(OpenglRender_vulkan all PUBLIC OpenglRender_vulkan_cereal android-emu)
android_target_include_directories(OpenglRender_vulkan
                                   all
                                   PRIVATE
                                   .
                                   cereal
                                   ${ANDROID_EMUGL_DIR}/host/include/vulkan)

# Unit tests
set(OpenglRender_vulkan_unittests_src VulkanStream_unittest.cpp)
android_add_test(OpenglRender_vulkan_unittests)
android_target_link_libraries(OpenglRender_vulkan_unittests all PRIVATE emugl_base)
android_target_link_libraries(OpenglRender_vulkan_unittests
                              all
                              PUBLIC
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              gmock_main)
android_target_include_directories(OpenglRender_vulkan_unittests
                                   all
                                   PRIVATE
                                   .
                                   cereal
                                   ${ANDROID_EMUGL_DIR}/host/include/vulkan)
