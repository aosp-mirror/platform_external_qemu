# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE/2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# The toolchain files get processed multiple times during compile detection keep
# these files simple, and do not setup libraries or anything else. merely tags,
# compiler toolchain and flags should be set here.

get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
# cmake will look for WinMSVCCrossCompile in Platform/WinMSVCCrossCompile.cmake
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}/Modules")
get_filename_component(AOSP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.."
                       ABSOLUTE)
set(ANDROID_QEMU2_TOP_DIR "${AOSP_ROOT}/external/qemu")
include(toolchain)
include(toolchain-rust)

# First we setup all the tags.
toolchain_configure_tags("windows_msvc-x86_64")

get_filename_component(ANDROID_QEMU2_TOP_DIR
                       "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)

# Cmake goes crazy if we set AR manually.. so let's not do that.
if(WIN32)
  # set(CMAKE_SYSTEM_NAME WinMSVCCrossCompile)
  get_clang_version(CLANG_VER)
  message(
    STATUS "Configuring native windows build using clang-cl: ${CLANG_VER}")
  get_filename_component(
    CLANG_DIR
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/clang/host/windows-x86/${CLANG_VER}"
    REALPATH)

  # clang-cl acts as a cl.exe drop replacement.
  set(CLANG_CL ${CLANG_DIR}/bin/clang-cl.exe)
  internal_get_env_cache(CC_EXE)
  if(OPTION_CCACHE)
    # Fix potential path escaping issues..
    string(REPLACE "\\" "/" OPTION_CCACHE "${OPTION_CCACHE}")
    message(STATUS "Enabling ${OPTION_CCACHE} with ${CLANG_CL}")
    # Compile the wrapper..

    if(NOT CC_EXE)
      message(STATUS "Building flattener to handle @rsp files.")
      execute_process(
        COMMAND
          "${CLANG_CL}"
          "${ANDROID_QEMU2_TOP_DIR}/android/build/win/flattenrsp.cpp"
          "/DCLANG_CL=${CLANG_CL}" "/DCCACHE=${OPTION_CCACHE}" "/DUNICODE" "/O2"
          "/std:c++17" "/Fe:${CMAKE_BINARY_DIR}/cc.exe" COMMAND_ECHO STDOUT
        OUTPUT_VARIABLE STD_OUT
        ERROR_VARIABLE STD_ERR
        RESULT_VARIABLE ERR_CODE)
      if(ERR_CODE)
        message(
          WARNING "Disabling sccache, will use ${CLANG_CL} as is instead.")
        internal_set_env_cache(CC_EXE "${CLANG_CL}")
      else()
        internal_set_env_cache(CC_EXE "${CMAKE_BINARY_DIR}/cc.exe")
      endif()
    endif()
  else()
    # NOT OPTION_CCACHE
    if(NOT CC_EXE)
      internal_set_env_cache(CC_EXE "${CLANG_CL}")
    endif()
  endif()

  set(CMAKE_C_COMPILER "${CC_EXE}")
  set(CMAKE_CXX_COMPILER "${CC_EXE}")
  set(ANDROID_LLVM_SYMBOLIZER "${CLANG_DIR}/bin/llvm-symbolizer.exe")
  # Set the debug flags, erasing whatever cmake stuffs in there. We are going to
  # produce "fat" binaries with all debug information in there.
  set(CMAKE_CXX_FLAGS_DEBUG "/MD /Z7")
  set(CMAKE_C_FLAGS_DEBUG "/MD /Z7")
  # Set release flags such that we create pdbs..
  set(CMAKE_C_FLAGS_RELEASE "/MD /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "/MD /Zi")

  # cmd has a limit of 8192 characters, we can easily hit this if we are
  # unlucky. so let's use response files which will help us around that.
  set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
  set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 1)
  set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
  set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)

  set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")

  # Add compiler definition for Win7
  add_definitions("-D_WIN32_WINNT=0x0601")
  # When configuring cmake on windows, it will do a series of compiler checks we
  # want to make sure it never tries to fall back to the msvc linker. CMake will
  # call the linker directly, v.s. through clang when cross building.
  set(CMAKE_LINKER "${CLANG_DIR}/bin/lld-link.exe" CACHE STRING "" FORCE)

  # See
  # https://www.wintellect.com/correctly-creating-native-c-release-build-pdbs/
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
      "/IGNORE:4099 /DEBUG /NODEFAULTLIB:LIBCMT /OPT:REF /OPT:ICF"
      CACHE STRING "" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS "/IGNORE:4099 /DEBUG /NODEFAULTLIB:LIBCMT")

  # The Mt.exe file is a tool that generates signed files and catalogs. This
  # normally gets invoked at the end of executable creation, however anti virus
  # software can easily interfer resulting in build breaks, so lets just not do
  # it.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")

  # Including windows.h will cause issues with std::min/std::max
  add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES
                  -DWIN32_LEAN_AND_MEAN)
