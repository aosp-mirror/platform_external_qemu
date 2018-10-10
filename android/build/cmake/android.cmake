# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

# This contains a set of functions to make working with different targets a lot easier. The idea is that you can now set
# properties that define your lib/executable through variables that follow the following naming conventions
#
# ANDROID_TARGET_TAG is in the set [linux-x86_64, darwin-x86_64, windows-x86, windows-x86_64] MODIFIER is in the set
# [public, private, interface]. The meaning of a modifier is as follows: (See
# https://cmake.org/cmake/help/v3.5/command/target_link_libraries.html for more details): The PUBLIC, PRIVATE and
# INTERFACE keywords can be used to specify both the link dependencies and the link interface in one command. Libraries
# and targets following PUBLIC are linked to, and are made part of the link interface. Libraries and targets following
# PRIVATE are linked to, but are not made part of the link interface. Libraries following INTERFACE are appended to the
# link interface and are not used for linking.
#
# The same explanation holds for options, definitions and includes.
#
# ${name}_src The set of sources for target name included in all platforms ${name}_${ANDROID_TARGET_TAG}_src The set of
# sources specific to the given target tag. These source will only be included if you are compling for the given target.
# ${name}_libs_${MODIFIER} The set of libraries that need to be linked in for the given target for all platforms.
# ${name}_defs_${MODIFIER} The set of compiler definitions (-Dxxxx) that need to be used for this target for all
# platforms. ${name}_includes_${MODIFIER} The set of includes (-Ixxxx) that need to be used for this target for all
# platforms. ${name}_options_${MODIFIER} The set of compiler flags that need to be used for this target for all
# platforms.
#
# You can restrict these by including the actual target tag. (See test/CMakeLists.txt) for a detailed example.

function(android_target_compile_definitions TGT OS MODIFIER ITEMS)
  if(${ANDROID_TARGET_TAG} MATCHES "${OS}.*" OR ${OS} STREQUAL "all")
    target_compile_definitions(${TGT} ${MODIFIER} ${ITEMS})
    foreach(DEF ${ARGN})
      target_compile_definitions(${TGT} ${MODIFIER} ${DEF})
    endforeach()
  endif()
endfunction()

function(android_target_link_libraries TGT OS MODIFIER ITEMS)
  if(${ARGC} GREATER "49")
    message(
      FATAL_ERROR "Currently cannot link more than 49 dependecies due to some weirdness with calling target_link_libs")
  endif()
  if(${ANDROID_TARGET_TAG} MATCHES "${OS}.*" OR ${OS} STREQUAL "all")
    # HACK ATTACK! We cannot properly expand unknown linker args as they are treated in a magical fashion. Some
    # arguments need to be "grouped" together somehow (for example optimized;lib) since we cannot resolve this properly
    # we just pass on the individual arguments..
    target_link_libraries(${TGT}
                          ${MODIFIER}
                          ${ARGV3}
                          ${ARGV4}
                          ${ARGV5}
                          ${ARGV6}
                          ${ARGV7}
                          ${ARGV8}
                          ${ARGV9}
                          ${ARGV10}
                          ${ARGV11}
                          ${ARGV12}
                          ${ARGV13}
                          ${ARGV14}
                          ${ARGV15}
                          ${ARGV16}
                          ${ARGV17}
                          ${ARGV18}
                          ${ARGV19}
                          ${ARGV20}
                          ${ARGV21}
                          ${ARGV22}
                          ${ARGV23}
                          ${ARGV24}
                          ${ARGV25}
                          ${ARGV26}
                          ${ARGV27}
                          ${ARGV28}
                          ${ARGV29}
                          ${ARGV30}
                          ${ARGV31}
                          ${ARGV32}
                          ${ARGV33}
                          ${ARGV34}
                          ${ARGV35}
                          ${ARGV36}
                          ${ARGV37}
                          ${ARGV38}
                          ${ARGV39}
                          ${ARGV40}
                          ${ARGV41}
                          ${ARGV42}
                          ${ARGV43}
                          ${ARGV44}
                          ${ARGV45}
                          ${ARGV46}
                          ${ARGV47}
                          ${ARGV48}
                          ${ARGV49})
  endif()
