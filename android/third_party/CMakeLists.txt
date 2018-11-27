cmake_minimum_required(VERSION 3.5)
add_library(emulator-tinyobjloader ../../../tinyobjloader/tiny_obj_loader.cc)
target_include_directories(emulator-tinyobjloader PUBLIC ../../../tinyobjloader)

# We cannot run the the tests when we are cross compiling
if(ANDROID_TARGET_TAG MATCHES "windows.*")
  set(OPTION_ASTC_TESTS OFF CACHE BOOL "No tests astc when cross-compiling" FORCE)
endif()
# Include astc-codec as a subproject.
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../astc-codec astc-codec)

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

add_library(picosha2 INTERFACE)
target_include_directories(picosha2 INTERFACE picosha2)

if(WIN32 OR ("${ANDROID_TARGET_TAG}" MATCHES "windows.*") )
  add_subdirectory(mman-win32)
  add_subdirectory(regex-win32)
  if("${ANDROID_TARGET_TAG}" MATCHES "windows_msvc.*")
    add_subdirectory(dirent-win32)
  endif()
endif()
