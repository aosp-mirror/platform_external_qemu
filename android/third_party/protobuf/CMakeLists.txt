cmake_minimum_required(VERSION 3.5)
if(NOT ANDROID_QEMU2_TOP_DIR)
  set(CMAKE_CXX_STANDARD 17)
  project(protobuf)
  include(android)
  include(prebuilts)
endif()

if(TARGET libprotobuf)
  # No need to include it twice.
  return()
endif()

# Display version information etc.
set(protobuf_VERBOSE TRUE)
if(NOT TARGET zlib)
  # Cross build needs zlib for protoc.
  add_subdirectory(../zlib zlib)
endif()

# Needed for the crosscompile case..
if(NOT TARGET absl::strings)
  add_subdirectory(../abseil-cpp abseil)
endif()

set(ZLIB_LIBRARIES zlib)
set(protobuf_DEBUG_POSTFIX "d" CACHE STRING "Default debug postfix")
set(protobuf_SOURCE_DIR "${ANDROID_QEMU2_TOP_DIR}/../protobuf")

if(protobuf_BUILD_SHARED_LIBS)
  set(protobuf_SHARED_OR_STATIC "SHARED")
else(protobuf_BUILD_SHARED_LIBS)
  set(protobuf_SHARED_OR_STATIC "STATIC")
endif()

if(WINDOWS_MSVC_X86_64)
  add_definitions(-utf-8)
  add_definitions(-fbigobj)
endif()

add_definitions(-DUNICODE -D_UNICODE)

cmake_policy(SET CMP0079 NEW)
set(protobuf_BUILD_CONFORMANCE FALSE)
set(protobuf_BUILD_EXAMPLES FALSE)
set(protobuf_INSTALL FALSE)
set(protobuf_BUILD_LIBUPB FALSE)

set(protobuf_BUILD_TESTS FALSE)
set(protobuf_BUILD_PROTOBUF_BINARIES TRUE)
set(protobuf_BUILD_PROTOC_BINARIES TRUE)
set(protobuf_BUILD_LIBPROTOC TRUE)
set(protobuf_DISABLE_RTTI TRUE)

if(NOT CROSSCOMPILE)
  add_subdirectory(${protobuf_SOURCE_DIR} protobuf)
else()

  include(ExternalProject)

  # If we are cross compiling we will need to build it for our actual OS we are
  # currently running on.
  get_filename_component(
    PROTOC
    ${ANDROID_CROSS_BUILD_DIRECTORY}/protobuf_ext_cross/src/protobuf_ext_cross-build/protobuf/protoc
    ABSOLUTE)
  message(STATUS "Cross compiling protobuf for host ${ANDROID_HOST_TAG}")
  externalproject_add(
    protobuf_ext_cross
    PREFIX ${ANDROID_CROSS_BUILD_DIRECTORY}/protobuf_ext_cross
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    CMAKE_ARGS
      "-DCMAKE_TOOLCHAIN_FILE=${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/toolchain-${ANDROID_HOST_TAG}.cmake"
      "-DIN_CROSS_COMPILE=TRUE"
    BUILD_BYPRODUCTS ${PROTOC}
    LOG_OUTPUT_ON_FAILURE TRUE
    INSTALL_COMMAND "")

  set(protobuf_BUILD_PROTOC_BINARIES FALSE)
  add_subdirectory(${protobuf_SOURCE_DIR} protobuf)

  message(STATUS "Using protoc: ${PROTOC}")
  add_executable(protobuf::protoc IMPORTED GLOBAL)
  set_target_properties(protobuf::protoc PROPERTIES IMPORTED_LOCATION
                                                    "${PROTOC}")

  set(PROTOBUF_PROTOC_EXECUTABLE ${PROTOC}
      CACHE PATH "Protocol buffer executable" FORCE)

endif()

android_license(
  TARGET "protobuf::libprotobuf" LIBNAME protobuf SPDX "BSD-3-Clause"
  LICENSE "BSD-3-Clause" LOCAL "${protobuf_SOURCE_DIR}/NOTICE")

android_license(TARGET "utf8_validity" LIBNAME protobuf SPDX "MIT" LICENSE "MIT"
                LOCAL "${protobuf_SOURCE_DIR}/third_party/utf8_range/NOTICE")
android_license(TARGET "libprotobuf" LIBNAME protobuf SPDX "BSD-3-Clause"
                LICENSE "BSD-3-Clause" LOCAL "${protobuf_SOURCE_DIR}/NOTICE")

# external/tink links to libprotobuf-lite, but we only want to use libprotobuf.
add_library(protobuf::libprotobuf-lite ALIAS libprotobuf)

# In the cross compile case we will have to make sure that what symlink the
# lib64 directory
if(IN_CROSS_COMPILE)
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_BINARY_DIR}/lib64"
      "${CMAKE_CURRENT_BINARY_DIR}/protobuf/lib64")
endif()
