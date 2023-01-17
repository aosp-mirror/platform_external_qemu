# This coverage-poc directory is just a proof-of-concept on code coverage and using it in gerrit.

android_add_library(
  TARGET test2
  LICENSE Apache-2.0
  SRC test2_lib.cpp)

target_include_directories(test2 PUBLIC .)
target_compile_options(test2 PRIVATE -fprofile-instr-generate -fcoverage-mapping)

android_add_test(
  TARGET test2-unittests
  SRC test2_unittest.cpp)

target_link_libraries(test2-unittests PRIVATE test2 gmock_main)
target_link_libraries(test2-unittests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
