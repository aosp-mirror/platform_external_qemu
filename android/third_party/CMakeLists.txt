add_subdirectory(googletest)
add_subdirectory(breakpad)
android_add_library(
  TARGET emulator-tinyobjloader
  LIBNAME
    tinyobjloader
    URL
    "https://android.googlesource.com/platform/external/tinyobjloader/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../tinyobjloader"
  NOTICE "REPO/LICENSE"
  LICENSE "MIT"
  SRC # cmake-format: sortable
      ${ANDROID_QEMU2_TOP_DIR}/../tinyobjloader/tiny_obj_loader.cc)
target_include_directories(emulator-tinyobjloader PUBLIC ../../../tinyobjloader)

# We cannot run the the tests when we are cross compiling
if(CROSSCOMPILE)
  set(OPTION_ASTC_TESTS OFF CACHE BOOL "No tests astc when cross-compiling"
                                  FORCE)
endif()
# Include astc-codec as a subproject.

# astc-codec defines CMAKE_CXX_STANDARD=11, which won't work with msvc. We did
# the same for googletest.
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../astc-codec/src/base
                 astc-codec-base)
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../astc-codec/src/decoder
                 astc-codec-decoder)
android_license(
  TARGET astc-codec astc_utils footprint base
  LIBNAME astc-codec
  SPDX Apache-2.0
  URL https://android.googlesource.com/platform/external/astc-codec/+/refs/heads/master
  LICENSE
    https://android.googlesource.com/platform/external/astc-codec/+/refs/heads/master/LICENSE
  LOCAL ${ANDROID_QEMU2_TOP_DIR}/../astc-codec/LICENSE)

# Our magical toolchain will likely not create runnable executables due to
# libc++ dependencies. So we must make sure we set the runtime_os_properties.
#
# TODO(jansene): Once we are using cmake > 3.7 we can replace these lines with
# android_add_subdirectory as this will do proper target discovery.
if(OPTION_ASTC_TESTS)
  set(DECLARED_TESTS
      base_test
      physical_astc_block_test
      partition_test
      integer_sequence_codec_test
      intermediate_astc_block_test
      quantization_test
      weight_infill_test
      endpoint_codec_test
      logical_astc_block_test
      codec_test
      footprint_test)
  foreach(TST ${DECLARED_TESTS})
    android_target_properties(${TST} all "${RUNTIME_OS_PROPERTIES}")
  endforeach()
endif()

add_subdirectory(libusb)
add_subdirectory(zlib)
add_subdirectory(protobuf)
add_subdirectory(libpng)
add_subdirectory(lz4)
add_subdirectory(libcurl)
add_subdirectory(jpeg-6b)
add_subdirectory(libdtb)
add_subdirectory(libselinux)
add_subdirectory(libslirp)
add_subdirectory(libsparse)
add_subdirectory(libwebp)
add_subdirectory(murmurhash)
add_subdirectory(tinyepoxy)
add_subdirectory(libkeymaster3)
add_subdirectory(ext4_utils)
add_subdirectory(google-benchmark)
add_subdirectory(mksdcard)
add_subdirectory(boringssl)
add_subdirectory(hostapd)
add_subdirectory(tink)
add_subdirectory(re2)

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  find_program(MIG_EXE "mig")
  if(NOT MIG_EXE)
    message(
      WARNING
        "Crashpad requires gnu/mig, which we do not have for our cross compile toolchain. Dropping support"
    )
    set(DISABLE_CRASHPAD TRUE)
  endif()
endif()

if(NOT DISABLE_CRASHPAD)
  add_subdirectory(crashpad)
endif()

add_subdirectory(modem-simulator)
add_library(picosha2 INTERFACE)
target_include_directories(picosha2 INTERFACE picosha2)

