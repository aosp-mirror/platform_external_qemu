# Copyright 2018 The Android Open Source Project
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
# the License.

# This contains a set of functions to make working with different targets a lot
# easier. The idea is that you can now set properties that define your
# lib/executable through variables that follow the following naming conventions
#
# ANDROID_TARGET_TAG is in the set [linux-x86_64, darwin-x86_64, windows-x86,
# windows-x86_64, windows_msvc-x86_64] MODIFIER is in the set [public, private, interface]. The
# meaning of a modifier is as follows: (See
# https://cmake.org/cmake/help/v3.5/command/target_link_libraries.html for more
# details): The PUBLIC, PRIVATE and INTERFACE keywords can be used to specify
# both the link dependencies and the link interface in one command. Libraries
# and targets following PUBLIC are linked to, and are made part of the link
# interface. Libraries and targets following PRIVATE are linked to, but are not
# made part of the link interface. Libraries following INTERFACE are appended to
# the link interface and are not used for linking.
#
# The same explanation holds for options, definitions and includes.
#
# ${name}_src The set of sources for target name included in all platforms
# ${name}_${ANDROID_TARGET_TAG}_src The set of sources specific to the given
# target tag. These source will only be included if you are compling for the
# given target. ${name}_libs_${MODIFIER} The set of libraries that need to be
# linked in for the given target for all platforms. ${name}_defs_${MODIFIER} The
# set of compiler definitions (-Dxxxx) that need to be used for this target for
# all platforms. ${name}_includes_${MODIFIER} The set of includes (-Ixxxx) that
# need to be used for this target for all platforms. ${name}_options_${MODIFIER}
# The set of compiler flags that need to be used for this target for all
# platforms.
#
# You can restrict these by including the actual target tag. (See
# test/CMakeLists.txt) for a detailed example.

function(internal_android_target_settings name modifier)
  string(TOLOWER ${modifier} modifier)
  string(TOUPPER ${modifier} MODIFIER)
  if(DEFINED ${name}_libs_${modifier})
    target_link_libraries(${name} ${MODIFIER} ${${name}_libs_${modifier}})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_libs_${modifier})
    target_link_libraries(${name} ${MODIFIER}
                          ${${name}_${ANDROID_TARGET_TAG}_libs_${modifier}})
  endif()

  # Definitions
  if(DEFINED ${name}_defs_${modifier})
    target_compile_definitions(${name} ${MODIFIER} ${${name}_defs_${modifier}})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_defs_${modifier})
    target_compile_definitions(
      ${name} ${MODIFIER} ${${name}_${ANDROID_TARGET_TAG}_defs_${modifier}})
  endif()

  # Includes
  if(DEFINED ${name}_includes_${modifier})
    target_include_directories(${name} ${MODIFIER}
                               ${${name}_includes_${modifier}})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_includes_${modifier})
    target_include_directories(
      ${name} ${MODIFIER} ${${name}_${ANDROID_TARGET_TAG}_includes_${modifier}})
  endif()

  # Compiler options
  if(DEFINED ${name}_compile_options_${modifier})
    target_compile_options(${name} ${MODIFIER}
                           ${${name}_compile_options_${modifier}})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_compile_options_${modifier})
    target_compile_options(
      ${name} ${MODIFIER}
      ${${name}_${ANDROID_TARGET_TAG}_compile_options_${modifier}})
  endif()
endfunction()

function(add_android_library name)
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_library(${name} ${${name}_src})
  internal_android_target_settings(${name} "PUBLIC")
  internal_android_target_settings(${name} "PRIVATE")
endfunction()

function(add_android_shared_library name)
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_library(${name} SHARED ${${name}_src})
  internal_android_target_settings(${name} "PUBLIC")
  internal_android_target_settings(${name} "PRIVATE")
endfunction()

# An interface library merely declares dependencies.
function(add_android_interface_library name)
  add_library(${name} INTERFACE)
  internal_android_target_settings(${name} "INTERFACE")
endfunction()

function(add_android_executable name)
  message(
    STATUS
      "DEFINED ${name}_${ANDROID_TARGET_TAG}_src = ${${name}_${ANDROID_TARGET_TAG}_src}"
    )
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_executable(${name} ${${name}_src})
  internal_android_target_settings(${name} "PUBLIC")
  internal_android_target_settings(${name} "PRIVATE")

  if(DEFINED ${name}_prebuilt_dependencies)
    list(APPEND prebuilt_dependencies ${${name}_prebuilt_dependencies})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_prebuilt_dependencies)
    list(APPEND prebuilt_dependencies
                ${${name}_${ANDROID_TARGET_TAG}_prebuilt_dependencies})
  endif()

  if(DEFINED ${name}_prebuilt_properties)
    list(APPEND prebuilt_properties ${${name}_prebuilt_properties})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_prebuilt_properties)
    list(APPEND prebuilt_properties
                ${${name}_${ANDROID_TARGET_TAG}_prebuilt_properties})
  endif()

  target_prebuilt_dependency(${name} "${prebuilt_dependencies}"
                             "${prebuilt_properties}")
endfunction()

function(add_android_protobuf libname protofiles)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${protofiles})
  set(${libname}_public ${PROTOBUF_INCLUDE_DIR})
  set(${libname}_src ${PROTO_SRCS} ${PROTO_HDRS})
  set(${libname}_includes_public ${PROTOBUF_INCLUDE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR})
  set(${libname}_libs_public ${PROTOBUF_LIBRARIES})
  add_android_library(${libname})
  if(FRANKENBUILD)
    add_library(${CMAKE_STATIC_LIBRARY_PREFIX}${libname}_proto ALIAS ${libname})
  endif()
endfunction()

# hw-config generator
function(generate_hw_config)
  add_custom_command(
    PRE_BUILD
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
    COMMAND
      python ${ANDROID_QEMU2_TOP_DIR}/android/scripts/gen-hw-config.py
      ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
      ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
    VERBATIM)
  set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
    PROPERTIES
    GENERATED
    TRUE)
  set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
    PROPERTIES
    HEADER_FILE_ONLY
    TRUE)
set(ANDROID_HW_CONFIG_H ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PARENT_SCOPE)
endfunction()