endfunction()

function(android_target_include_directories TGT OS MODIFIER ITEMS)
  if(${ANDROID_TARGET_TAG} MATCHES "${OS}.*" OR ${OS} STREQUAL "all")
    target_include_directories(${TGT} ${MODIFIER} ${ITEMS})
    foreach(DIR ${ARGN})
      target_include_directories(${TGT} ${MODIFIER} ${DIR})
    endforeach()
  endif()
endfunction()

function(android_target_compile_options TGT OS MODIFIER ITEMS)
  if(${ANDROID_TARGET_TAG} MATCHES "${OS}.*" OR ${OS} STREQUAL "all")
    target_compile_options(${TGT} ${MODIFIER} "${ITEMS};${ARGN}")
  endif()
endfunction()

function(android_add_library name)
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  if(${ANDROID_TARGET_TAG} MATCHES "windows.*" AND DEFINED ${name}_windows_src)
    list(APPEND ${name}_src ${${name}_windows_src})
  endif()
  add_library(${name} ${${name}_src})
endfunction()

function(android_add_shared_library name)
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_library(${name} SHARED ${${name}_src})
endfunction()

# An interface library merely declares dependencies.
function(android_add_interface name)
  add_library(${name} INTERFACE)
endfunction()

function(android_add_test name)
  android_add_executable(${name})
  add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> WORKING_DIRECTORY $<TARGET_FILE_DIR:${name}>)
endfunction()

function(android_add_executable name)
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_executable(${name} ${${name}_src})

  android_target_dependency(${name} all "${RUNTIME_OS_DEPENDENCIES}")
  android_target_properties(${name} all "${RUNTIME_OS_PROPERTIES}")
endfunction()

function(android_add_protobuf libname protofiles)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${protofiles})
  set(${libname}_src ${PROTO_SRCS} ${PROTO_HDRS})
  android_add_library(${libname})
  target_include_directories(${libname} PUBLIC ${PROTOBUF_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${libname} PUBLIC ${PROTOBUF_LIBRARIES})
  # Disable generation of information about every class with virtual functions for use by the C++ runtime type
  # identification features (dynamic_cast and typeid). If you don't use those parts of the language, you can save some
  # space by using this flag. Note that exception handling uses the same information, but it will generate it as needed.
  # The  dynamic_cast operator can still be used for casts that do not require runtime type information, i.e. casts to
  # void * or to unambiguous base classes.
  target_compile_options(${libname} PRIVATE -fno-rtti)
  target_compile_definitions(${libname} PUBLIC -DGOOGLE_PROTOBUF_NO_RTTI)
endfunction()

# This function generates the hw config file. It translates android- emu/androd/avd/hardware-properties.ini ->
# android/avd/hw-config-defs.h
#
# This file will be placed on the current binary dir, so it can be included if this directory is on the include path.
function(android_generate_hw_config)
  add_custom_command(POST_BUILD
                     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                     COMMAND python ${ANDROID_QEMU2_TOP_DIR}/android/scripts/gen-hw-config.py
                             ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
                             ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                     VERBATIM)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES GENERATED TRUE)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES HEADER_FILE_ONLY TRUE)
  set(ANDROID_HW_CONFIG_H ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PARENT_SCOPE)
endfunction()

# Copies the list of test data files to the given destination The test data resides in the prebuilts/android-emulator-
# build/common/testdata folder.
#
# TGT The target as part of which the test files should be copied. SOURCE_LIST The list of files that need to be copied
# DEST The subdirectory under TGT where the files should be copied to.
function(android_copy_test_files TGT SOURCE_LIST DEST)
  get_filename_component(TESTDATA_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/"
                         ABSOLUTE)

  foreach(SRC_FILE ${SOURCE_LIST})
    get_filename_component(DEST_FILE ${SRC_FILE} NAME)
    add_custom_command(TARGET ${TGT} PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TESTDATA_ROOT}/${SRC_FILE}
                               $<TARGET_FILE_DIR:${TGT}>/${DEST}/${DEST_FILE})
  endforeach()
