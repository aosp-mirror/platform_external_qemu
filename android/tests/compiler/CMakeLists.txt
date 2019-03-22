
# This test needs to turn green in order for qemu to have a chance
# at booting arm.
set(win_clang_unittests_src enum_bit_fields.cpp)
android_add_test(win_clang_unittests)
target_include_directories(win_clang_unittests PRIVATE ${ANDROID_QEMU2_TOP_DIR})
target_link_libraries(win_clang_unittests PRIVATE gmock_main)