# Let's not install gtest..
set(INSTALL_GTEST OFF CACHE BOOL "Never install gtest" FORCE)

if (WINDOWS_MSVC_X86_64)
  # Including the top-level CMakeLists.txt file doesn't work because the Windows SDK
  # library requires C++14 support. So let's just include this manually.

  set(gtest_disable_pthreads ON CACHE BOOL "Disable uses of pthreads in gtest.")
  # All right, we must make sure we don't start adding the /MT flag
  if (WINDOWS_MSVC_X86_64)
    set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib." FORCE)
  endif()
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../googletest/googlemock googlemock)
  if(WINDOWS_MSVC_X86_64)
    # Okay, google test is not happy under msvc compile
    target_compile_options(gtest PRIVATE "-Wno-sign-conversion")
    target_compile_options(gmock PRIVATE "-Wno-sign-conversion")
    target_compile_definitions(gmock_main PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
    target_compile_definitions(gmock PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
    target_compile_definitions(gtest PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
  endif()
else()
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../googletest googletest)
  # Enable type information and exceptions in the test framework, and compile it with full debug info
  target_compile_options(gmock_main PRIVATE -frtti -O0  -fexceptions)
  target_compile_options(gmock PRIVATE -frtti -O0 -fexceptions)
  target_compile_options(gtest PRIVATE -frtti -O0 -fexceptions)
endif()

if (WINDOWS_MSVC_X86_64)
    # Don't include the custom TempDir function. It is for Android only.
	target_compile_definitions(gtest PRIVATE
            -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
endif ()


