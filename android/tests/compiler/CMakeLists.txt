
# Place your compiler tests that assure that the build is behaving as expected across
# various platforms here.
#
# For example: bit fields behave differently accross windows/posix.
# Tests here will make sure that if we change compilers again that we catch failures early.
set(win_clang_unittests_src enum_bit_fields.c enum_bit_fields.cpp)
android_add_test(win_clang_unittests)

target_include_directories(win_clang_unittests PRIVATE
        ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-x86_64
        ${ANDROID_QEMU2_TOP_DIR}/target/i386
        ${ANDROID_QEMU2_TOP_DIR}/qemu2-auto-generated
        ${ANDROID_QEMU2_TOP_DIR}/qemu2-auto-generated/tcg)

target_link_libraries(win_clang_unittests PRIVATE android-qemu-deps gmock_main)
target_compile_definitions(win_clang_unittests PRIVATE -DNEED_CPU_H)
