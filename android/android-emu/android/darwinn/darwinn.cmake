if(LINUX_X86_64)
  prebuilt(DARWINN)
  android_add_library(TARGET darwinn LICENSE "Apache-2.0"
                      SRC android/darwinn/darwinn-service.cpp)
  target_compile_options(darwinn PUBLIC ${DARWINN_COMPILE_DEFINITIONS})

  target_compile_options(darwinn PRIVATE -Wno-macro-redefined)
  target_link_libraries(darwinn PUBLIC darwinnmodelconfig darwinnpipe
                                       libprotobuf)
  target_include_directories(
    darwinn PUBLIC ${DARWINN_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}
                   ${CMAKE_CURRENT_BINARY_DIR})

  android_add_test(
    TARGET darwinn_unittests
    SRC # This file seems to be necessary to make the unittest program compile
        android/emulation/AndroidAsyncMessagePipe_unittest.cpp
        # Darwinn pipe unittest files
        android/darwinn/DarwinnPipe_unittest.cpp
        android/darwinn/MockDarwinnDriver.cpp
        android/darwinn/MockDarwinnDriverFactory.cpp
        # Third party file essential for compiling the program
        android/darwinn/external/src/third_party/darwinn/port/default/logging.cc
        android/darwinn/external/src/third_party/darwinn/port/default/status.cc
        android/darwinn/external/src/third_party/darwinn/port/default/statusor.cc
        android/darwinn/external/src/third_party/darwinn/api/buffer.cc)

  target_compile_options(darwinn_unittests PRIVATE -O0)
  android_target_compile_options(
    darwinn_unittests Clang PRIVATE -Wno-invalid-constexpr -Wno-macro-redefined)
  target_compile_definitions(darwinn_unittests PRIVATE -DGTEST_HAS_RTTI=0)
  target_link_libraries(darwinn_unittests PRIVATE android-emu libqemu2-glue
                                                  gmock_main)
endif()
