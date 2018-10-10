set(OpenglRender_vulkan_cereal_src
    common/goldfish_vk_marshaling.cpp
    guest/goldfish_vk_frontend.cpp
    common/goldfish_vk_testing.cpp
    common/goldfish_vk_deepcopy.cpp
    common/goldfish_vk_handlemap.cpp
    common/goldfish_vk_dispatch.cpp)
android_add_library(OpenglRender_vulkan_cereal)

target_link_libraries(OpenglRender_vulkan_cereal PRIVATE emugl_base)
target_link_libraries(OpenglRender_vulkan_cereal PUBLIC android-emu-base)

target_include_directories(OpenglRender_vulkan_cereal
                                   PRIVATE
                                   ${ANDROID_EMUGL_DIR}/host/include
                                   ${ANDROID_EMUGL_DIR}/host/libs/libOpenglRender/vulkan/
                                   ${ANDROID_EMUGL_DIR}/host/libs/libOpenglRender/vulkan/cereal
                                   ${ANDROID_EMUGL_DIR}/host/include/vulkan)