android_license(
  TARGET picosha2
  LIBNAME picosha2
  URL "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev/android/third_party/picosha2/"
  SPDX "MIT"
  LICENSE
    "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev/android/third_party/picosha2/LICENSE"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/android/third_party/picosha2/LICENSE")

if(WINDOWS)
  add_subdirectory(mman-win32)
  add_subdirectory(regex-win32)
  if(WINDOWS_MSVC_X86_64)
    add_subdirectory(dirent-win32)
  endif()
  add_subdirectory(usbpassthrough-prebuilt-win32)
endif()

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(json)

if(EXISTS ${ANDROID_QEMU2_TOP_DIR}/../perfetto AND (NOT LINUX_AARCH64)
   AND (NOT CROSSCOMPILE))
  add_subdirectory(perfetto-tracing-only)
else()
  add_subdirectory(perfetto-tracing-only-stub)
endif()

add_subdirectory(abseil-cpp)

# libevent is needed for bluetooth, and webrtc in case of linux.
if(LINUX_X86_64 OR LINUX_AARCH64)
  add_subdirectory(libevent)
endif()

if(NOT TARGET flatc)
  include(ExternalProject)
  set(FLATC_BINARY_DIR)
  cmake_path(APPEND CMAKE_CURRENT_BINARY_DIR flatbuffers-flatc OUTPUT_VARIABLE
             FLATC_BINARY_DIR)
  set(FLATC_BINARY_PATH)
  cmake_path(APPEND FLATC_BINARY_DIR flatc OUTPUT_VARIABLE FLATC_BINARY_PATH)
  externalproject_add(
    flatbuffers-flatc
    SOURCE_DIR ${AOSP_ROOT}/external/flatbuffers
    BINARY_DIR ${FLATC_BINARY_DIR}
    CMAKE_ARGS -DFLATBUFFERS_BUILD_TESTS=OFF -DFLATBUFFERS_BUILD_CPP17=ON
               -DFLATBUFFERS_BUILD_FLAT_LIB=OFF -DFLATBUFFERS_BUILD_FLATC=ON
               -DFLATBUFFERS_STATIC_FLATC=OFF -DFLATBUFFERS_BUILD_FLATHASH=OFF
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${FLATC_BINARY_PATH})
  add_executable(flatc IMPORTED GLOBAL)
  set_property(TARGET flatc PROPERTY IMPORTED_LOCATION ${FLATC_BINARY_PATH})
  add_dependencies(flatc flatbuffers-flatc)
endif()

if(NOT TARGET flatbuffers)
  option(FLATBUFFERS_BUILD_TESTS "" OFF)
  option(FLATBUFFERS_BUILD_CPP17 "" ON)
  option(FLATBUFFERS_BUILD_FLAT_LIB "" ON)
  option(FLATBUFFERS_BUILD_FLATC "" OFF)
  option(FLATBUFFERS_STATIC_FLATC "" OFF)
  option(FLATBUFFERS_BUILD_FLATHASH "" OFF)
  add_subdirectory(${AOSP_ROOT}/external/flatbuffers flatbuffers-flatlib)
endif()

android_license(
  LIBNAME flatbuffers URL "https://github.com/google/flatbuffers"
  TARGET flatc flatbuffers SPDX Apache-2.0
  LOCAL ${AOSP_ROOT}/external/flatbuffers/LICENSE.txt LICENSE Apache-2.0)

# Let's bring the grpc module.
cmake_policy(SET CMP0079 NEW)

set(_gRPC_RE2_LIBRARIES re2)
set(_gRPC_RE2_INCLUDE_DIR "${ANDROID_QEMU2_TOP_DIR}/android/third_party/re2")

add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../cares cares)
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../grpc/emulator grpc)

if(LINUX_X86_64)
  # Our linux headers are from 2013, and do not define newer socket options.
  # (b/156635589)
  target_compile_options(grpc PRIVATE -DSO_REUSEPORT=15)
  target_compile_options(grpc_unsecure PRIVATE -DSO_REUSEPORT=15)
endif()

