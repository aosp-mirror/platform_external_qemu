# The cutils library.
set(cutils_src androidImpl/Ashmem.cpp androidImpl/Properties.cpp androidImpl/GrallocDispatch.cpp)
android_add_shared_library(cutils)
android_target_include_directories(cutils all PRIVATE ${ANDROID_EMUGL_DIR}/host/include .)
android_target_include_directories(cutils all PUBLIC .)
android_target_link_libraries(cutils all PUBLIC android-emu-base)
android_target_link_libraries(cutils all PRIVATE emugl_base)
android_target_compile_options(cutils all PRIVATE -fvisibility=default -Wno-invalid-constexpr)

# A small logging library
set(log_src androidImpl/Log.cpp)
android_add_shared_library(log)
android_target_include_directories(log all PRIVATE android/android-emugl/guest android/android-emu)
android_target_link_libraries(log all PRIVATE emugl_base)

# The gui library
set(gui_src
    androidImpl/AndroidBufferQueue.cpp
    androidImpl/ANativeWindow.cpp
    androidImpl/AndroidWindow.cpp
    androidImpl/AndroidWindowBuffer.cpp
    androidImpl/SurfaceFlinger.cpp
    androidImpl/Vsync.cpp)
android_add_shared_library(gui)
android_target_link_libraries(gui all PUBLIC android-emu-base)
android_target_include_directories(gui all PUBLIC ${ANDROID_EMUGL_DIR}/guest)
android_target_compile_options(gui all PRIVATE -fvisibility=default)
android_target_link_libraries(gui all PRIVATE emugl_base)

# The unit tests
set(gui_unittests_src androidImpl/libgui_unittest.cpp)
android_add_test(gui_unittests)
android_target_link_libraries(gui_unittests all PUBLIC gui emugl_base gtest gtest_main)
