
set(utils_src androidImpl/libutils_placeholder.cpp)
android_add_shared_library(utils)

# The cutils library.
set(cutils_src androidImpl/Ashmem.cpp androidImpl/Properties.cpp androidImpl/GrallocDispatch.cpp)
android_add_shared_library(cutils)
target_include_directories(cutils PRIVATE ${ANDROID_EMUGL_DIR}/host/include .)
target_include_directories(cutils PUBLIC .)
target_link_libraries(cutils PUBLIC android-emu-base)
target_link_libraries(cutils PRIVATE emugl_base)
android_target_compile_options(cutils Clang PRIVATE -fvisibility=default -Wno-invalid-constexpr)

# A small logging library
set(log_src androidImpl/Log.cpp)
android_add_shared_library(log)
target_include_directories(log PRIVATE android/android-emugl/guest android/android-emu)
target_link_libraries(log PRIVATE emugl_base)

# The gui library
set(gui_src
    androidImpl/AndroidBufferQueue.cpp
    androidImpl/ANativeWindow.cpp
    androidImpl/AndroidWindow.cpp
    androidImpl/AndroidWindowBuffer.cpp
    androidImpl/SurfaceFlinger.cpp
    androidImpl/Vsync.cpp
    sync/sync.cpp
    )
android_add_shared_library(gui)
target_link_libraries(gui PUBLIC android-emu-base)
target_include_directories(gui PUBLIC ${ANDROID_EMUGL_DIR}/guest)
target_compile_options(gui PRIVATE -fvisibility=default)
target_link_libraries(gui PRIVATE emugl_base)

# The unit tests
set(gui_unittests_src androidImpl/libgui_unittest.cpp)
android_add_test(gui_unittests)
target_link_libraries(gui_unittests PUBLIC gui emugl_base gtest gtest_main)

set(grallocusage_src libgrallocusage/GrallocUsageConversion.cpp)
android_add_shared_library(grallocusage)
target_link_libraries(grallocusage PUBLIC emugl_base)

set(vulkan_android_src
    libvulkan/api.cpp
    libvulkan/api_gen.cpp
    libvulkan/debug_report.cpp
    libvulkan/driver.cpp
    libvulkan/driver_gen.cpp
    libvulkan/layers_extensions.cpp
    libvulkan/stubhal.cpp
    libvulkan/swapchain.cpp
)
android_add_shared_library(vulkan_android)
target_compile_definitions(vulkan_android PRIVATE
   -DLOG_TAG=\"vulkan\"
   -DVK_USE_PLATFORM_ANDROID_KHR
   -DVK_NO_PROTOTYPES)
target_include_directories(vulkan_android PRIVATE ${ANDROID_EMUGL_DIR}/host/include)
target_compile_options(vulkan_android PRIVATE -fvisibility=hidden -fstrict-aliasing)
target_link_libraries(vulkan_android PRIVATE grallocusage)
target_link_libraries(vulkan_android PUBLIC emugl_base log gui cutils utils)

set(vulkanhal_unittests_src androidImpl/vulkanhal_unittest.cpp)
android_add_test(vulkanhal_unittests)
target_include_directories(
    vulkanhal_unittests PRIVATE
    ${ANDROID_EMUGL_DIR}/host/include
    ${ANDROID_EMUGL_DIR}/combined
    ${ANDROID_EMUGL_DIR}/guest/grallocusage)
target_link_libraries(
    vulkanhal_unittests PRIVATE
    grallocusage vulkan_android aemugraphics PRIVATE
    gtest_main)
android_target_dependency(vulkanhal_unittests all EMULATOR_FEATURE_DEPENDENCIES)
android_target_dependency(vulkanhal_unittests all SWIFTSHADER_DEPENDENCIES)
android_target_properties(vulkanhal_unittests darwin-x86_64 "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
android_target_properties(vulkanhal_unittests linux-x86_64 "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")