if(FEATURE_NETSIM)
  set(EXTERNAL_QEMU ${ANDROID_QEMU2_TOP_DIR})
  add_subdirectory(${AOSP_ROOT}/tools/netsim netsim)
  add_subdirectory(mynewt-nimble)

  if(TARGET Rust::Rustc)
    add_subdirectory(${AOSP_ROOT}/packages/modules/Bluetooth/tools/rootcanal
                     rootcanal)
  endif()

  if(DARWIN_AARCH64 AND NOT TARGET Rust::Rustc AND OPTION_RUST)
    message(STATUS "Cross compiling netsim to target Rosetta X86_64")
    include(ExternalProject)
    set(NETSIM_CROSS_DIR
        ${ANDROID_CROSS_BUILD_DIRECTORY}/netsim_ext_cross/src/netsim_ext_cross-build/
    )
    externalproject_add(
      netsim_ext_cross
      PREFIX ${ANDROID_CROSS_BUILD_DIRECTORY}/netsim_ext_cross
      DOWNLOAD_COMMAND ""
      SOURCE_DIR ${AOSP_ROOT}/tools/netsim
      CMAKE_ARGS
        "-DCMAKE_TOOLCHAIN_FILE=${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/toolchain-darwin-x86_64.cmake"
        "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        "-DBREAKPAD_API_KEY=${BREAKPAD_API_KEY}"
        "-DBREAKPAD_API_URL=${BREAKPAD_API_URL}"
        "-DANDROID_SYMBOL_DIR=${ANDROID_SYMBOL_DIR}"
        "-DPython_EXECUTABLE=${Python_EXECUTABLE}"
        "-DCLANG_COMPILER_VERSION=${CLANG_COMPILER_VERSION}"
        "-DRUST_COMPILER_VERSION=${RUST_COMPILER_VERSION}"
        "-DNETSIM_EXT=TRUE"
      BUILD_BYPRODUCTS "${NETSIM_CROSS_DIR}/netsim;${NETSIM_CROSS_DIR}/netsimd"
      LOG_BUILD TRUE
      LOG_INSTALL TRUE
      LOG_OUTPUT_ON_FAILURE TRUE
      INSTALL_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      INSTALL_COMMAND "")
    install(PROGRAMS ${NETSIM_CROSS_DIR}/netsim ${NETSIM_CROSS_DIR}/netsimd
            DESTINATION .)
  endif()

  # Add netsim-ui into objs
  set(NETSIM_GUI_STATIC
    ${AOSP_ROOT}/tools/netsim/ui/dist/index.html>netsim-ui/index.html;
    ${AOSP_ROOT}/tools/netsim/ui/dist/dev.html>netsim-ui/dev.html;
    ${AOSP_ROOT}/tools/netsim/ui/dist/assets/grid-background.svg>netsim-ui/assets/grid-background.svg;
    ${AOSP_ROOT}/tools/netsim/ui/dist/assets/hexagonal-background.png>netsim-ui/assets/hexagonal-background.png;
    ${AOSP_ROOT}/tools/netsim/ui/dist/assets/netsim-logo-b.svg>netsim-ui/assets/netsim-logo-b.svg;
    ${AOSP_ROOT}/tools/netsim/ui/dist/assets/netsim-logo.svg>netsim-ui/assets/netsim-logo.svg;
    ${AOSP_ROOT}/tools/netsim/ui/dist/assets/polar-background.svg>netsim-ui/assets/polar-background.svg;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/cube-sprite.js>netsim-ui/js/cube-sprite.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/customize-map-button.js>netsim-ui/js/customize-map-button.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/device-dragzone.js>netsim-ui/js/device-dragzone.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/device-dropzone.js>netsim-ui/js/device-dropzone.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/device-info.js>netsim-ui/js/device-info.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/device-list.js>netsim-ui/js/device-list.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/device-map.js>netsim-ui/js/device-map.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/device-observer.js>netsim-ui/js/device-observer.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/license-info.js>netsim-ui/js/license-info.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/model.js>netsim-ui/js/model.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/navigation-bar.js>netsim-ui/js/navigation-bar.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/netsim-app.js>netsim-ui/js/netsim-app.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/packet-info.js>netsim-ui/js/packet-info.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/js/pyramid-sprite.js>netsim-ui/js/pyramid-sprite.js;
    ${AOSP_ROOT}/tools/netsim/ui/dist/node_modules/tslib/tslib.es6.js>netsim-ui/node_modules/tslib/tslib.es6.js;
    PARENT_SCOPE
  )
  android_license(TARGET "NETSIM_GUI_STATIC" LIBNAME None SPDX None LICENSE None LOCAL None)
else()
  message(STATUS "Netsim is not enabled.")
endif()

