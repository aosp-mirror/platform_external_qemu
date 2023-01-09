# This coverage-poc directory is just a proof-of-concept on code coverage and using it in gerrit.

android_add_library(
  TARGET test1
  LICENSE Apache-2.0
  SRC test1_lib.cpp)

target_include_directories(test1 PUBLIC .)
target_compile_options(test1 PRIVATE -fprofile-instr-generate -fcoverage-mapping)

android_add_test(
  TARGET test1-unittests
  SRC test1_unittest.cpp)

target_link_libraries(test1-unittests PRIVATE test1 gmock_main)
target_link_libraries(test1-unittests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
