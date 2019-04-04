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
include(prebuilts)

# We want to make sure all the cross targets end up in a unique location
set(ANDROID_CROSS_BUILD_DIRECTORY ${CMAKE_BINARY_DIR}/build/${ANDROID_HOST_TAG})


# Checks to make sure the TAG is valid.
function(_check_target_tag TAG)
  set(VALID_TARGETS windows windows-x86_64 windows_msvc-x86_64 linux-x86_64 darwin-x86_64 all)
  if(NOT (TAG IN_LIST VALID_TARGETS))
     message(FATAL_ERROR "The target ${TAG} does not exist, has to be one of: ${VALID_TARGETS}")
  endif()
endfunction()

# Cross compiles the given cmake project if needed.
#
# EXE the name of the target we are interested in. This is
# the main build product you want to use.
# SOURCE the location of the CMakeList.txt describing the
# project.
# OUT_PATH Name of the variable that will contain the resulting
# executable.
function(android_compile_for_host EXE SOURCE OUT_PATH)
  if(NOT CROSSCOMPILE)
    # We can add this project without any translation..
    if (NOT TARGET ${EXE})
        message(STATUS "Adding ${EXE} as subproject, not cross compiling.")
        add_subdirectory(${SOURCE} ${EXE}_ext)
    endif()
    set(${OUT_PATH} "$<TARGET_FILE:${EXE}>" PARENT_SCOPE)
  else()
    include(ExternalProject)

    # If we are cross compiling we will need to build it for our actual OS we are currently running on.
    get_filename_component(BUILD_PRODUCT ${ANDROID_CROSS_BUILD_DIRECTORY}/${EXE}_ext_cross/src/${EXE}_ext_cross-build/${EXE} ABSOLUTE)
    if (NOT TARGET ${EXE}_ext_cross)
      message(STATUS "Cross compiling ${EXE} for host ${ANDROID_HOST_TAG}")
      externalproject_add(${EXE}_ext_cross
            PREFIX ${ANDROID_CROSS_BUILD_DIRECTORY}/${EXE}_ext_cross
            DOWNLOAD_COMMAND ""
            SOURCE_DIR ${SOURCE}
            CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/toolchain-${ANDROID_HOST_TAG}.cmake"
            BUILD_BYPRODUCTS ${BUILD_PRODUCT}
            TEST_BEFORE_INSTALL True
            INSTALL_COMMAND "")
    endif()
    set(${OUT_PATH} ${BUILD_PRODUCT} PARENT_SCOPE)
   endif()
endfunction()

