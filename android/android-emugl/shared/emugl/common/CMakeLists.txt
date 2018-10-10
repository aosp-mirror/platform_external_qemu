set(emugl_common_src
    crash_reporter.cpp
    dma_device.cpp
    feature_control.cpp
    logging.cpp
    misc.cpp
    shared_library.cpp
    stringparsing.cpp
    sync_device.cpp)
android_add_library(emugl_common)
android_target_link_libraries(emugl_common all PRIVATE emugl_base)
android_target_link_libraries(emugl_common all PUBLIC android-emu-base)
android_target_include_directories(emugl_common all PRIVATE ${ANDROID_EMUGL_DIR}/host/include
                                   ${ANDROID_EMUGL_DIR}/shared)

set(emugl_test_shared_library_src testing/test_shared_library.cpp)
android_add_shared_library(emugl_test_shared_library)
# TODO: We are not consistent with our prefix naming..
set_target_properties(emugl_test_shared_library PROPERTIES PREFIX "lib64")

set(emugl_common_host_unittests_src shared_library_unittest.cpp stringparsing_unittest.cpp)
android_add_test(emugl_common_host_unittests)
android_target_link_libraries(emugl_common_host_unittests all PRIVATE emugl_base)
android_target_link_libraries(emugl_common_host_unittests
                              all
                              PUBLIC
                              emugl_test_shared_library
                              emugl_common
                              gmock_main)
android_target_include_directories(emugl_common_host_unittests all PRIVATE ${ANDROID_EMUGL_DIR}/host/include
                                   ${ANDROID_EMUGL_DIR}/shared)
