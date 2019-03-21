set(emugl_common_src
    crash_reporter.cpp
    dma_device.cpp
    vm_operations.cpp
    window_operations.cpp
    feature_control.cpp
    logging.cpp
    misc.cpp
    shared_library.cpp
    stringparsing.cpp
    sync_device.cpp)

android_add_shared_library(emugl_common)
android_target_compile_definitions(emugl_common windows_msvc-x86_64 PRIVATE "-DBUILDING_EMUGL_COMMON_SHARED")
target_compile_options(emugl_common PRIVATE -fvisibility=default)
target_link_libraries(emugl_common PRIVATE emugl_base)
target_link_libraries(emugl_common PRIVATE android-emu-base)
target_include_directories(emugl_common PRIVATE ${ANDROID_EMUGL_DIR}/host/include
                                   ${ANDROID_EMUGL_DIR}/shared)

android_install_shared(emugl_common)

set(emugl_test_shared_library_src testing/test_shared_library.cpp)
android_add_shared_library(emugl_test_shared_library)
android_target_compile_definitions(emugl_test_shared_library windows_msvc-x86_64 PRIVATE "-DBUILDING_EMUGL_COMMON_SHARED")

set(emugl_common_host_unittests_src shared_library_unittest.cpp stringparsing_unittest.cpp)
android_add_test(emugl_common_host_unittests)
target_link_libraries(emugl_common_host_unittests PRIVATE emugl_base)
target_link_libraries(emugl_common_host_unittests
                              PUBLIC
                              android-emu-base
                              emugl_test_shared_library
                              emugl_common
                              gmock_main)
target_include_directories(emugl_common_host_unittests PRIVATE ${ANDROID_EMUGL_DIR}/host/include
                                   ${ANDROID_EMUGL_DIR}/shared)
