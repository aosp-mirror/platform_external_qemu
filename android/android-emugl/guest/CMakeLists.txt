android_add_library(
  TARGET utils SHARED LICENSE Apache-2.0
  SRC # cmake-format: sortable
      androidImpl/libutils_placeholder.cpp)

# A small logging library
android_add_library(
  TARGET log SHARED LICENSE Apache-2.0 SRC # cmake-format: sortable
                                           androidImpl/Log.cpp)
target_include_directories(log PRIVATE android/android-emugl/guest
                                       android/android-emu)
target_link_libraries(log PRIVATE emugl_base)
if(NOT BUILDING_FOR_AARCH64)
  # The cutils library.
  android_add_library(
    TARGET cutils
    SHARED
    LICENSE Apache-2.0
    SRC # cmake-format: sortable
        androidImpl/AndroidVulkanDispatch.cpp
        androidImpl/Ashmem.cpp
        androidImpl/GrallocDispatch.cpp
        androidImpl/native_handle.cpp
        androidImpl/Properties.cpp
        androidImpl/RefBase.cpp
        androidImpl/SharedBuffer.cpp
        androidImpl/String16.cpp
        androidImpl/String8.cpp
        androidImpl/trace-host.cpp
        androidImpl/Unicode.cpp)
  target_include_directories(cutils PRIVATE ${ANDROID_EMUGL_DIR}/host/include .)
  target_include_directories(cutils PUBLIC .)
  target_link_libraries(cutils PUBLIC android-emu-base android-emu-tracing)
  target_link_libraries(cutils PRIVATE log emugl_base)
  android_target_compile_options(
      cutils Clang PRIVATE -fvisibility=default -Wno-invalid-constexpr -Wno-format
                         -Wno-macro-redefined)
  target_compile_definitions(cutils PRIVATE -D__ANDROID_API__=28)

  android_add_library(
    TARGET grallocusage SHARED LICENSE Apache-2.0
    SRC # cmake-format: sortable
        libgrallocusage/GrallocUsageConversion.cpp)
  target_include_directories(grallocusage PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(grallocusage PUBLIC emugl_base)
  target_compile_definitions(grallocusage PRIVATE -D__ANDROID_API__=28)

  # The gui library
  android_add_library(
    TARGET gui
    SHARED
    LICENSE Apache-2.0
    SRC # cmake-format: sortable
        androidImpl/AHardwareBuffer.cpp
        androidImpl/ANativeWindow.cpp
        androidImpl/AndroidBufferQueue.cpp
        androidImpl/AndroidWindow.cpp
        androidImpl/AndroidWindowBuffer.cpp
        androidImpl/Gralloc2.cpp
        androidImpl/GraphicBuffer.cpp
        androidImpl/GraphicBufferAllocator.cpp
        androidImpl/GraphicBufferMapper.cpp
        androidImpl/PixelFormat.cpp
        androidImpl/SurfaceFlinger.cpp
        androidImpl/Vsync.cpp
        sync/sync.cpp)
  target_link_libraries(gui PUBLIC android-emu-base)
  target_include_directories(gui PUBLIC ${ANDROID_EMUGL_DIR}/guest)
  target_compile_options(gui PRIVATE -fvisibility=default)
  target_compile_definitions(gui PRIVATE -D__ANDROID_API__=28)
  target_link_libraries(gui PRIVATE cutils log emugl_base grallocusage)

  # The unit tests
  android_add_test(TARGET gui_unittests SRC # cmake-format: sortable
                                            androidImpl/libgui_unittest.cpp)
  target_link_libraries(gui_unittests PUBLIC gui emugl_base gtest gtest_main)

endif()
