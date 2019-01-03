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

# This function is the same as target_compile_definitions
# (https://cmake.org/cmake/help/v3.5/command/target_compile_definitions.html) The only difference is that the
# definitions will only be applied if the OS parameter matches the ANDROID_TARGET_TAG or compiler variable.
function(android_target_compile_definitions TGT OS MODIFIER ITEMS)
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
function(android_add_library name)
  if((ANDROID_TARGET_TAG MATCHES "windows_msvc.*") AND (DEFINED ${name}_windows_msvc_src))
    list(APPEND ${name}_src ${${name}_windows_msvc_src})
  endif()
  if((ANDROID_TARGET_TAG MATCHES "windows.*") AND (DEFINED ${name}_windows_src))
    list(APPEND ${name}_src ${${name}_windows_src})
  endif()
  if(DEFINED ${name}_${ANDROID_TARGET_TAG}_src)
    list(APPEND ${name}_src ${${name}_${ANDROID_TARGET_TAG}_src})
  endif()
  add_library(${name} ${${name}_src})
endfunction()

# Adds an object library with the given name. The source files for this target will be resolved as follows: The variable
# ${name}_src should have the set of sources The variable ${name}_${ANDROID_TARGET_TAG}_src should have the sources only
# specific for the given target.
#
# An object library is not a "real" library, but just a collection objects that can be included in a dependency.
function(android_add_object_library name)
  if((ANDROID_TARGET_TAG MATCHES "windows_msvc.*") AND (DEFINED ${name}_windows_msvc_src))
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

  # We are not registering tests when cross compiling to windows - The build bots do not have wine installed - Some
  # tests are flaky under wine
  if(ANDROID_TARGET_TAG MATCHES "windows.*" AND NOT MSVC)
    return()
  endif()

  # Configure the test to run with asan..
  file(READ "${ANDROID_QEMU2_TOP_DIR}/android/asan_overrides" ASAN_OVERRIDES)
  add_test(NAME ${name}
           COMMAND $<TARGET_FILE:${name}> --gtest_output=xml:$<TARGET_FILE_NAME:${name}>.xml
           WORKING_DIRECTORY $<TARGET_FILE_DIR:${name}>)
  set_property(TEST ${name} PROPERTY ENVIRONMENT "ASAN_OPTIONS=${ASAN_OVERRIDES}")
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "LLVM_PROFILE_FILE=$<TARGET_FILE_NAME:${name}>.profraw")

  if(MSVC)
    # Let's include the .dll path for our test runner and set a timeout so we are guaranteed to complete the tests
    string(REPLACE "/" "\\" WIN_PATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY};${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_mesa;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/qt/lib")
    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH};$ENV{PATH}")
    set_property(TEST ${name} PROPERTY TIMEOUT 300)
  endif()


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
  target_include_directories(${name} PUBLIC ${PROTOBUF_INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(${name} PUBLIC ${PROTOBUF_LIBRARIES})
  # Disable generation of information about every class with virtual functions for use by the C++ runtime type
  # identification features (dynamic_cast and typeid). If you don't use those parts of the language, you can save some
  # space by using this flag. Note that exception handling uses the same information, but it will generate it as needed.
  # The  dynamic_cast operator can still be used for casts that do not require runtime type information, i.e. casts to
  # void * or to unambiguous base classes.
  target_compile_options(${name} PRIVATE -fno-rtti)
  # This needs to be public, as we don't want the headers to start exposing exceptions.
  target_compile_definitions(${name} PUBLIC -DGOOGLE_PROTOBUF_NO_RTTI)
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
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(WARNING "Not adding ${FLGS} under visual studio compiler")
  endif()
  foreach(FLAG ${FLGS})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
  endforeach()
endfunction()

function(add_cxx_flag FLGS)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(WARNING "Not adding ${FLGS} under visual studio compiler")
  endif()
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
  if (MSVC)
    # Mission abort!
    set(${VER} "Visual Studio, unknown SHA")
    return()
  endif()
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
  if(ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
    set(${LIBCMD} "-Wl,-force_load,$<TARGET_FILE:${LIBNAME}>" PARENT_SCOPE)
  elseif(MSVC)
    set(${LIBCMD} "")
  else()
    set(${LIBCMD} "-Wl,-whole-archive" ${LIBNAME} "-Wl,-no-whole-archive" PARENT_SCOPE)
  endif()
endfunction()

# Constructs the qemu executable.
#
# ANDROID_AARCH The android architecture name
# QEMU_AARCH The qemu architecture name.
# CONFIG_AARCH The configuration architecture used.
# STUBS The set of stub sources to use.
# CPU The target cpu architecture used by qemu
#
# (Maybe on one day we will standardize all the naming, between qemu and configs and cpus..)
function(android_add_qemu_executable ANDROID_AARCH QEMU_AARCH CONFIG_AARCH STUBS CPU)
  # Workaround b/121393952, older cmake does not have proper object archives
  if (NOT MSVC)
    android_complete_archive(QEMU_COMPLETE_LIB "qemu2-common")
  endif()
  add_executable(qemu-system-${ANDROID_AARCH}
                 android-qemu2-glue/main.cpp
                 vl.c
                 ${STUBS}
                 ${qemu-system-${QEMU_AARCH}_sources}
                 ${qemu-system-${QEMU_AARCH}_generated_sources})
  target_include_directories(qemu-system-${ANDROID_AARCH} PRIVATE android-qemu2-glue/config/target-${CONFIG_AARCH} target/${CPU})
  target_compile_definitions(qemu-system-${ANDROID_AARCH} PRIVATE
      -DNEED_CPU_H -DCONFIG_ANDROID
      -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
      -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER})
  target_link_libraries(qemu-system-${ANDROID_AARCH}
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
  if (MSVC)
       # Workaround b/121393952, msvc linker does not have notion of whole-archive.
       # so we need to use the general approach supported by newer cmake versions
       target_link_libraries(qemu-system-${ANDROID_AARCH} PRIVATE $<TARGET_OBJECTS:qemu2-common>)
       set_target_properties(qemu-system-${ANDROID_AARCH}
                             PROPERTIES LINK_FLAGS "/FORCE:multiple /NODEFAULTLIB:LIBCMT")

  endif()
  # Make the common dependency explicit, as some generators might not detect it properly (Xcode)
  add_dependencies(qemu-system-${ANDROID_AARCH} qemu2-common)
  # XCode bin places this not where we want this...
  set_target_properties(qemu-system-${ANDROID_AARCH}
                        PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qemu/${ANDROID_TARGET_OS_FLAVOR}-x86_64")
  install(TARGETS qemu-system-${ANDROID_AARCH} RUNTIME DESTINATION "./qemu/${ANDROID_TARGET_OS_FLAVOR}-x86_64")
endfunction()

# Copies a
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