# Enable the compilation of .asm files using Yasm. This will include the YASM
# project if needed to compile the assembly files.
#
# The following parameters are accepted
#
# ``TARGET`` The library target to generate.
# ``INCLUDES`` Optional list of include paths to pass to yasm
# ``SOURCES`` List of source files to be compiled.
#
# For example:
# android_yasm_compile(TARGET foo_asm INCLUDES /tmp/foo /tmp/more_foo SOURCES /tmp/bar /tmp/z)
#
# Yasm will be compiled for the HOST platform if needed.
function(android_yasm_compile)
    # Parse arguments
    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs INCLUDES SOURCES)
    cmake_parse_arguments(android_yasm_compile "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # Configure yasm
    android_compile_for_host(yasm ${ANDROID_QEMU2_TOP_DIR}/../yasm YASM_EXECUTABLE)

    # Setup the includes.
    set(LIBNAME ${android_yasm_compile_TARGET})
    set(ASM_INC "")
    foreach(INCLUDE ${android_yasm_compile_INCLUDES})
        set(ASM_INC ${ASM_INC} -I ${INCLUDE})
    endforeach()

    # Configure the yasm compile command.
    foreach(asm ${android_yasm_compile_SOURCES})
        get_filename_component(asm_base ${asm} NAME_WE)
        set(DST ${CMAKE_CURRENT_BINARY_DIR}/${asm_base}.o)
        add_custom_command(OUTPUT ${DST}
                           COMMAND ${YASM_EXECUTABLE} -f ${ANDROID_YASM_TYPE} -o ${DST} ${asm} ${ASM_INC}
                           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                           VERBATIM
                           DEPENDS ${YASM_EXECUTABLE} ${asm})
        list(APPEND ${LIBNAME}_asm_o ${DST})
    endforeach()

    # Make the library available
    add_library(${LIBNAME} ${${LIBNAME}_asm_o})
    set_target_properties(${LIBNAME} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()


# This function is the same as target_compile_definitions
# (https://cmake.org/cmake/help/v3.5/command/target_compile_definitions.html) The only difference is that the
# definitions will only be applied if the OS parameter matches the ANDROID_TARGET_TAG or compiler variable.
function(android_target_compile_definitions TGT OS MODIFIER ITEMS)
  _check_target_tag(${OS})
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all" OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    target_compile_definitions(${TGT} ${MODIFIER} ${ITEMS})
    foreach(DEF ${ARGN})
      target_compile_definitions(${TGT} ${MODIFIER} ${DEF})
    endforeach()
  endif()
endfunction()

# This function is the same as target_link_libraries
# (https://cmake.org/cmake/help/v3.5/command/target_link_libraries.html) The only difference is that the definitions
# will only be applied if the OS parameter matches the ANDROID_TARGET_TAG or Compiler variable.
function(android_target_link_libraries TGT OS MODIFIER ITEMS)
  if(ARGC GREATER "49")
    message(
      FATAL_ERROR "Currently cannot link more than 49 dependecies due to some weirdness with calling target_link_libs")
  endif()
  _check_target_tag(${OS})
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all" OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    # HACK ATTACK! We cannot properly expand unknown linker args as they are treated in a magical fashion. Some
    # arguments need to be "grouped" together somehow (for example optimized;lib) since we cannot resolve this properly
    # we just pass on the individual arguments..
    target_link_libraries(${TGT} ${MODIFIER} ${ARGV3} ${ARGV4} ${ARGV5} ${ARGV6} ${ARGV7} ${ARGV8} ${ARGV9}
                          ${ARGV10} ${ARGV11} ${ARGV12} ${ARGV13} ${ARGV14} ${ARGV15} ${ARGV16} ${ARGV17} ${ARGV18} ${ARGV19}
                          ${ARGV20} ${ARGV21} ${ARGV22} ${ARGV23} ${ARGV24} ${ARGV25} ${ARGV26} ${ARGV27} ${ARGV28} ${ARGV29}
                          ${ARGV30} ${ARGV31} ${ARGV32} ${ARGV33} ${ARGV34} ${ARGV35} ${ARGV36} ${ARGV37} ${ARGV38} ${ARGV39}
                          ${ARGV40} ${ARGV41} ${ARGV42} ${ARGV43} ${ARGV44} ${ARGV45} ${ARGV46} ${ARGV47} ${ARGV48} ${ARGV49})
  endif()
endfunction()

# This function is the same as target_include_directories
# (https://cmake.org/cmake/help/v3.5/command/target_include_directories.html) The only difference is that the
# definitions will only be applied if the OS parameter matches the ANDROID_TARGET_TAG variable.
function(android_target_include_directories TGT OS MODIFIER ITEMS)
  _check_target_tag(${OS})
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all" OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    target_include_directories(${TGT} ${MODIFIER} ${ITEMS})
    foreach(DIR ${ARGN})
      target_include_directories(${TGT} ${MODIFIER} ${DIR})
    endforeach()
  endif()
endfunction()

# This function is the same as target_compile_options
# (https://cmake.org/cmake/help/v3.5/command/target_compile_options.html) The only difference is that the definitions
# will only be applied if the OS parameter matches the ANDROID_TARGET_TAG variable.
function(android_target_compile_options TGT OS MODIFIER ITEMS)
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all" OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    target_compile_options(${TGT} ${MODIFIER} "${ITEMS};${ARGN}")
  endif()
endfunction()

# Adds a library with the given name. The source files for this target will be resolved as follows: The variable
# ${name}_src should have the set of sources The variable ${name}_${ANDROID_TARGET_TAG}_src should have the sources only
# specific for the given target.
#
# For example: set(foo_src a.c b.c)  <-- sources for foo target set(foo_windows_src d.c) <-- sources only for the
# windows target set(foo_darwin-x86_64_src d.c) <-- sources only for the darwin-x86_64 target
#
# For the PRIVATE msvc-posix-compat layer will always be made available for windows_msvc builds.
function(android_add_library name)
  if((WINDOWS_MSVC_X86_64) AND (DEFINED ${name}_windows_msvc_src))
    list(APPEND ${name}_src ${${name}_windows_msvc_src})
  endif()
  if((ANDROID_TARGET_TAG MATCHES "windows.*") AND (DEFINED ${name}_windows_src))
    list(APPEND ${name}_src ${${name}_windows_src})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_library(${name} ${${name}_src})
  if (WINDOWS_MSVC_X86_64)
    target_link_libraries(${name} PRIVATE msvc-posix-compat)
  endif()
endfunction()

# Adds an object library with the given name. The source files for this target will be resolved as follows: The variable
# ${name}_src should have the set of sources The variable ${name}_${ANDROID_TARGET_TAG}_src should have the sources only
# specific for the given target.
#
# An object library is not a "real" library, but just a collection objects that can be included in a dependency.
function(android_add_object_library name)
  if((WINDOWS_MSVC_X86_64) AND (DEFINED ${name}_windows_msvc_src))
    list(APPEND ${name}_src ${${name}_windows_msvc_src})
  endif()
  if((ANDROID_TARGET_TAG MATCHES "windows.*") AND (DEFINED ${name}_windows_src))
    list(APPEND ${name}_src ${${name}_windows_src})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_library(${name} OBJECT ${${name}_src})
endfunction()

# Discovers all the targets that are registered by this subdirectory.
#
# result: The variable containing all the targets defined by this project
# subdir: The directory of interest
function(android_discover_targets result subdir)
  if (CMAKE_VERSION VERSION_LESS "3.7.0")
    message(FATAL_ERROR "This function cannot be used by older cmake versions (${CMAKE_VERSION}<3.7.0)")
  endif()
  get_directory_property(subdirs DIRECTORY "${subdir}" SUBDIRECTORIES)
  foreach(subdir IN LISTS subdirs)
    android_discover_targets(${result} "${subdir}")
  endforeach()
  get_property(targets_in_dir DIRECTORY "${subdir}" PROPERTY BUILDSYSTEM_TARGETS)
  set(${result} ${${result}} ${targets_in_dir} PARENT_SCOPE)
endfunction()

# Adds an external project, transforming all external "executables" to include the runtime properties.
# On linux for example this will set the rpath to find libc++
#
# NOTE: This function requires CMake version > 3.7
function(android_add_subdirectory external_directory name)
  get_filename_component(abs_dir ${external_directory} ABSOLUTE)
  add_subdirectory(${abs_dir} ${name})
  android_discover_targets(targets ${abs_dir})
  foreach(target IN LISTS targets)
    get_target_property(tgt_type ${target} TYPE)
    if(tgt_type STREQUAL "EXECUTABLE")
      android_target_properties(${target} all "${RUNTIME_OS_PROPERTIES}")
    endif()
  endforeach()
endfunction()

# Adds a shared library with the given name. The source files for this target will be resolved as follows: The variable
# ${name}_src should have the set of sources The variable ${name}_${ANDROID_TARGET_TAG}_src should have the sources only
# specific for the given target.
#
# For example: set(foo_src a.c b.c)  <-- sources for foo target set(foo_windows_src d.c) <-- sources only for the
# windows target set(foo_darwin-x86_64_src d.c) <-- sources only for the darwin-x86_64 target
#
# See https://cmake.org/cmake/help/v3.5/command/add_library.html for details
function(android_add_shared_library name)
  if((ANDROID_TARGET_TAG MATCHES "windows.*") AND (DEFINED ${name}_windows_src))
    list(APPEND ${name}_src ${${name}_windows_src})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_library(${name} SHARED ${${name}_src})
  if (WINDOWS_MSVC_X86_64)
    target_link_libraries(${name} PRIVATE msvc-posix-compat)
  endif()
  # We don't want cmake to binplace the shared libraries into the bin directory As this can make them show up in
  # unexpected places!
  if(ANDROID_TARGET_TAG MATCHES "windows.*")
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  endif()
  if(CMAKE_SYSTEM_NAME MATCHES "WinMSVCCrossCompile")
    # For windows-msvc build (on linux), this generates a dll and a lib (import library) file. The files are being
    # placed at ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} which is correct for the dll, but the lib file needs to be in the
    # ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} or we can't link to it. Most windows compilers, including clang, don't allow you
    # to directly link to a dll (unlike mingw), and instead, need to link to it's import library.
    #
    # Another headache: it seems we attach a prefix to some of our shared libraries, which make cmake unable to locate
    # the import library later on to whoever tries to link to it (e.g. OpenglRender -> lib64OpenglRender), as it will
    # look for an import library by <target_library_name>.lib. So let's just move the import library to the archive
    # directory and rename it back to the library name without the prefix.
    add_custom_command(
      TARGET ${name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:${name}>/$<TARGET_LINKER_FILE_NAME:${name}>
              ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/$<TARGET_LINKER_FILE_NAME:${name}>
      COMMENT
        "Copying $<TARGET_FILE_DIR:${name}>/$<TARGET_LINKER_FILE_NAME:${name}> to ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/$<TARGET_LINKER_FILE_NAME:${name}>"
      )
    add_custom_command(TARGET ${name} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE_DIR:${name}>/$<TARGET_LINKER_FILE_NAME:${name}>
                       COMMENT "Removing $<TARGET_FILE_DIR:${name}>/$<TARGET_LINKER_FILE_NAME:${name}>")
  endif()
  if (WINDOWS_MSVC_X86_64)
    target_link_libraries(${name} PRIVATE msvc-posix-compat)
  endif()
endfunction()

# Adds an interface library with the given name. The source files for this target will be resolved as follows: The
# variable ${name}_src should have the set of sources The variable ${name}_${ANDROID_TARGET_TAG}_src should have the
# sources only specific for the given target.
#
# For example: set(foo_src a.c b.c)  <-- sources for foo target set(foo_windows_src d.c) <-- sources only for the
# windows target set(foo_darwin-x86_64_src d.c) <-- sources only for the darwin-x86_64 target See
# https://cmake.org/cmake/help/v3.5/command/add_library.html for details
function(android_add_interface name)
  add_library(${name} INTERFACE)
endfunction()


function(android_add_default_test_properties name)
  # Configure the test to run with asan..
  file(READ "${ANDROID_QEMU2_TOP_DIR}/android/asan_overrides" ASAN_OVERRIDES)
  set_property(TEST ${name} PROPERTY ENVIRONMENT "ASAN_OPTIONS=${ASAN_OVERRIDES}")
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "LLVM_PROFILE_FILE=$<TARGET_FILE_NAME:${name}>.profraw")
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "ASAN_SYMBOLIZER_PATH=${ANDROID_LLVM_SYMBOLIZER}")
  set_property(TEST ${name} PROPERTY TIMEOUT 600)

  if(WINDOWS_MSVC_X86_64)
    # Let's include the .dll path for our test runner
    string(REPLACE "/" "\\" WIN_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY};${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_mesa;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/qt/lib")
    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH};$ENV{PATH}")
  endif()
endfunction()

# Adds a test target. It will create and register the test with the given name
#
# The variable ${name}_src should have the set of sources The variable ${name}_${ANDROID_TARGET_TAG}_src should have the
# sources only specific for the given target.
#
# For example: set(foo_src a.c b.c)  <-- sources for foo target set(foo_windows_src d.c) <-- sources only for the
# windows target set(foo_darwin-x86_64_src d.c) <-- sources only for the darwin-x86_64 target See
# https://cmake.org/cmake/help/v3.5/command/add_executable.html for more details
function(android_add_test name)
  android_add_executable(${name})

  # We cannot run tests when we are cross compiling.
  if(CROSSCOMPILE)
    return()
  endif()

  add_test(NAME ${name}
           COMMAND $<TARGET_FILE:${name}> --gtest_output=xml:$<TARGET_FILE_NAME:${name}>.xml --gtest_catch_exceptions=0
           WORKING_DIRECTORY $<TARGET_FILE_DIR:${name}>)

  # Let's not optimize our tests.
  if (WINDOWS_MSVC_X86_64)
      target_compile_options(${name} PRIVATE -Od)
  else()
      target_compile_options(${name} PRIVATE -O0)
  endif()

  android_add_default_test_properties(${name})
endfunction()

# Adds an executable target. The RUNTIME_OS_DEPENDENCIES and RUNTIME_OS_PROPERTIES will registed for the given target,
# so it should be able to execute after compilation.
#
# The variable ${name}_src should have the set of sources The variable ${name}_${ANDROID_TARGET_TAG}_src should have the
# sources only specific for the given target.
#
# For example: set(foo_src a.c b.c)  <-- sources for foo target set(foo_windows_src d.c) <-- sources only for the
# windows target set(foo_darwin-x86_64_src d.c) <-- sources only for the darwin-x86_64 target See
# https://cmake.org/cmake/help/v3.5/command/add_executable.html for more details
function(android_add_executable name)
  if((ANDROID_TARGET_TAG MATCHES "windows.*") AND (DEFINED ${name}_windows_src))
    list(APPEND ${name}_src ${${name}_windows_src})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_executable(${name} ${${name}_src})
  if (WINDOWS_MSVC_X86_64)
    target_link_libraries(${name} PRIVATE msvc-posix-compat)
  endif()
  android_target_dependency(${name} all RUNTIME_OS_DEPENDENCIES)
  android_target_properties(${name} all "${RUNTIME_OS_PROPERTIES}")

  if(ANDROID_CODE_COVERAGE)
    # TODO Clean out existing .gcda files.
  endif()

endfunction()

# Adds a protobuf library with the given name. It will export all the needed headers, and libraries You can take a
# dependency on this by adding: target_link_libraries(my_target ${name}) for your target. The generated library will not
# use execeptions.
#
# name: The name of the generated library. You can take a dependency on this with setting
# target_linke_libraries(my_target ${name})
#
# protofiles: The set of protofiles to be included.
function(android_add_protobuf name protofiles)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${protofiles})
  set(${name}_src ${PROTO_SRCS} ${PROTO_HDRS})
  android_add_library(${name})
  target_link_libraries(${name} PUBLIC libprotobuf)
  target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  # Disable generation of information about every class with virtual functions for use by the C++ runtime type
  # identification features (dynamic_cast and typeid). If you don't use those parts of the language, you can save some
  # space by using this flag. Note that exception handling uses the same information, but it will generate it as needed.
  # The  dynamic_cast operator can still be used for casts that do not require runtime type information, i.e. casts to
  # void * or to unambiguous base classes.
  target_compile_options(${name} PRIVATE -fno-rtti)
  # This needs to be public, as we don't want the headers to start exposing exceptions.
  target_compile_definitions(${name} PUBLIC -DGOOGLE_PROTOBUF_NO_RTTI)
