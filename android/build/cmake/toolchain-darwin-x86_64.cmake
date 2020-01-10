# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# The toolchain files get processed multiple times during compile detection keep these files simple, and do not setup
# libraries or anything else. merely tags, compiler toolchain and flags should be set here.

get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
include(toolchain)

# First we setup all the tags.
toolchain_configure_tags("darwin-x86_64")

# Make sure we always set the rpath to include current dir and ./lib64. We need this to create self contained
# executables that dynamically load dylibs.
set(RUNTIME_OS_PROPERTIES
    "INSTALL_RPATH>=@loader_path;INSTALL_RPATH>=@loader_path/lib64;BUILD_WITH_INSTALL_RPATH=ON;INSTALL_RPATH_USE_LINK_PATH=ON"
)

if(NOT APPLE)
  # We will use the osx cross compiler, and we expect it to be in /opt/osxcross/bin
  set(OSXCROSS_TARGET_DIR /opt/osxcross/osxcross)

  set(OSXCROSS_HOST x86_64-apple-darwin19)
  set(CMAKE_SYSTEM_NAME "Darwin")
  string(REGEX REPLACE "-.*" "" CMAKE_SYSTEM_PROCESSOR "${OSXCROSS_HOST}")

  # Detect sdk version
  foreach(sdk_version 10.13 10.14 10.15)
    if(EXISTS "${OSXCROSS_TARGET_DIR}/SDK/MacOSX${sdk_version}.sdk")
        set(CMAKE_OSX_DEPLOYMENT_TARGET ${sdk_version})
    endif()
  endforeach()

  # Detect cross compiler name, needed for linker config.
  foreach(darwin_ver 16 17 18 19)
    if(EXISTS "${OSXCROSS_TARGET_DIR}/bin/x86_64-apple-darwin${darwin_ver}-clang")
       set(OSXCROSS_HOST x86_64-apple-darwin${darwin_ver})
    endif()
  endforeach()

  message(STATUS "Using MacOS SDK ${CMAKE_OSX_DEPLOYMENT_TARGET}, and compiler host ${OSXCROSS_HOST}")
  # specify the cross compiler
  set(CMAKE_C_COMPILER "${OSXCROSS_TARGET_DIR}/bin/o64-clang")
  set(CMAKE_CXX_COMPILER "${OSXCROSS_TARGET_DIR}/bin/o64-clang++")

  # where is the target environment set(CMAKE_FIND_ROOT_PATH "${OSXCROSS_SDK}"
  # "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local")

  # search for programs in the build host directories
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  # for libraries and headers in the target directories
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

  set(CMAKE_AR
      "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-ar"
      CACHE FILEPATH "ar")
  set(CMAKE_RANLIB
      "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-ranlib"
      CACHE FILEPATH "ranlib")
  set(CMAKE_INSTALL_NAME_TOOL
      "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-install_name_tool"
      CACHE FILEPATH "install_name_tool")
  set(CMAKE_STRIP_CMD "${OSXCROSS_TARGET_DIR}/bin/${OSXCROSS_HOST}-strip")

  set(ENV{PKG_CONFIG_LIBDIR} "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local/lib/pkgconfig")
  set(ENV{PKG_CONFIG_SYSROOT_DIR} "${OSXCROSS_TARGET_DIR}/macports/pkgs")
  add_definitions(-DMACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
  set(CMAKE_INSTALL_DO_STRIP TRUE)
else()
  toolchain_generate("darwin-x86_64")
  # Configure how we strip executables.
  set(CMAKE_STRIP_CMD "${CMAKE_STRIP} -S")

endif()

# Always consider the source to be darwin.
add_definitions(-D_DARWIN_C_SOURCE=1)


# And the asm type if we are compiling with yasm
set(ANDROID_YASM_TYPE macho64)
# No magical includes or dependencies for darwin..
