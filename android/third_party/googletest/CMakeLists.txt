# Let's not install gtest..
# set(INSTALL_GTEST OFF CACHE BOOL "Never install gtest" FORCE)
set(GOOGLETEST_VERSION 1.10.0)
set(gtest_SOURCE_DIR ${ANDROID_QEMU2_TOP_DIR}/../googletest/googletest)
set(gmock_SOURCE_DIR ${ANDROID_QEMU2_TOP_DIR}/../googletest/googlemock)

set(gtest_build_include_dirs "${gtest_SOURCE_DIR}/include"
                             "${gtest_SOURCE_DIR}")

# Adds Google Mock's and Google Test's header directories to the search path.
set(gmock_build_include_dirs
    "${gmock_SOURCE_DIR}/include"
    "${gmock_SOURCE_DIR}"
    "${gtest_SOURCE_DIR}/include"
    # This directory is needed to build directly from Google Test sources.
    "${gtest_SOURCE_DIR}")

android_add_library(
  TARGET gtest
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      ${gtest_SOURCE_DIR}/src/gtest-all.cc)

set_target_properties(gtest PROPERTIES VERSION ${GOOGLETEST_VERSION})
target_include_directories(gtest PUBLIC ${gtest_build_include_dirs})

android_add_library(
  TARGET gtest_main
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      ${gtest_SOURCE_DIR}/src/gtest_main.cc)

set_target_properties(gtest_main PROPERTIES VERSION ${GOOGLETEST_VERSION})
target_include_directories(gtest_main PUBLIC ${gtest_build_include_dirs})
target_link_libraries(gtest_main PRIVATE gtest PUBLIC Threads::Threads)

android_add_library(
  TARGET gmock
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      "${gmock_SOURCE_DIR}/src/gmock-all.cc")
target_include_directories(gmock PUBLIC ${gmock_build_include_dirs})
target_link_libraries(gmock PRIVATE gtest)
target_compile_options(gmock PUBLIC -Wno-inconsistent-missing-override)

android_add_library(
  TARGET gmock_main
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      "${gmock_SOURCE_DIR}/src/gmock_main.cc")

target_include_directories(gmock_main PUBLIC ${gmock_build_include_dirs})
target_link_libraries(gmock_main PRIVATE gmock PUBLIC Threads::Threads)

if(WINDOWS_MSVC_X86_64)
  # Including the top-level CMakeLists.txt file doesn't work because the Windows
  # SDK library requires C++14 support. So let's just include this manually.

  set(gtest_disable_pthreads ON CACHE BOOL "Disable uses of pthreads in gtest.")
  # All right, we must make sure we don't start adding the /MT flag
  if(WINDOWS_MSVC_X86_64)
    set(gtest_force_shared_crt
        ON
        CACHE
          BOOL
          "Use shared (DLL) run-time lib even when Google Test is built as static lib."
          FORCE)
  endif()
  if(WINDOWS_MSVC_X86_64)
    # Okay, google test is not happy under msvc compile
    target_compile_options(gtest PRIVATE "-Wno-everything")
    target_compile_options(gmock PRIVATE "-Wno-everything")
    target_compile_definitions(
      gmock_main PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
    target_compile_definitions(
      gmock PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
    target_compile_definitions(
      gtest PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
  endif()
else()
  # Enable type information and exceptions in the test framework, and compile it
  # with full debug info
  target_compile_options(gmock_main PRIVATE -frtti -O0 -fexceptions)
  target_compile_options(gmock PRIVATE -frtti -O0 -fexceptions)
  target_compile_options(gtest PRIVATE -frtti -O0 -fexceptions)
endif()

if(WINDOWS_MSVC_X86_64)
  # Don't include the custom TempDir function. It is for Android only.
  target_compile_definitions(
    gtest PRIVATE -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
endif()
