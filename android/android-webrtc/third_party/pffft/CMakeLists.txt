android_add_library(
  TARGET webrtc_pffft
  LICENSE
    FFTPACK
    URL
    "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/android/android-webrtc/third_party/webrtc_pffft/LICENSE"
  SRC # cmake-format: sortable
      src/pffft.c)
target_include_directories(
  webrtc_pffft
  PUBLIC src ${CMAKE_CURRENT_BINARY_DIR}
         # Some folks expect us in third_party
         ../..)
android_target_compile_definitions(webrtc_pffft windows_msvc-x86_64
                                   PUBLIC _USE_MATH_DEFINES)

android_add_test(TARGET webrtc_pffft_unittest SRC pffft_unittest.cc
                                                  src/fftpack.c)
target_link_libraries(webrtc_pffft_unittest PRIVATE webrtc_pffft gtest_main)

message("Creating webrtc symlinks ${CMAKE_CURRENT_BINARY_DIR}/testing/")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testing)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink
          "${AOSP_ROOT}/external/googletest/googletest/" "gtest"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/testing)
# Define this for arm.. defines = [ "webrtc_pffft_SIMD_DISABLE" ]