endfunction()

# For adding big proto files that mingw can't handle.
function(android_add_big_protobuf name protofiles)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${protofiles})
  set(${name}_src ${PROTO_SRCS} ${PROTO_HDRS})
  android_add_library(${name})
  target_link_libraries(${name} PUBLIC libprotobuf)
  target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  # Disable generation of information about every class with virtual functions for use by the C++ runtime type
  # identification features (dynamic_cast and typeid). If you don't use those parts of the language, you can save some
  # space by using this flag. Note that exception handling uses the same information, but it will generate it as needed.
  # The  dynamic_cast operator can still be used for casts that do not require runtime type information, i.e. casts to
  # void * or to unambiguous base classes.
  if (WINDOWS_X86_64)
    target_compile_options(${name} PRIVATE -fno-rtti "-Wa,-mbig-obj")
  else()
    target_compile_options(${name} PRIVATE -fno-rtti)
  endif()

  # This needs to be public, as we don't want the headers to start exposing exceptions.
  target_compile_definitions(${name} PUBLIC -DGOOGLE_PROTOBUF_NO_RTTI)
endfunction()

# This function generates the hw config file. It translates android- emu/androd/avd/hardware-properties.ini ->
# android/avd/hw-config-defs.h
#
# This file will be placed on the current binary dir, so it can be included if this directory is on the include path.
function(android_generate_hw_config)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                     COMMAND python ${ANDROID_QEMU2_TOP_DIR}/android/scripts/gen-hw-config.py
                             ${ANDROID_QEMU2_TOP_DIR}/android/android-emu/android/avd/hardware-properties.ini
                             ${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h
                     VERBATIM)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES GENERATED TRUE)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/android/avd/hw-config-defs.h PROPERTIES SKIP_AUTOGEN ON)
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