else()
  set(CMAKE_SYSTEM_NAME WinMSVCCrossCompile)
  toolchain_generate("${ANDROID_TARGET_TAG}")
  # need to set ar to llvm-ar to unpack windows static libs
  set(CMAKE_AR ${COMPILER_PREFIX}ar)

  # here is the target environment located, used to locate packages. We don't
  # want to do any package resolution with windows/msvc, so we explicitly
  # disable it.
  set(CMAKE_FIND_ROOT_PATH "${ANDROID_SYSROOT}")

  # We must be able to resolve JAVA/JAVAC in order to build our jni + jar for
  # android studio.
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

  # Disable any searching as it might lead to unexpected behavior that varies
  # amongst build environments
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)
  add_definitions(-DWINDOWS_CROSS_COMPILE)
endif()

# And the asm type if we are compiling with ASM
set(ANDROID_ASM_TYPE win64)
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")

# Only configure the rust toolchain if one is present.
if(EXISTS "${AOSP_ROOT}/prebuilts/rust/windows-x86/")
  # Uncomment this once we have a rust toolchain for msvc in AOSP
  get_rust_version(RUST_VER)
  set(RUST_PATH
      "${AOSP_ROOT}\\prebuilts\\gcc\\linux-x86\\host\\x86_64-w64-mingw32-4.8\\x86_64-w64-mingw32\\lib\\\;${AOSP_ROOT}\\prebuilts\\gcc\\linux-x86\\host\\x86_64-w64-mingw32-4.8\\x86_64-w64-mingw32\\bin"
  )

  string(REPLACE "/" "\\" RUST_PATH "${RUST_PATH}")
  configure_rust(COMPILER_ROOT
                 "${AOSP_ROOT}/prebuilts/rust/windows-x86/${RUST_VER}/bin")

  if("${Rust_CARGO}" STREQUAL "NOT_FOUND")
    message(STATUS "No rust toolchain found.")
  else()
    # We have a rust toolchain, make sure we have the proper linker scripts.
    create_windows_linker_script()
    set(Rust_CARGO_TARGET "x86_64-pc-windows-gnu")
    set(Rust_CARGO_LINKER_FLAGS
        "--config target.x86_64-pc-windows-gnu.linker=\\\"${WINDOWS_LINK_SCRIPT}\\\""
    )
  endif()

  # Make sure we add our created libraries to the default link path, and make
  # sure We always explicitly link against gcc_eh.lib to pick up the stack
  # handling from rust.
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /LIBPATH:${RUST_LINK_PATH} ${RUST_LINK_PATH}\\gcc_eh.lib"
  )
  set(CMAKE_MODULE_LINKER_FLAGS
      "${CMAKE_MODULE_LINKER_FLAGS} /LIBPATH:${RUST_LINK_PATH} ${RUST_LINK_PATH}\\gcc_eh.lib"
  )
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} /LIBPATH:${RUST_LINK_PATH} ${RUST_LINK_PATH}\\gcc_eh.lib"
  )
endif()
