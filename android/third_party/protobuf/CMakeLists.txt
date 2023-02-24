cmake_minimum_required(VERSION 3.5)
if(NOT ANDROID_QEMU2_TOP_DIR)
  set(CMAKE_CXX_STANDARD 17)
  project(protobuf)
  include(android)
  include(prebuilts)
endif()

# Display version information etc.
set(protobuf_VERBOSE TRUE)
if(NOT TARGET zlib)
  # Cross build needs zlib for protoc.
  add_subdirectory(../zlib zlib)
endif()

set(ZLIB_LIBRARIES zlib)

set(protobuf_DEBUG_POSTFIX "d" CACHE STRING "Default debug postfix")

set(protobuf_SOURCE_DIR "${ANDROID_QEMU2_TOP_DIR}/../protobuf")
# Path to main configure script
set(protobuf_CONFIGURE_SCRIPT "${protobuf_SOURCE_DIR}/configure.ac")

# Parse configure script
set(protobuf_AC_INIT_REGEX
  "^AC_INIT\\(\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\]\\)$")
file(STRINGS "${protobuf_CONFIGURE_SCRIPT}" protobuf_AC_INIT_LINE
  LIMIT_COUNT 1 REGEX "^AC_INIT")
# Description
string(REGEX REPLACE        "${protobuf_AC_INIT_REGEX}" "\\1"
    protobuf_DESCRIPTION    "${protobuf_AC_INIT_LINE}")
# Version
string(REGEX REPLACE        "${protobuf_AC_INIT_REGEX}" "\\2"
    protobuf_VERSION_STRING "${protobuf_AC_INIT_LINE}")
# Contact
string(REGEX REPLACE        "${protobuf_AC_INIT_REGEX}" "\\3"
    protobuf_CONTACT        "${protobuf_AC_INIT_LINE}")
# Parse version tweaks
set(protobuf_VERSION_REGEX "^([0-9]+)\\.([0-9]+)\\.([0-9]+)([-]rc[-]|\\.)?([0-9]*)$")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\1"
  protobuf_VERSION_MAJOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\2"
  protobuf_VERSION_MINOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\3"
  protobuf_VERSION_PATCH "${protobuf_VERSION_STRING}")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\5"
  protobuf_VERSION_PRERELEASE "${protobuf_VERSION_STRING}")

# message(STATUS "${protobuf_VERSION_PRERELEASE}")

# Package version
set(protobuf_VERSION
  "${protobuf_VERSION_MAJOR}.${protobuf_VERSION_MINOR}.${protobuf_VERSION_PATCH}")

if(protobuf_VERSION_PRERELEASE)
  set(protobuf_VERSION "${protobuf_VERSION}.${protobuf_VERSION_PRERELEASE}")
else()
  set(protobuf_VERSION "${protobuf_VERSION}.0")
endif()

if(protobuf_VERBOSE)
  message(STATUS "Configuration script parsing status [")
  message(STATUS "  Description : ${protobuf_DESCRIPTION}")
  message(STATUS "  Version     : ${protobuf_VERSION} (${protobuf_VERSION_STRING})")
  message(STATUS "  Contact     : ${protobuf_CONTACT}")
  message(STATUS "]")
endif()

add_definitions(-DGOOGLE_PROTOBUF_CMAKE_BUILD)

if(protobuf_BUILD_SHARED_LIBS)
  set(protobuf_SHARED_OR_STATIC "SHARED")
else(protobuf_BUILD_SHARED_LIBS)
  set(protobuf_SHARED_OR_STATIC "STATIC")
endif()

if(WINDOWS_MSVC_X86_64)
  add_definitions(-utf-8)
  add_definitions(-bigobj)
endif()

add_definitions(-DUNICODE -D_UNICODE)

# This definition makes sure we only build C/C++ generators
# And we can successfully compile as both shared/static library.
add_definitions(-DANDROID_EMULATOR)

include(${protobuf_SOURCE_DIR}/cmake/libprotobuf-lite.cmake)
# Linking to both libprotobuf and libprotobuf-lite will cause unexpected behavior,
# so we exclude libprotobuf-lite from the build.
set_target_properties(libprotobuf-lite PROPERTIES
    EXCLUDE_FROM_ALL TRUE)
include(${protobuf_SOURCE_DIR}/cmake/libprotobuf.cmake)
# external/tink links to libprotobuf-lite, but we only want to use libprotobuf.
add_library(protobuf::libprotobuf-lite ALIAS libprotobuf)

target_compile_options(libprotobuf PRIVATE -Wno-invalid-noreturn)
if(NOT CROSSCOMPILE)
  include(${protobuf_SOURCE_DIR}/cmake/libprotoc.cmake)
  include(${protobuf_SOURCE_DIR}/cmake/protoc.cmake)

  android_target_dependency(protoc all RUNTIME_OS_DEPENDENCIES)
  android_target_properties(protoc all "${RUNTIME_OS_PROPERTIES}")

  if(NOT DEFINED protobuf_PROTOC_EXE)
    set(protobuf_PROTOC_EXE protoc)
  endif(NOT DEFINED protobuf_PROTOC_EXE)
  set(PROTOBUF_PROTOC_EXECUTABLE "$<TARGET_FILE:protoc>"
      CACHE PATH "Protocol buffer executable" FORCE)
else()
  android_compile_for_host(protoc ${CMAKE_CURRENT_LIST_DIR} PROTOBUF_EXE)
  set(PROTOBUF_PROTOC_EXECUTABLE ${PROTOBUF_EXE}
      CACHE PATH "Protocol buffer executable" FORCE)
  add_executable(protobuf::protoc IMPORTED GLOBAL)
  set_target_properties(
    protobuf::protoc PROPERTIES IMPORTED_LOCATION
                                "${PROTOBUF_PROTOC_EXECUTABLE}")
endif()

set(PROTOBUF_LIBRARY "${PROTO_DESC}")
set(PROTOBUF_INCLUDE_DIR "${protobuf_SOURCE_DIR}/src")

# Later versions of cmake want this to be lower case..
set(Protobuf_INCLUDE_DIR "${PROTOBUF_INCLUDE_DIR}")
set(Protobuf_LIBRARY "${PROTOBUF_LIBRARY}")
set(Protobuf_PROTOC_EXECUTABLE "${PROTOBUF_PROTOC_EXECUTABLE}")

android_license(
  TARGET "protobuf::libprotobuf" LIBNAME protobuf SPDX "BSD-3-Clause"
  LICENSE "BSD-3-Clause" LOCAL "${protobuf_SOURCE_DIR}/NOTICE")
android_license(TARGET "libprotobuf" LIBNAME protobuf SPDX "BSD-3-Clause"
                LICENSE "BSD-3-Clause" LOCAL "${protobuf_SOURCE_DIR}/NOTICE")

if(protobuf_BUILD_SHARED_LIBS)
  android_install_shared(libprotobuf)
endif()