# Copies the given file from ${SRC} -> ${DST} if needed.
function(android_copy_file TGT SRC DST)
  add_custom_command(TARGET ${TGT} PRE_BUILD
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

# Append the given flags to the existing CMAKE_C_FLAGS. Be careful as these flags are global and used for every target!
# Note this will not do anything under vs for now
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
  execute_process(COMMAND "git" "describe"
                  WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                  RESULT_VARIABLE GIT_RES
                  OUTPUT_VARIABLE STD_OUT
                  ERROR_VARIABLE STD_ERR)
  if(NOT "${GIT_RES}" STREQUAL "0")
    message(WARNING "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR}, out: ${STD_OUT}, err: ${STD_ERR}")
    if (NOT MSVC)
      execute_process(COMMAND "date" "+%Y-%m-%d"
                      WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                      RESULT_VARIABLE DATE_RES
                      OUTPUT_VARIABLE STD_OUT
                      ERROR_VARIABLE STD_ERR)
      if(NOT "${DATE_RES}" STREQUAL "0")
        message(FATAL_ERROR "Unable to retrieve date!")
      endif()
    else()
      execute_process(COMMAND "date" "/T"
                      WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                      RESULT_VARIABLE DATE_RES
                      OUTPUT_VARIABLE STD_OUT
                      ERROR_VARIABLE STD_ERR)
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
# . LIBCMD The variable which will contain the complete linker command
# . LIBNAME The archive that needs to be included
# completely
function(android_complete_archive LIBCMD LIBNAME)
  if(DARWIN_X86_64)
    set(${LIBCMD} "-Wl,-force_load,$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
  elseif(WINDOWS_MSVC_X86_64)
    if (MSVC)
      # The native build calls the linker directly
      set(${LIBCMD} "-wholearchive:$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
    else()
      # The cross compiler calls the linker through clang.
      set(${LIBCMD} "-Wl,-wholearchive:$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
    endif()
  else()
    set(${LIBCMD} "-Wl,-whole-archive" ${LIBNAME} "-Wl,-no-whole-archive" PARENT_SCOPE)
  endif()
endfunction()

# Constructs the upstream qemu executable.
#
# ANDROID_AARCH The android architecture name
# QEMU_AARCH The qemu architecture name.
# CONFIG_AARCH The configuration architecture used.
# STUBS The set of stub sources to use.
# CPU The target cpu architecture used by qemu
#
# (Maybe on one day we will standardize all the naming, between qemu and configs and cpus..)
function(android_build_qemu_variant)
    # Parse arguments
    set(options INSTALL)
    set(oneValueArgs CPU EXE)
    set(multiValueArgs SOURCES DEFINITIONS LIBRARIES)
    cmake_parse_arguments(qemu_build "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  ## translate various names.. because of inconsistent naming in the code base..
  if (qemu_build_CPU STREQUAL "x86_64")
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
  set(${qemu_build_EXE}_src ${qemu_build_SOURCES}
      ${qemu-system-${QEMU_AARCH}_sources}
      ${qemu-system-${QEMU_AARCH}_generated_sources}
  )
  android_add_executable(${qemu_build_EXE})
  target_include_directories(${qemu_build_EXE}
                             PRIVATE android-qemu2-glue/config/target-${CONFIG_AARCH} target/${CPU})
  target_compile_definitions(${qemu_build_EXE} PRIVATE ${qemu_build_DEFINITIONS})
  target_link_libraries(${qemu_build_EXE}
                        PRIVATE ${QEMU_COMPLETE_LIB} ${qemu_build_LIBRARIES})

  # Make the common dependency explicit, as some generators might not detect it properly (Xcode/MSVC native)
  add_dependencies(${qemu_build_EXE} qemu2-common)
  # XCode bin places this not where we want this...
  if (qemu_build_INSTALL)
    set_target_properties(${qemu_build_EXE}
                          PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qemu/${ANDROID_TARGET_OS_FLAVOR}-x86_64")
    android_install_exe(${qemu_build_EXE} "./qemu/${ANDROID_TARGET_OS_FLAVOR}-x86_64")
  endif()
endfunction()

# Constructs the qemu executable.
#
# ANDROID_AARCH The android architecture name
# STUBS The set of stub sources to use.
function(android_add_qemu_executable ANDROID_AARCH STUBS)
  android_build_qemu_variant(INSTALL
                             EXE qemu-system-${ANDROID_AARCH}
                             CPU ${ANDROID_AARCH}
                             SOURCES android-qemu2-glue/main.cpp vl.c ${STUBS}
                             DEFINITIONS -DNEED_CPU_H
                                         -DCONFIG_ANDROID
                                         -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
                                         -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}
                             LIBRARIES   libqemu2-glue libqemu2-glue-vm-operations
                                         libqemu2-util
                                         emulator-libui
                                         android-emu
                                         OpenGLESDispatch
                                         android-qemu-deps
                                         android-qemu-deps-headful)
endfunction()


# Constructs the qemu headless executable.
#
# ANDROID_AARCH The android architecture name
# STUBS The set of stub sources to use.
function(android_add_qemu_headless_executable ANDROID_AARCH STUBS)
  android_build_qemu_variant(INSTALL
                             EXE qemu-system-${ANDROID_AARCH}-headless
                             CPU ${ANDROID_AARCH}
                             SOURCES android-qemu2-glue/main.cpp vl.c ${STUBS}
                             DEFINITIONS -DNEED_CPU_H
                                         -DCONFIG_ANDROID
                                         -DCONFIG_HEADLESS
                                         -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
                                         -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}
                             LIBRARIES  libqemu2-glue libqemu2-glue-vm-operations
                                        libqemu2-util
                                        android-emu
                                        emulator-libui-headless
                                        OpenGLESDispatch
                                        android-qemu-deps
                                        android-qemu-deps-headless)
endfunction()


# Constructs the qemu upstream executable.
#
# ANDROID_AARCH The android architecture name
# STUBS The set of stub sources to use.
function(android_add_qemu_upstream_executable ANDROID_AARCH STUBS)
  android_build_qemu_variant( #INSTALL We do not install this target.
                             EXE qemu-upstream-${ANDROID_AARCH}
                             CPU ${ANDROID_AARCH}
                             SOURCES vl.c ${STUBS}
                             DEFINITIONS -DNEED_CPU_H
                             LIBRARIES   android-emu
                                         libqemu2-glue libqemu2-glue-vm-operations
                                         libqemu2-util
                                         SDL2::SDL2
                                         android-qemu-deps
                                         android-qemu-deps-headful)
endfunction()


# Copies a shared library
function(android_copy_shared_lib TGT SHARED_LIB NAME)
  android_copy_file(${TGT} $<TARGET_FILE:${SHARED_LIB}>
                    $<TARGET_FILE_DIR:${TGT}>/lib64/${NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
endfunction()

function(android_log MSG)
  if(ANDROID_LOG)
    message(STATUS ${MSG})
  endif()
endfunction()

function(android_validate_sha256 FILE EXPECTED)
  file(SHA256 ${FILE} CHECKSUM)
  if(NOT CHECKSUM STREQUAL "${EXPECTED}")
    get_filename_component(DEST "${ANDROID_QEMU2_TOP_DIR}/../../device/generic/goldfish-opengl" ABSOLUTE)
    message(
      FATAL_ERROR
        "Checksum mismatch for ${FILE} = ${CHECKSUM}, expecting ${EXPECTED}, you need to regenerate the cmake files by executing 'make' in ${DEST}"
      )
  endif()
endfunction()

# Uploads the symbols to the breakpad crash server
function(android_upload_symbols TGT)
  if(NOT ANDROID_EXTRACT_SYMBOLS)
    return()
  endif()
  set(STDOUT "/dev/stdout")
  if (WINDOWS_MSVC_X86_64 AND NOT CROSSCOMPILE)
    set(STDOUT "CON")
  endif()
  set(DEST "${ANDROID_SYMBOL_DIR}/${TGT}.sym")
  install(
    CODE
    # Upload the symbols, with warnings/error logging only.
    "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\"
                             \"${ANDROID_QEMU2_TOP_DIR}/android/build/python/aemu/upload_symbols.py\"
                             \"-v\" \"0\"
                             \"--symbol_file\" \"${DEST}\"
                             \"--environment\" \"${OPTION_CRASHUPLOAD}\"
                             OUTPUT_FILE ${STDOUT}
                             ERROR_FILE ${STDOUT})"
  )
endfunction()

# Installs the given target executable into the given destinations. Symbols will be extracted during build, and uploaded
# during install.
function(android_install_exe TGT DST)
  install(TARGETS ${TGT} RUNTIME DESTINATION ${DST})

  # Make it available on the build server
  android_extract_symbols(${TGT})
  android_upload_symbols(${TGT})
endfunction()

# Installs the given shared library. The shared library will end up in ../lib64 Symbols will be extracted during build,
# and uploaded during install.
function(android_install_shared TGT)
  install(TARGETS ${TGT}
          RUNTIME DESTINATION lib64 # We don't want windows to binplace dlls in the exe dir
          LIBRARY DESTINATION lib64)
  android_extract_symbols(${TGT})
  android_upload_symbols(${TGT})
endfunction()

# Strips the given prebuilt executable during install..
function(android_strip_prebuilt FNAME)
  # MSVC stores debug info in seperate file, so no need to strip
  if(NOT WINDOWS_MSVC_X86_64)
    install(CODE "if(CMAKE_INSTALL_DO_STRIP) \n
                        execute_process(COMMAND ${CMAKE_STRIP} \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${FNAME}\")\n
                      endif()\n
                     ")
  endif()
endfunction()

# Extracts symbols from a file that is not built. This is mainly here if we wish to extract symbols for a prebuilt file.
function(android_extract_symbols_file FNAME)
  get_filename_component(BASENAME ${FNAME} NAME)
  set(DEST "${ANDROID_SYMBOL_DIR}/${BASENAME}.sym")

  if(WINDOWS_MSVC_X86_64)
    # In msvc we need the pdb to generate the symbols, pdbs are not yet available for The prebuilts. b/122728651
    message(WARNING "Extracting symbols requires access to the pdb for ${FNAME}, ignoring for now.")
    return()
  endif()
  install(
    CODE
    "execute_process(COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dump_syms -g ${FNAME} OUTPUT_FILE ${DEST} RESULT_VARIABLE RES ERROR_QUIET) \n
                 message(STATUS \"Extracted symbols for ${FNAME} ${RES}\")")
  install(
    CODE
    "execute_process(COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sym_upload ${DEST} ${ANDROID_SYMBOL_URL}  OUTPUT_VARIABLE RES ERROR_QUIET) \n
                 message(STATUS \"Uploaded symbols for ${FNAME} --> ${ANDROID_SYMBOL_URL} ${RES}\")")
endfunction()

# Extracts the symbols from the given target if extraction is requested. TODO: We need generator expressions to move
# this to the install phase. Which are available in cmake 3.13
function(android_extract_symbols TGT)
  if(NOT ANDROID_EXTRACT_SYMBOLS)
    # Note: we do not need to extract symbols on windows for uploading.
    return()
  endif()
  set(DEV_NULL "/dev/null")
  if (WINDOWS_MSVC_X86_64 AND NOT CROSSCOMPILE)
      set(DEV_NULL "nul")
  endif()

  set(DEST "${ANDROID_SYMBOL_DIR}/${TGT}.sym")
  add_custom_command(TARGET ${TGT} POST_BUILD
                     COMMAND dump_syms "$<TARGET_FILE:${TGT}>" > ${DEST} 2> ${DEV_NULL}
                     DEPENDS dump_syms
                     COMMENT "Extracting symbols for ${TGT}"
                     VERBATIM)
endfunction()

# Make the compatibility layer available for every target
if (WINDOWS_MSVC_X86_64)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/msvc-posix-compat/ msvc-posix-compat)
endif()

