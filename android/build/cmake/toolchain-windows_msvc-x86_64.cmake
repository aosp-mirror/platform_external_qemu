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
get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
# cmake will look for WinMSVCCrossCompile in Platform/WinMSVCCrossCompile.cmake
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}/Modules")

include(toolchain)

# First we create the toolchain
get_host_tag(ANDROID_HOST_TAG)
set(ANDROID_TARGET_TAG "windows_msvc-x86_64")
set(ANDROID_TARGET_OS "windows_msvc")
set(ANDROID_TARGET_OS_FLAVOR "windows")
set(CMAKE_SYSTEM_PROCESSOR AMD64)

get_filename_component(ANDROID_QEMU2_TOP_DIR "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)

# Cmake goes crazy if we set AR manually.. so let's not do that.
if(WIN32)
  message(STATUS "Native windows")
  set(CLANG_VER "clang-r346389b")
  get_filename_component(CLANG_DIR "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/clang/host/windows-x86/${CLANG_VER}"
                         REALPATH)
  if(NOT EXISTS "${CLANG_DIR}/bin/clang-cl.exe")
    execute_process(COMMAND copy "${CLANG_DIR}/bin/clang.exe" "${CLANG_DIR}/bin/clang-cl.exe"
                    WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                    RESULT_VARIABLE CP_RES
                    OUTPUT_VARIABLE STD_OUT
                    ERROR_VARIABLE STD_ERR)
    message(
      STATUS
        "'copy ${CLANG_VER} ${CLANG_DIR}/bin/clang.exe ${CLANG_DIR}/bin/clang-cl.exe', out: ${STD_OUT}, err: ${STD_ERR}"
      )
    if(NOT "${CP_RES}" STREQUAL "0")
      message(
        WARNING
          "Unable to 'copy ${CLANG_DIR}/bin/clang.exe ${CLANG_DIR}/bin/clang-cl.exe', out: ${STD_OUT}, err: ${STD_ERR}")
    endif()
  endif()
  set(CMAKE_C_COMPILER "${CLANG_DIR}/bin/clang-cl.exe")
  set(CMAKE_CXX_COMPILER "${CLANG_DIR}/bin/clang-cl.exe")

  # Set the debug and release flags
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -MD")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -MD")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -MD")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -MD")


  # Make sure nobody is accidently trying to do a 32 bit build.
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    # message(FATAL_ERROR "Please configure the compiler for a x64 build!")
  endif()

  # The Mt.exe file is a tool that generates signed files and catalogs.
  # This normally gets invoked at the end of executable creation, however
  # anti virus software can easily interfer resulting in build breaks,
  # so lets just not do it.
  set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")

  # Including windows.h will cause issues with std::min/std::max
  add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES -DWIN32_LEAN_AND_MEAN)
else()
  set(CMAKE_SYSTEM_NAME WinMSVCCrossCompile)
  toolchain_generate("${ANDROID_TARGET_TAG}")
  # need to set ar to llvm-ar to unpack windows static libs
  set(CMAKE_AR ${COMPILER_PREFIX}ar)

  # here is the target environment located, used to locate packages. We don't want to do any package resolution with
  # windows-msvc, so we explicitly disable it.
  set(CMAKE_FIND_ROOT_PATH "${ANDROID_SYSROOT}")

  # Disable any searching as it might lead to unexpected behavior that varies amongst build environments
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
  add_definitions(-DWINDOWS_CROSS_COMPILE)
endif()

include(emu-windows-libs)
