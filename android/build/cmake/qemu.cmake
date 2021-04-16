
# Copyright 2021 The Android Open Source Project
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

# This contains all emulator specific code..

if(INCLUDE_QEMU_CMAKE)
  return()
endif()

set(INCLUDE_QEMU_CMAKE 1)

if (NOT ANDROID_QEMU2_TOP_DIR)
    set(ANDROID_QEMU2_TOP_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
endif()

# This function retrieves th git_version, or reverting to the date if we cannot
# fetch it.
#
# VER The variable to set the version in Sets ${VER} The version as reported by
# git, or the date
function(get_git_version VER)
  execute_process(
    COMMAND "git" "describe" WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
    RESULT_VARIABLE GIT_RES OUTPUT_VARIABLE STD_OUT ERROR_VARIABLE STD_ERR)
  if(NOT "${GIT_RES}" STREQUAL "0")
    message(
      WARNING
        "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR}, out: ${STD_OUT}, err: ${STD_ERR}, not setting version."
    )
  else()
    # Clean up and make visibile
    string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
    set(${VER} ${STD_OUT} PARENT_SCOPE)
  endif()

endfunction()


# This function generates the hw config file. It translates android-
# emu/androd/avd/hardware-properties.ini -> android/avd/hw-config-defs.h
#
# This file will be placed on the current binary dir, so it can be included if
# this directory is on the include path.
function(android_generate_hw_config)
  set(ANDROID_HW_CONFIG_H
      ${CMAKE_CURRENT_BINARY_DIR}/avd_config/android/avd/hw-config-defs.h)
  add_custom_command(
    OUTPUT ${ANDROID_HW_CONFIG_H}
    COMMAND
      python ${ANDROID_QEMU2_TOP_DIR}/android/scripts/gen-hw-config.py
      ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
      ${ANDROID_HW_CONFIG_H}
    DEPENDS
      ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
    VERBATIM)
  android_add_library(TARGET android-hw-config LICENSE Apache-2.0
                      SRC ${ANDROID_HW_CONFIG_H} dummy.c)
  set_source_files_properties(
    ${ANDROID_HW_CONFIG_H} PROPERTIES GENERATED TRUE SKIP_AUTOGEN ON
                                      HEADER_FILE_ONLY TRUE)
  target_include_directories(android-hw-config
                             PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/avd_config)
endfunction()

# Constructs a linker command that will make sure the whole archive is included,
# not just the ones referenced.
#
# . LIBCMD The variable which will contain the complete linker command . LIBNAME
# The archive that needs to be included completely
function(android_complete_archive LIBCMD LIBNAME)
  if(DARWIN_X86_64 OR DARWIN_AARCH64)
    set(${LIBCMD} "-Wl,-force_load,$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
  elseif(WINDOWS_MSVC_X86_64)
    if(MSVC)
      # The native build calls the linker directly
      set(${LIBCMD} "-wholearchive:$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
    else()
      # The cross compiler calls the linker through clang.
      set(${LIBCMD} "-Wl,-wholearchive:$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
    endif()
  else()
    set(${LIBCMD} "-Wl,--whole-archive" ${LIBNAME} "-Wl,--no-whole-archive"
        PARENT_SCOPE)
  endif()
endfunction()

# Constructs the upstream qemu executable.
#
# ANDROID_AARCH The android architecture name QEMU_AARCH The qemu architecture
# name. CONFIG_AARCH The configuration architecture used. STUBS The set of stub
# sources to use. CPU The target cpu architecture used by qemu
#
# (Maybe on one day we will standardize all the naming, between qemu and configs
# and cpus..)
function(android_build_qemu_variant)
  # Parse arguments
  set(options INSTALL)
  set(oneValueArgs CPU EXE)
  set(multiValueArgs SOURCES DEFINITIONS LIBRARIES)
  cmake_parse_arguments(qemu_build "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  # translate various names.. because of inconsistent naming in the code base..
  if(qemu_build_CPU STREQUAL "x86_64")
    set(CPU "i386")
    set(QEMU_AARCH "x86_64")
    set(CONFIG_AARCH "x86_64")
  elseif(qemu_build_CPU STREQUAL "i386")
    set(CPU "i386")
    set(QEMU_AARCH "i386")
    set(CONFIG_AARCH "x86")
  elseif(qemu_build_CPU STREQUAL "aarch64")
    set(CPU "arm")
    set(QEMU_AARCH "aarch64")
    set(CONFIG_AARCH "arm64")
  elseif(qemu_build_CPU STREQUAL "armel")
    set(CPU "arm")
    set(QEMU_AARCH "arm")
    set(CONFIG_AARCH "arm")
  else()
    message(FATAL_ERROR "Unknown cpu type.")
  endif()

  # Workaround b/121393952, older cmake does not have proper object archives
  android_complete_archive(QEMU_COMPLETE_LIB "qemu2-common")
  android_add_executable(
    TARGET ${qemu_build_EXE}
    LICENSE Apache-2.0
    LIBNAME
      qemu-system
      URL
      "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev"
    REPO "${ANDROID_QEMU2_TOP_DIR}"
    NOTICE "REPO/LICENSES/LICENSE.APACHE2"
    SRC ${qemu-system-${QEMU_AARCH}_generated_sources}
        ${qemu-system-${QEMU_AARCH}_sources} ${qemu_build_SOURCES})
  target_include_directories(
    ${qemu_build_EXE} PRIVATE android-qemu2-glue/config/target-${CONFIG_AARCH}
                              target/${CPU})
  target_compile_definitions(${qemu_build_EXE}
                             PRIVATE ${qemu_build_DEFINITIONS})
  target_link_libraries(${qemu_build_EXE} PRIVATE ${QEMU_COMPLETE_LIB}
                                                  ${qemu_build_LIBRARIES})

  # Make the common dependency explicit, as some generators might not detect it
  # properly (Xcode/MSVC native)
  add_dependencies(${qemu_build_EXE} qemu2-common)
  # XCode bin places this not where we want this...
  if(qemu_build_INSTALL)
    set_target_properties(
      ${qemu_build_EXE}
      PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qemu/${ANDROID_TARGET_OS_FLAVOR}-${ANDROID_TARGET_ARCH}"
    )
    android_install_exe(
      ${qemu_build_EXE}
      "./qemu/${ANDROID_TARGET_OS_FLAVOR}-${ANDROID_TARGET_ARCH}")
  endif()
endfunction()

# Constructs the qemu executable.
#
# ANDROID_AARCH The android architecture name STUBS The set of stub sources to
# use.
function(android_add_qemu_executable ANDROID_AARCH STUBS)
  if(WINDOWS_MSVC_X86_64)
    set(WINDOWS_LAUNCHER emulator-winqt-launcher)
  endif()
  android_build_qemu_variant(
    INSTALL
    EXE qemu-system-${ANDROID_AARCH}
    CPU ${ANDROID_AARCH}
    SOURCES ${STUBS} android-qemu2-glue/main.cpp vl.c
    DEFINITIONS
      -DNEED_CPU_H -DCONFIG_ANDROID
      -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
      -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}
    LIBRARIES libqemu2-glue
              libqemu2-glue-vm-operations
              libqemu2-util
              emulator-libui
              android-emu
              android-qemu-deps
              android-qemu-deps-headful
              emulator-libusb
              ${WINDOWS_LAUNCHER})
endfunction()

# Constructs the qemu headless executable.
#
# ANDROID_AARCH The android architecture name STUBS The set of stub sources to
# use.
function(android_add_qemu_headless_executable ANDROID_AARCH STUBS)
  android_build_qemu_variant(
    INSTALL
    EXE qemu-system-${ANDROID_AARCH}-headless
    CPU ${ANDROID_AARCH}
    SOURCES ${STUBS} android-qemu2-glue/main.cpp vl.c
    DEFINITIONS
      -DNEED_CPU_H -DCONFIG_ANDROID -DCONFIG_HEADLESS
      -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
      -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}
    LIBRARIES libqemu2-glue
              libqemu2-glue-vm-operations
              libqemu2-util
              android-emu
              emulator-libui-headless
              android-qemu-deps
              android-qemu-deps-headless
              emulator-libusb)
endfunction()

# Constructs the qemu upstream executable.
#
# ANDROID_AARCH The android architecture name STUBS The set of stub sources to
# use.
function(android_add_qemu_upstream_executable ANDROID_AARCH STUBS)
  if(DARWIN_AARCH64)
    android_build_qemu_variant(
      # INSTALL We do not install this target.
      EXE qemu-upstream-${ANDROID_AARCH}
      CPU ${ANDROID_AARCH}
      SOURCES ${STUBS} vl.c
      DEFINITIONS -DNEED_CPU_H
      LIBRARIES android-emu
                libqemu2-glue
                libqemu2-glue-vm-operations
                libqemu2-util
                android-qemu-deps
                android-qemu-deps-headful
                emulator-libusb)
  else()
    android_build_qemu_variant(
      # INSTALL We do not install this target.
      EXE qemu-upstream-${ANDROID_AARCH}
      CPU ${ANDROID_AARCH}
      SOURCES ${STUBS} vl.c
      DEFINITIONS -DNEED_CPU_H
      LIBRARIES android-emu
                libqemu2-glue
                libqemu2-glue-vm-operations
                libqemu2-util
                SDL2::SDL2
                android-qemu-deps
                android-qemu-deps-headful
                emulator-libusb)
  endif()
endfunction()

# ~~~
# Finalize all licenses:
#
# 1. Writes out all license related information to a file.
# 2. Invokes the python license validator.
# ~~~
function(finalize_all_licenses)
  android_discover_targets(ALL_TARGETS ${ANDROID_QEMU2_TOP_DIR})
  get_property(LICENSES GLOBAL PROPERTY LICENSES_LST)
  get_property(TARGETS GLOBAL PROPERTY INSTALL_TARGETS_LST)
  file(REMOVE ${CMAKE_BINARY_DIR}/TARGET_DEPS.LST)
  file(WRITE ${CMAKE_BINARY_DIR}/LICENSES.LST "${LICENSES}")
  file(WRITE ${CMAKE_BINARY_DIR}/INSTALL_TARGETS.LST "${TARGETS}")
  foreach(tgt ${ALL_TARGETS})
    get_target_property(target_type "${tgt}" TYPE)
    set(DEPS "")
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
      get_target_property(DEPS ${tgt} LINK_LIBRARIES)
    endif()
    file(APPEND ${CMAKE_BINARY_DIR}/TARGET_DEPS.LST
         ";${tgt}|${target_type}|${DEPS}\n")
  endforeach()

  execute_process(
    COMMAND "python" "android/build/python/aemu/licensing.py"
            "${PROJECT_BINARY_DIR}" WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
    RESULT_VARIABLE LICENSE_RES OUTPUT_VARIABLE STD_OUT ERROR_VARIABLE STD_ERR)
  if(NOT "${LICENSE_RES}" STREQUAL "0")
    message(
      FATAL_ERROR
        "Unable to validate licenses, out: ${STD_OUT}, err: ${STD_ERR}")
  endif()
endfunction()

# Creates the dependency
function(android_install_license tgt targetname)
  set_property(GLOBAL APPEND PROPERTY INSTALL_TARGETS_LST
                                      "${tgt}|${targetname}\n")
endfunction()


# ~~~
# ! android_license: Registers the given license, ands adds it to LICENSES_LST for post processing !
#
# ``LIBNAME`` The name of the library, this is how it is usually known in the world.
#             for example: libpng
# ``URL``     The location where the source code for this library can be found.
# ``TARGET``  List of targets that are part of the named library. For example crypto;ssl
# ``SPDX``    The spdx license identifier. (See https://spdx.org/)
# ``LOCAL``   Path to the NOTICE/LICENSE file.
# ``LICENSE`` Url to the actual license file.
# ~~
function(android_license)
  # Parse arguments
  set(options)
  set(oneValueArgs LIBNAME URL SPDX LICENSE LOCAL)
  set(multiValueArgs TARGET)
  cmake_parse_arguments(args "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})
  string(REPLACE "//" "/" URL "${args_URL}")
  string(REPLACE ":/" "://" URL "${URL}")
  string(REPLACE "//" "/" LICENSE "${args_LICENSE}")
  string(REPLACE ":/" "://" LICENSE "${LICENSE}")
  set_property(
    GLOBAL APPEND
    PROPERTY
      LICENSES_LST
      "${args_TARGET}|${args_LIBNAME}|${URL}|${args_SPDX}|${LICENSE}|${args_LOCAL}\n"
  )
endfunction()

# Uploads the symbols to the breakpad crash server
function(android_upload_symbols TGT)
  find_package(PythonInterp)
  if(NOT ANDROID_EXTRACT_SYMBOLS)
    return()
  endif()
  set(DEST "${ANDROID_SYMBOL_DIR}/${TGT}.sym")
  set(LOG "${ANDROID_SYMBOL_DIR}/${TGT}.log")
  install(
    CODE # Upload the symbols, with warnings/error logging only.
         "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\"
                             \"${ANDROID_QEMU2_TOP_DIR}/android/build/python/aemu/upload_symbols.py\"
                             \"--symbol_file\" \"${DEST}\"
                             \"--environment\" \"${OPTION_CRASHUPLOAD}\"
                             OUTPUT_FILE ${LOG}
                             ERROR_FILE ${LOG})\n
    if (EXISTS ${LOG})
      FILE(READ ${LOG} contents)
      STRING(STRIP \"\$\{contents\}\" contents)
    else()
        SET(contents \"No logfile in ${LOG} for ${DEST} was created\")
    endif()
    MESSAGE(STATUS \$\{contents\})")
endfunction()


# Extracts symbols from a file that is not built. This is mainly here if we wish
# to extract symbols for a prebuilt file.
function(android_extract_symbols_file FNAME)
  get_filename_component(BASENAME ${FNAME} NAME)
  set(DEST "${ANDROID_SYMBOL_DIR}/${BASENAME}.sym")

  if(WINDOWS_MSVC_X86_64)
    # In msvc we need the pdb to generate the symbols, pdbs are not yet
    # available for The prebuilts. b/122728651
    message(
      WARNING
        "Extracting symbols requires access to the pdb for ${FNAME}, ignoring for now."
    )
    return()
  endif()
  install(
    CODE "execute_process(COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dump_syms -g ${FNAME} OUTPUT_FILE ${DEST} RESULT_VARIABLE RES ERROR_QUIET) \n
                 message(STATUS \"Extracted symbols for ${FNAME} ${RES}\")")
  install(
    CODE "execute_process(COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sym_upload ${DEST} ${ANDROID_SYMBOL_URL}  OUTPUT_VARIABLE RES ERROR_QUIET) \n
                 message(STATUS \"Uploaded symbols for ${FNAME} --> ${ANDROID_SYMBOL_URL} ${RES}\")"
  )
endfunction()

# Extracts the symbols from the given target if extraction is requested. TODO:
# We need generator expressions to move this to the install phase. Which are
# available in cmake 3.13
function(android_extract_symbols TGT)
  if(NOT ANDROID_EXTRACT_SYMBOLS)
    # Note: we do not need to extract symbols on windows for uploading.
    return()
  endif()
  set(DEST "${ANDROID_SYMBOL_DIR}/${TGT}.sym")
  add_custom_command(
    TARGET ${TGT} POST_BUILD COMMAND dump_syms "$<TARGET_FILE:${TGT}>" > ${DEST}
                                     DEPENDS dump_syms
    COMMENT "Extracting symbols for ${TGT}" VERBATIM)
endfunction()

# Make the compatibility layer available for every target
if(WINDOWS_MSVC_X86_64 AND NOT INCLUDE_MSVC_POSIX)
  set(INCLUDE_MSVC_POSIX 1)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/msvc-posix-compat/
                   msvc-posix-compat)
endif()

# Rule to build cf crosvm based on a combination of dependencies built locally /
# prebuilt
function(android_crosvm_build DEP)
  message(STATUS "building crosvm with dependency ${DEP}")
  set(CMAKE_CROSVM_HOST_PACKAGE_TOOLS_PATH
      "${AOSP_ROOT}/prebuilts/android-emulator-build/common/cf-host-package"
  )
  set(CMAKE_CROSVM_BUILD_SCRIPT_PATH
      "${CMAKE_CROSVM_HOST_PACKAGE_TOOLS_PATH}/crosvm-build.sh")

  set(CMAKE_CROSVM_REPO_TOP_LEVEL_PATH "${AOSP_ROOT}")
  set(CMAKE_CROSVM_BUILD_ENV_DIR "${CMAKE_BINARY_DIR}/crosvm-build-env")
  set(CMAKE_CROSVM_GFXSTREAM_BUILD_DIR "${CMAKE_BINARY_DIR}")
  set(CMAKE_CROSVM_DIST_DIR "${CMAKE_INSTALL_PREFIX}")
  add_custom_command(
    OUTPUT "${CMAKE_CROSVM_BUILD_ENV_DIR}/release/crosvm"
    COMMAND
      "${CMAKE_CROSVM_BUILD_SCRIPT_PATH}" "${CMAKE_CROSVM_REPO_TOP_LEVEL_PATH}"
      "${CMAKE_CROSVM_HOST_PACKAGE_TOOLS_PATH}" "${CMAKE_CROSVM_BUILD_ENV_DIR}"
      "${CMAKE_CROSVM_GFXSTREAM_BUILD_DIR}" "${CMAKE_CROSVM_DIST_DIR}"
    DEPENDS ${DEP})
endfunction()

# Copies the list of test data files to the given destination The test data
# resides in the prebuilts/android-emulator- build/common/testdata folder.
#
# TGT The target as part of which the test files should be copied. SOURCE_LIST
# The list of files that need to be copied DEST The subdirectory under TGT where
# the files should be copied to.
function(android_copy_test_files TGT SOURCE_LIST DEST)
  get_filename_component(
    TESTDATA_ROOT
    "${AOSP_ROOT}/prebuilts/android-emulator-build/common/"
    ABSOLUTE)

  foreach(SRC_FILE ${SOURCE_LIST})
    get_filename_component(DEST_FILE ${SRC_FILE} NAME)
    add_custom_command(
      TARGET ${TGT}
      PRE_BUILD
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different ${TESTDATA_ROOT}/${SRC_FILE}
        $<TARGET_FILE_DIR:${TGT}>/${DEST}/${DEST_FILE})
  endforeach()
endfunction()

# Copies the given file from ${SRC} -> ${DST} if needed.
function(android_copy_file TGT SRC DST)
  add_custom_command(
    TARGET ${TGT} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                    ${SRC} ${DST} MAIN_DEPENDENCY ${SRC})
endfunction()

# Copies the given test directory to the given destination. The test data
# resides in the prebuilts/android-emulator- build/common/testdata folder.
#
# TGT The target as part of which the test files should be copied. SRC_DIR The
# source directories to copy., DST_DIR The subdirectory under TGT where the
# files should be copied to.
function(android_copy_test_dir TGT SRC_DIR DST_DIR)
  get_filename_component(
    TESTDATA_ROOT
    "${AOSP_ROOT}/prebuilts/android-emulator-build/common/testdata"
    ABSOLUTE)

  add_custom_command(
    TARGET ${TGT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TESTDATA_ROOT}/${SRC_DIR}
            $<TARGET_FILE_DIR:${TGT}>/${DST_DIR})
endfunction()
