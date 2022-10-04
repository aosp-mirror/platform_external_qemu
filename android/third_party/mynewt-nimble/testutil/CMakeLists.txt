android_add_library(TARGET _nimble_test_util LICENSE Apache-2.0
                    SRC src/case.c src/suite.c src/testutil.c
                    DEPS android-emu-base-headers)
target_link_libraries(_nimble_test_util PUBLIC _nimble_os_abstraction_headers
                                               _nimble_os_abstraction)
target_include_directories(_nimble_test_util PUBLIC include PRIVATE src)
target_compile_options(
  _nimble_test_util PRIVATE -Wno-implicit-function-declaration
                            -Wno-parentheses-equality -Wno-int-to-pointer-cast)
