# Place your compiler tests that assure that the build is behaving as expected across
# various platforms here.
#
# For example: bit fields behave differently accross windows/posix.
# Tests here will make sure that if we change compilers again that we catch failures early.
android_add_test(
  TARGET win_clang_unittests
  SRC # cmake-format: sortable
      compiler_tests.cpp enum_bit_fields.c longjump_test.c)

# the longjump errors manifest with -O2
set_source_files_properties(longjump_test.c PRIVATE COMPILE_FLAGS "-O2")
target_include_directories(
  win_clang_unittests
  PRIVATE ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-x86_64
          ${ANDROID_QEMU2_TOP_DIR}/target/i386
          ${ANDROID_AUTOGEN}
          ${ANDROID_AUTOGEN}/tcg)

target_link_libraries(win_clang_unittests PRIVATE android-qemu-deps gmock_main
                                                  glib2)
target_compile_definitions(win_clang_unittests PRIVATE -DNEED_CPU_H)