# gfxstream, aemu-common modules
set(GFXSTREAM_REPO_ROOT
    ${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/gfxstream)
set(ENABLE_VKCEREAL_TESTS ON)
function(discover_gfxstream_internal_targets result subdir)
  if(CMAKE_VERSION VERSION_LESS "3.7.0")
    message(
      FATAL_ERROR
        "This function cannot be used by older cmake versions (${CMAKE_VERSION}<3.7.0)"
    )
  endif()
  message("gfxstream_internal_targets result=${result} subdir=${subdir}")
  get_filename_component(abs_subdir ${subdir} ABSOLUTE)
  set(GFXSTREAM_THIRD_PARTY_ROOT ${GFXSTREAM_REPO_ROOT})
  cmake_path(APPEND GFXSTREAM_THIRD_PARTY_ROOT "third-party")
  cmake_path(IS_PREFIX GFXSTREAM_THIRD_PARTY_ROOT ${abs_subdir} NORMALIZE
             IS_THIRD_PARTY)
  if(IS_THIRD_PARTY)
    return()
  endif()
  cmake_path(IS_PREFIX GFXSTREAM_REPO_ROOT ${abs_subdir} NORMALIZE
             IS_GFXSTREAM_SUBDIR)
  cmake_path(IS_PREFIX AEMU_COMMON_REPO_ROOT ${abs_subdir} NORMALIZE
             IS_AEMU_COMMON_SUBDIR)
  cmake_path(IS_PREFIX GFXSTREAM_PROTOCOLS_DIR ${abs_subdir} NORMALIZE
             IS_GFXSTREAM_PROTOCOLS_SUBDIR)
  if(NOT IS_GFXSTREAM_SUBDIR AND NOT IS_AEMU_COMMON_SUBDIR
     AND NOT IS_GFXSTREAM_PROTOCOLS_SUBDIR)
    return()
  endif()

  get_directory_property(subdirs DIRECTORY "${abs_subdir}" SUBDIRECTORIES)
  foreach(subdir IN LISTS subdirs)
    discover_gfxstream_internal_targets(${result} "${subdir}")
  endforeach()

  get_property(targets_in_dir DIRECTORY "${subdir}"
               PROPERTY BUILDSYSTEM_TARGETS)
  set(${result} ${${result}} ${targets_in_dir} PARENT_SCOPE)
endfunction()
set(GFXSTREAM_PREFIX "gfxstream")
set(ANGLE_REPO_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../external/angle)
add_library(gfxstream_egl_headers INTERFACE)
target_include_directories(gfxstream_egl_headers
                           INTERFACE ${ANGLE_REPO_ROOT}/include)
add_subdirectory(${GFXSTREAM_REPO_ROOT}/gl-host-common
                 ${GFXSTREAM_PREFIX}-gl-host-common)
# Include external/gfxstream-protocols project
add_subdirectory(${GFXSTREAM_REPO_ROOT}/common
                 ${GFXSTREAM_PREFIX}-common)
# Include hardware/google/aemu project
set(AEMU_COMMON_BUILD_CONFIG "qemu-android" CACHE STRING "")
set(GFXSTREAM_REPO_ROOT
    ${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/gfxstream)
set(GFXSTREAM_PROTOCOLS_DIR
    ${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/gfxstream)
set(AEMU_COMMON_REPO_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/aemu)
add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/aemu
                 aemu_common)
if(ENABLE_VKCEREAL_TESTS)
  set(AEMU_COMMON_TESTS aemu-base_unittests aemu-host-common_unittests)
  foreach(TST ${AEMU_COMMON_TESTS})
    android_target_properties(${TST} all "${RUNTIME_OS_PROPERTIES}")
  endforeach()
endif()

# Figure out all gfxstream dependencies
include(${GFXSTREAM_REPO_ROOT}/cmake/SetSubdirectorProperties.cmake)
get_directory_property(GFXSTREAM_SUBDIRS SUBDIRECTORIES)
foreach(GFXSTREAM_SUBDIR IN LISTS GFXSTREAM_SUBDIRS)
  discover_gfxstream_internal_targets(GFXSTREAM_INTERNAL_TARGETS
                                      ${GFXSTREAM_SUBDIR})
endforeach()
set(GFXSTREAM_DEPENDENCIES ${GFXSTREAM_INTERNAL_TARGETS} angle_common
                           angle_shader_translator renderdoc)
android_license(
  TARGET "${GFXSTREAM_DEPENDENCIES}"
  LIBNAME gfxstream
  URL "https://android.googlesource.com/platform/hardware/google/gfxstream/+/refs/heads/master"
  SPDX Apache-2.0
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/../../hardware/google/gfxstream/LICENSE"
  LICENSE
    "https://android.googlesource.com/platform/hardware/google/gfxstream/+/refs/heads/master/LICENSE"
)
# Used by gfxstream but has different license.
android_license(
  TARGET "libdrm_headers"
  LIBNAME None
  SPDX MIT
  LICENSE "MIT"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/../libdrm/LICENSE")
