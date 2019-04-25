add_subdirectory(googletest)
add_library(emulator-tinyobjloader ../../../tinyobjloader/tiny_obj_loader.cc)
target_include_directories(emulator-tinyobjloader PUBLIC ../../../tinyobjloader)

# We cannot run the the tests when we are cross compiling
if(CROSSCOMPILE)
  set(OPTION_ASTC_TESTS OFF CACHE BOOL "No tests astc when cross-compiling" FORCE)
endif()
# Include astc-codec as a subproject.

if(WINDOWS_MSVC_X86_64)
  # astc-codec defines CMAKE_CXX_STANDARD=11, which won't work with msvc. We did the same for googletest.
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../astc-codec/src/base astc-codec-base)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../astc-codec/src/decoder astc-codec-decoder)
else()
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../astc-codec astc-codec)
endif()

# Our magical toolchain will likely not create runnable executables due to libc++ dependencies.
# So we must make sure we set the runtime_os_properties.
#
# TODO(jansene): Once we are using cmake > 3.7 we can replace these lines with
# android_add_subdirectory as this will do proper target discovery.
if (OPTION_ASTC_TESTS)
  set(DECLARED_TESTS base_test physical_astc_block_test partition_test integer_sequence_codec_test intermediate_astc_block_test quantization_test weight_infill_test endpoint_codec_test logical_astc_block_test codec_test footprint_test)
  foreach(TST ${DECLARED_TESTS})
    android_target_properties(${TST} all "${RUNTIME_OS_PROPERTIES}")
  endforeach()
endif()

add_subdirectory(zlib)
add_subdirectory(protobuf)
add_subdirectory(libpng)
add_subdirectory(lz4)
add_subdirectory(libcurl)
add_subdirectory(jpeg-6b)
add_subdirectory(libdtb)
add_subdirectory(libselinux)
add_subdirectory(libsparse)
add_subdirectory(libwebp)
add_subdirectory(libyuv)
add_subdirectory(murmurhash)
add_subdirectory(tinyepoxy)
add_subdirectory(libkeymaster3)
add_subdirectory(ext4_utils)
add_subdirectory(google-benchmark)
add_subdirectory(mksdcard)

# Old windows & linux version will use old breakpad version, until we track down
# crash detection bugs under linux.
if (WINDOWS_X86_64 OR LINUX_X86_64)
  prebuilt(BREAKPAD)
else()
  add_subdirectory(breakpad)
endif()

# Only use boringssl for msvc, due to   b/129081460
if(WINDOWS_MSVC_X86_64)
  add_subdirectory(boringssl)
else()
  # Use prebuilt due to b/129076232
  message(STATUS "Using prebuilt openssl..")
  prebuilt(OpenSSL)
endif()

add_library(picosha2 INTERFACE)
target_include_directories(picosha2 INTERFACE picosha2)

if(WINDOWS)
  add_subdirectory(mman-win32)
  add_subdirectory(regex-win32)
  if(WINDOWS_MSVC_X86_64)
    add_subdirectory(dirent-win32)
 endif()
endif()


set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(json)