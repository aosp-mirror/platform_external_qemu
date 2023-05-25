# Copyright 2023 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License. This contains a set of functions to make working with different
# targets a lot easier. The idea is that you can now set properties that define
# your lib/executable through variables that follow the following naming
# conventions
#
if(INCLUDE_ANDROID_BAZEL)
  return()
endif()
set(INCLUDE_ANDROID_BAZEL 1)

get_filename_component(AOSP "${CMAKE_CURRENT_LIST_DIR}/../../../../.." ABSOLUTE)
if(LINUX_X86_64)
  set(BAZEL_PLATFORM "linux_x64")
  get_filename_component(BAZEL_BIN "${AOSP}/prebuilts/bazel/linux-x86_64/bazel"
                         ABSOLUTE)
elseif(DARWIN_X86_64)
  # Bazel is a fat binary, so both arm + x86

  set(BAZEL_PLATFORM "macos_x64")
  get_filename_component(BAZEL_BIN
                         "${AOSP}/prebuilts/bazel/darwin-x86_64/bazel" ABSOLUTE)
elseif(DARWIN_AARCH64)
  # Bazel is a fat binary, so both arm + x86
  set(BAZEL_PLATFORM "macos_aarch64")
  get_filename_component(BAZEL_BIN
                         "${AOSP}/prebuilts/bazel/darwin-x86_64/bazel" ABSOLUTE)
  # Force bazel to not run under rosetta so we can rely on auto platform detection
  set(BAZEL_BIN /usr/bin/arch -arm64 ${BAZEL_BIN})
endif()

set(BAZEL_OUTPUT_BASE ${AOSP})
# set(BAZEL_BIN ${BAZEL_BIN} --output_base=${BAZEL_OUTPUT_BASE}) # use build dir
# as output base ?
set(BAZEL_OUT "${BAZEL_OUTPUT_BASE}/bazel-out")

# ~~~
# Registers the given as a bazel target, meaning it will be compiled using the
# bazel toolchain.
#
# ``TARGET``  The library/executable target. For example emulatory-libyuv
# ``BAZEL``   The bazel target name for example "@zlib//:z"
# ``INCLUDES`` The public includes that should be added
# ~~~
function(android_add_bazel_lib)
  if(WINDOWS_X86_64 OR LINUX_AARCH64)
    message(
      FATAL_ERROR
        "We only support bazel compilation under linux x64 and darwin.")
  endif()
  set(options)
  set(oneValueArgs TARGET BAZEL)
  set(multiValueArgs INCLUDES)
  cmake_parse_arguments(bazel "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  message(STATUS "Building ${build_TARGET} with bazel ${build_BAZEL}")
  # First we extract the path of the static archive that the bazel target
  # creates
  execute_process(
    COMMAND ${BAZEL_BIN} cquery --output=files --output_groups=archive
            "${bazel_BAZEL}"
    WORKING_DIRECTORY ${AOSP}
    OUTPUT_VARIABLE BAZEL_TGT
    ERROR_VARIABLE BAZEL_DETAILS COMMAND_ECHO STDOUT ECHO_OUTPUT_VARIABLE
                   ECHO_ERROR_VARIABLE)
  string(REPLACE "\n" "" BAZEL_TGT "${BAZEL_TGT}")

  # Next we create a custom command to generate this static archive.
  add_custom_command(
    OUTPUT "${AOSP}/${BAZEL_TGT}"
    # Nasty workaround for the fact that ninja seems to be creating the
    # bazel-out directory see b/274180576 for details.
    COMMAND
      sh -c
      "if [ ! -L ${BAZEL_OUT} ] && [ -d ${BAZEL_OUT} ] ; then rm -rf ${BAZEL_OUT} ; fi"
    COMMAND ${BAZEL_BIN} build "${bazel_BAZEL}"
    COMMENT "Compiling ${bazel_TARGET} with bazel"
    WORKING_DIRECTORY ${AOSP}
    VERBATIM)

  # After which we add a custom target that is used to generate this archive We
  # will create an imported target with a dependency on the bazel_... target
  # that will do the actual compilation.
  add_custom_target(bazel_${bazel_TARGET} DEPENDS "${AOSP}/${BAZEL_TGT}")
  add_library(${bazel_TARGET} STATIC IMPORTED GLOBAL)
  set_target_properties(
    ${bazel_TARGET}
    PROPERTIES IMPORTED_LOCATION "${AOSP}/${BAZEL_TGT}"
               INTERFACE_INCLUDE_DIRECTORIES "${bazel_INCLUDES}")
  add_dependencies(${bazel_TARGET} bazel_${bazel_TARGET})
endfunction()