endfunction()

function(android_copy_file TGT SRC DST)
  # get_filename_component(DST_DIR ${DST} DIRECTORY)
  add_custom_command(TARGET ${TGT} PRE_BUILD
                     # COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DST}
                     MAIN_DEPENDENCY ${SRC})
endfunction()

# Copies the given test directory to the given destination. The test data resides in the prebuilts/android-emulator-
# build/common/testdata folder.
#
# TGT The target as part of which the test files should be copied. SRC_DIR The source directories to copy., DST_DIR The
# subdirectory under TGT where the files should be copied to.
function(android_copy_test_dir TGT SRC_DIR DST_DIR)
  get_filename_component(TESTDATA_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/testdata"
                         ABSOLUTE)

  add_custom_command(TARGET ${TGT} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy_directory ${TESTDATA_ROOT}/${SRC_DIR}
                             $<TARGET_FILE_DIR:${TGT}>/${DST_DIR})
endfunction()

# Makes the target dependent on e2fsprogs. This will guarantee that the e2fsprogs will be available in the expected
# binary directory
function(android_add_e2fsprogs_dependency TGT)
  set(TARGET_TAG ${ANDROID_TARGET_TAG})
  if(${ANDROID_TARGET_TAG} MATCHES "windows.*")
    set(TARGET_TAG "windows-x86")
  endif()

  get_filename_component(
    PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/e2fsprogs/${TARGET_TAG}"
    ABSOLUTE)

  # binplace in bin64/bin
  set(DST_DIR "bin")
  if(${ANDROID_TARGET_TAG} MATCHES ".*x86_64")
    set(DST_DIR "bin64")
  endif()

  set(EXT4_FILES e2fsck fsck.ext4 mkfs.ext4 resize2fs tune2fs)
  foreach(EXT_FILE ${EXT4_FILES})
    android_copy_file(${TGT} ${PREBUILT_ROOT}/sbin/${EXT_FILE} $<TARGET_FILE_DIR:${TGT}>/${DST_DIR}/${EXT_FILE})
  endforeach()

endfunction()

function(add_c_flag FLGS)
  foreach(FLAG ${FLGS})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
  endforeach()
endfunction()

