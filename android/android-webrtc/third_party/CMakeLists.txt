set(WEBRTC_ROOT "${ANDROID_QEMU2_TOP_DIR}/../webrtc")
add_library(
  webrtc_spl_sqrt_floor
  ${WEBRTC_ROOT}/common_audio/third_party/spl_sqrt_floor/spl_sqrt_floor.c)
target_include_directories(webrtc_spl_sqrt_floor PUBLIC ${WEBRTC_ROOT})

add_library(
  webrtc_fft_size_128 # cmake-format: sortable
  ${WEBRTC_ROOT}/common_audio/third_party/ooura/fft_size_128/ooura_fft.cc
  # Remove for arm..
  ${WEBRTC_ROOT}/common_audio/third_party/ooura/fft_size_128/ooura_fft_sse2.cc)
target_include_directories(webrtc_fft_size_128 PUBLIC ${WEBRTC_ROOT})

add_library(webrtc_fft_size_256
            ${WEBRTC_ROOT}/common_audio/third_party/ooura/fft_size_256/fft4g.cc)
target_include_directories(webrtc_fft_size_256 PUBLIC ${WEBRTC_ROOT})

add_library(webrtc_fft ${WEBRTC_ROOT}/modules/third_party/fft/fft.c)
target_include_directories(webrtc_fft PUBLIC ${WEBRTC_ROOT})

android_add_library(TARGET emulator_test_overrides NODISTRIBUTE
                    SRC emu_file_utils_override.cc)
target_link_libraries(
  emulator_test_overrides
  PRIVATE android-emu-base webrtc_test_fileutils_override_api absl::optional)
target_compile_definitions(
  emulator_test_overrides
  PRIVATE "EMU_THIRD_PARTY_WEBRTC=\"${CMAKE_CURRENT_LIST_DIR}\""
          "EMU_OBJS_DIR=\"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\"")

if(LINUX_X86_64 OR LINUX_AARCH64)
    add_subdirectory(libaom)
endif()

add_subdirectory(libvpx)
add_subdirectory(jsoncpp)
add_subdirectory(pffft)
add_subdirectory(opus)
add_subdirectory(rnnoise)
add_subdirectory(webrtc)
add_subdirectory(usrsctp)
add_subdirectory(libsrtp)
add_subdirectory(catapult/tracing)
add_subdirectory(libyuv)