function(add_cxx_flag FLGS)
  foreach(FLAG ${FLGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
  endforeach()
endfunction()

# This function retrieves th git_version, or reverting to the date if we cannot fetch it.
#
# VER The variable to set the version in Sets ${VER} The version as reported by git, or the date
function(get_git_version VER)
  execute_process(COMMAND "git" "describe" "--match" "v*"
                  WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                  RESULT_VARIABLE GIT_RES
                  OUTPUT_VARIABLE STD_OUT
                  ERROR_VARIABLE STD_ERR)
  if(NOT "${GIT_RES}" STREQUAL "0")
    message(WARNING "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR}, out: ${STD_OUT}, err: ${STD_ERR}")
    execute_process(COMMAND "date" "+%Y-%m-%d"
                    WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                    RESULT_VARIABLE DATE_RES
                    OUTPUT_VARIABLE STD_OUT
                    ERROR_VARIABLE STD_ERR)
    if(NOT "${DATE_RES}" STREQUAL "0")
      message(FATAL_ERROR "Unable to retrieve date!")
    endif()
  endif()

  # Clean up and make visibile
  string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
  set(${VER} ${STD_OUT} PARENT_SCOPE)
endfunction()

# VER The variable to set the sha in Sets ${VER} The latest sha as reported by git
function(get_git_sha VER)
  execute_process(COMMAND "git" "log" "-n" "1" "--pretty=format:'%H'"
                  WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                  RESULT_VARIABLE GIT_RES
                  OUTPUT_VARIABLE STD_OUT
                  ERROR_VARIABLE STD_ERR)
  if(NOT "${GIT_RES}" STREQUAL "0")
    message(
      FATAL_ERROR "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR} : out: ${STD_OUT}, err: ${STD_ERR}")
  endif()

  # Clean up and make visibile
  string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
  set(${VER} ${STD_OUT} PARENT_SCOPE)
endfunction()

# Constructs a linker command that will make sure the whole archive is included, not just the ones referenced.
#
# LIBCMD The variable which will contain the complete linker command LIBNAME The archive that needs to be included
# completely
function(android_complete_archive LIBCMD LIBNAME)
  if(ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
    set(${LIBCMD} "-Wl,-force_load,$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
  else()
    set(${LIBCMD} "-Wl,-whole-archive" ${LIBNAME} "-Wl,-no-whole-archive" PARENT_SCOPE)
  endif()
endfunction()

function(android_add_qemu_executable AARCH LOCAL_AARCH STUBS CPU)
  android_complete_archive(QEMU_COMPLETE_LIB "libqemu2-common")
  add_executable(qemu-system-${AARCH}
                 android-qemu2-glue/main.cpp
                 vl.c
                 ${STUBS}
                 ${qemu-system-${AARCH}_sources}
                 ${qemu-system-${AARCH}_generated_sources})
  target_include_directories(qemu-system-${AARCH} PRIVATE android-qemu2-glue/config/target-${LOCAL_AARCH} target/${CPU})
  target_compile_definitions(qemu-system-${AARCH} PRIVATE -DNEED_CPU_H -DCONFIG_ANDROID)
  target_link_libraries(qemu-system-${AARCH}
                        PRIVATE android-qemu-deps
                                -w
                                ${QEMU_COMPLETE_LIB}
                                libqemu2-glue
                                libqemu2-util
                                emulator-libui
                                android-emu
                                OpenGLESDispatch
                                ${VIRGLRENDERER_LIBRARIES}
                                android-qemu-deps)
  # Make the common dependency explicit, as some generators might not detect it properly (Xcode)
  add_dependencies(qemu-system-${AARCH} libqemu2-common)
  # TODO XCode bin places this not where we want this..
  set_target_properties(qemu-system-${AARCH}
                        PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qemu/${ANDROID_TARGET_TAG}")
endfunction()

# Create a file that provide the linker with information about exports, attributes, and other information about the
# program to be linked
function(android_generate_symbol_file TGT ENTRIES)
  if(${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
    set(MODE sym)
  elseif(${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
    set(MODE _symbols)
  elseif(${ANDROID_TARGET_TAG} MATCHES "windows-x86.*")
    set(MODE def)
  endif()
  string(REPLACE ".entries" "" DEST ${ENTRIES})
  set(DEST ${CMAKE_CURRENT_BINARY_DIR}/${DEST})
  add_custom_command(PRE_BUILD
                     OUTPUT ${DEST}
                     COMMAND python ${ANDROID_QEMU2_TOP_DIR}/android/scripts/gen-entries.py
                             --mode ${MODE}
                             --output=${DEST} ${ENTRIES}
                     VERBATIM)
  set_source_files_properties(${DEST} PROPERTIES GENERATED TRUE)
  set(SYMBOLS_FILE ${DEST} PARENT_SCOPE)
endfunction()

# Exports the sybmols of a shared library. The symbols to be exported should be defined in the file entries.
function(android_export_symbols TGT ENTRIES)
  android_generate_symbol_file(${TGT} ${ENTRIES})
  if(${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
    target_link_libraries(${TGT} "-WL,--version-script=${SYMBOLS_FILE}")
  elseif(${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
    target_link_libraries(${TGT} "-WL,-exported_symbols_list,${SYMBOLS_FILE}")
  elseif(${ANDROID_TARGET_TAG} MATCHES "windows-x86.*")
    target_link_libraries(${TGT} "-WL,-def:${SYMBOLS_FILE}")
  endif()
endfunction()

function(android_copy_shared_lib TGT DEP NAME)
  android_copy_file(${TGT} $<TARGET_FILE:${DEP}>
                    $<TARGET_FILE_DIR:${TGT}>/lib${ANDROID_TARGET_BITS}/${NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
endfunction()

function(android_log msg)
    if (ANDROID_LOG)
        message(STATUS ${MSG})
    endif()
endfunction()
