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
if(INCLUDE_ANDROID_CMAKE)
  return()
endif()

set(INCLUDE_ANDROID_CMAKE 1)

include(prebuilts)

# We want to make sure all the cross targets end up in a unique location
set(ANDROID_CROSS_BUILD_DIRECTORY ${CMAKE_BINARY_DIR}/build/${ANDROID_HOST_TAG})

# Checks to make sure the TAG is valid.
function(_check_target_tag TAG)
  set(VALID_TARGETS
      windows
      windows_msvc-x86_64
      linux-x86_64
      linux-aarch64
      darwin-x86_64
      all
      Clang)
  if(NOT (TAG IN_LIST VALID_TARGETS))
    message(
      FATAL_ERROR
        "The target ${TAG} does not exist, has to be one of: ${VALID_TARGETS}")
  endif()
endfunction()

# Cross compiles the given cmake project if needed.
#
# EXE the name of the target we are interested in. This is the main build
# product you want to use. SOURCE the location of the CMakeList.txt describing
# the project. OUT_PATH Name of the variable that will contain the resulting
# executable.
function(android_compile_for_host EXE SOURCE OUT_PATH)
  if(NOT CROSSCOMPILE)
    # We can add this project without any translation..
    if(NOT TARGET ${EXE})
      message(STATUS "Adding ${EXE} as subproject, not cross compiling.")
      add_subdirectory(${SOURCE} ${EXE}_ext)
    endif()
    set(${OUT_PATH} "$<TARGET_FILE:${EXE}>" PARENT_SCOPE)
  else()
    include(ExternalProject)

    # If we are cross compiling we will need to build it for our actual OS we
    # are currently running on.
    get_filename_component(
      BUILD_PRODUCT
      ${ANDROID_CROSS_BUILD_DIRECTORY}/${EXE}_ext_cross/src/${EXE}_ext_cross-build/${EXE}
      ABSOLUTE)
    if(NOT TARGET ${EXE}_ext_cross)
      message(STATUS "Cross compiling ${EXE} for host ${ANDROID_HOST_TAG}")
      externalproject_add(
        ${EXE}_ext_cross
        PREFIX ${ANDROID_CROSS_BUILD_DIRECTORY}/${EXE}_ext_cross
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${SOURCE}
        CMAKE_ARGS
          "-DCMAKE_TOOLCHAIN_FILE=${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/toolchain-${ANDROID_HOST_TAG}.cmake"
        BUILD_BYPRODUCTS ${BUILD_PRODUCT}
        TEST_BEFORE_INSTALL True
        INSTALL_COMMAND "")
    endif()
    set(${OUT_PATH} ${BUILD_PRODUCT} PARENT_SCOPE)
  endif()
endfunction()

# ~~~
# Enable the compilation of .asm files using nasm. This will include the nasm project if needed to compile the assembly
# files.
#
# The following parameters are accepted
#
# ``TARGET`` The library target to generate.
# ``INCLUDES`` Optional list of include paths to pass to nasm
# ``SOURCES`` List of source files to be compiled.
#
# For example:
#    android_nasm_compile(TARGET foo_asm INCLUDES /tmp/foo /tmp/more_foo SOURCES /tmp/bar /tmp/z)
#
# nasm will be compiled for the HOST platform if needed.
# ~~~
function(android_nasm_compile)
  _register_target(${ARGN})
  # Parse arguments
  set(options)
  set(oneValueArgs TARGET)
  set(multiValueArgs INCLUDES SOURCES)
  cmake_parse_arguments(android_nasm_compile "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  # Configure nasm
  android_compile_for_host(
    nasm ${ANDROID_QEMU2_TOP_DIR}/android/third_party/nasm nasm_EXECUTABLE)

  # Setup the includes.
  set(LIBNAME ${android_nasm_compile_TARGET})
  set(ASM_INC "")
  foreach(INCLUDE ${android_nasm_compile_INCLUDES})
    set(ASM_INC ${ASM_INC} -I ${INCLUDE})
  endforeach()

  # Configure the nasm compile command.
  foreach(asm ${REGISTERED_SRC})
    get_filename_component(asm_base ${asm} NAME_WE)
    set(DST ${CMAKE_CURRENT_BINARY_DIR}/${asm_base}.o)
    add_custom_command(
      OUTPUT ${DST}
      COMMAND ${nasm_EXECUTABLE} -f ${ANDROID_ASM_TYPE} -o ${DST} ${asm}
              ${ASM_INC}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
      DEPENDS ${nasm_EXECUTABLE} ${asm})
    list(APPEND ${LIBNAME}_asm_o ${DST})
  endforeach()

  # Make the library available
  add_library(${LIBNAME} ${${LIBNAME}_asm_o})
  set_target_properties(${LIBNAME} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

# This function is the same as target_compile_definitions
# (https://cmake.org/cmake/help/v3.5/command/target_compile_definitions.html)
# The only difference is that the definitions will only be applied if the OS
# parameter matches the ANDROID_TARGET_TAG or compiler variable.
function(android_target_compile_definitions TGT OS MODIFIER ITEMS)
  _check_target_tag(${OS})
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all"
     OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    target_compile_definitions(${TGT} ${MODIFIER} ${ITEMS})
    foreach(DEF ${ARGN})
      target_compile_definitions(${TGT} ${MODIFIER} ${DEF})
    endforeach()
  endif()
endfunction()

# This function is the same as target_link_libraries
# (https://cmake.org/cmake/help/v3.5/command/target_link_libraries.html) T he
# only difference is that the definitions will only be applied if the OS
# parameter matches the ANDROID_TARGET_TAG or Compiler variable.
function(android_target_link_libraries TGT OS MODIFIER ITEMS)
  if(ARGC GREATER "49")
    message(
      FATAL_ERROR
        "Currently cannot link more than 49 dependecies due to some weirdness with calling target_link_libs"
    )
  endif()
  _check_target_tag(${OS})
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all"
     OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    # HACK ATTACK! We cannot properly expand unknown linker args as they are
    # treated in a magical fashion. Some arguments need to be "grouped" together
    # somehow (for example optimized;lib) since we cannot resolve this properly
    # we just pass on the individual arguments..
    # cmake-format: off
    target_link_libraries(${TGT} ${MODIFIER} ${ARGV3} ${ARGV4} ${ARGV5} ${ARGV6} ${ARGV7} ${ARGV8} ${ARGV9}
                          ${ARGV10} ${ARGV11} ${ARGV12} ${ARGV13} ${ARGV14} ${ARGV15} ${ARGV16} ${ARGV17} ${ARGV18} ${ARGV19}
                          ${ARGV20} ${ARGV21} ${ARGV22} ${ARGV23} ${ARGV24} ${ARGV25} ${ARGV26} ${ARGV27} ${ARGV28} ${ARGV29}
                          ${ARGV30} ${ARGV31} ${ARGV32} ${ARGV33} ${ARGV34} ${ARGV35} ${ARGV36} ${ARGV37} ${ARGV38} ${ARGV39}
                          ${ARGV40} ${ARGV41} ${ARGV42} ${ARGV43} ${ARGV44} ${ARGV45} ${ARGV46} ${ARGV47} ${ARGV48} ${ARGV49})
    # cmake-format: on
  endif()
endfunction()

# This function is the same as target_include_directories
# (https://cmake.org/cmake/help/v3.5/command/target_include_directories.html)
# The only difference is that the definitions will only be applied if the OS
# parameter matches the ANDROID_TARGET_TAG variable.
function(android_target_include_directories TGT OS MODIFIER ITEMS)
  _check_target_tag(${OS})
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all"
     OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    target_include_directories(${TGT} ${MODIFIER} ${ITEMS})
    foreach(DIR ${ARGN})
      target_include_directories(${TGT} ${MODIFIER} ${DIR})
    endforeach()
  endif()
endfunction()

# This function is the same as target_compile_options
# (https://cmake.org/cmake/help/v3.5/command/target_compile_options.html) The
# only difference is that the definitions will only be applied if the OS
# parameter matches the ANDROID_TARGET_TAG variable.
function(android_target_compile_options TGT OS MODIFIER ITEMS)
  _check_target_tag(${OS})
  if(ANDROID_TARGET_TAG MATCHES "${OS}.*" OR OS STREQUAL "all"
     OR OS MATCHES "${CMAKE_CXX_COMPILER_ID}")
    target_compile_options(${TGT} ${MODIFIER} "${ITEMS};${ARGN}")
  endif()
endfunction()

# ~~~
# Registers the given target, by calculating the source set and setting licensens.
#
# ``TARGET``  The library/executable target. For example emulatory-libyuv
# ``LIBNAME`` Public library name, this is how it is known in the world. For example libyuv.
# ``SOURCES`` List of source files to be compiled, part of every target.
# ``LINUX``   List of source files to be compiled if the current target is LINUX_X86_64
# ``DARWIN``  List of source files to be compiled if the current target is DARWIN_X86_64
# ``WINDOWS`` List of source files to be compiled if the current target is WINDOWS_MSVC_X86_64
# ``MSVC``    List of source files to be compiled if the current target is  WINDOWS_MSVC_X86_64
# ``URL``     URL where the source code can be found.
# ``REPO``    Internal location where the, where the notice can be found.
# ``LICENSE`` SPDX License identifier.
# ``NOTICE``  Location where the NOTICE can be found
# ~~~
function(_register_target)
  set(options NODISTRIBUTE)
  set(oneValueArgs TARGET LICENSE LIBNAME REPO URL NOTICE)
  set(multiValueArgs SRC LINUX MSVC WINDOWS DARWIN)
  cmake_parse_arguments(build "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})
  if(NOT DEFINED build_TARGET)
    message(FATAL_ERROR "Undefined target for library.")
  endif()

  set(src ${build_SRC})
  if(LINUX AND build_LINUX)
    list(APPEND src ${build_LINUX})
  elseif(DARWIN_X86_64 AND build_DARWIN)
    list(APPEND src ${build_DARWIN})
  elseif(WINDOWS_MSVC_X86_64 AND (build_MSVC OR build_WINDOWS))
    list(APPEND src ${build_MSVC} ${build_WINDOWS})
  endif()

  set(REGISTERED_SRC ${src} PARENT_SCOPE)
  if(build_NODISTRIBUTE)
    return()
  endif()

  if(NOT DEFINED build_LIBNAME)
    set(build_LIBNAME ${build_TARGET})
  endif()

  if(NOT DEFINED build_LICENSE)
    message(
      FATAL_ERROR
        "You must set a license for ${build_TARGET}, or declare NODISTRIBUTE")
  endif()

  if(NOT DEFINED build_NOTICE)
    set(build_URL
        "https://android.googlesource.com/platform/external/qemu.git/+/refs/heads/emu-master-dev"
    )
    set(build_REPO "INTERNAL")
    set(build_NOTICE "REPO/LICENSES/LICENSE.APACHE2")
  endif()

  if(NOT DEFINED build_URL OR NOT DEFINED build_REPO OR NOT DEFINED
                                                        build_NOTICE)
    message(
      FATAL_ERROR
        "You must set a notice/url/repo for ${build_TARGET}, or declare NODISTRIBUTE"
    )
  endif()
  if(DEFINED build_NOTICE AND NOT ${build_NODISTRIBUTE})
    string(REPLACE "REPO" "${build_URL}" REMOTE_LICENSE ${build_NOTICE})
    string(REPLACE "REPO" "${build_REPO}" LOCAL_LICENSE ${build_NOTICE})
    android_license(
      TARGET ${build_TARGET} URL ${build_URL} LIBNAME ${build_LIBNAME}
      SPDX ${build_LICENSE} LICENSE ${REMOTE_LICENSE} LOCAL ${LOCAL_LICENSE})
  endif()
endfunction()

# ~~~
# Registers the given library, by calculating the source set and setting licensens.
#
# ``SHARED``  Option indicating that this is a shared library.
# ``TARGET``  The library/executable target. For example emulatory-libyuv
# ``LIBNAME`` Public library name, this is how it is known in the world. For example libyuv.
# ``SOURCES`` List of source files to be compiled, part of every target.
# ``LINUX``   List of source files to be compiled if the current target is LINUX_X86_64
# ``DARWIN``  List of source files to be compiled if the current target is DARWIN_X86_64
# ``WINDOWS`` List of source files to be compiled if the current target is WINDOWS_MSVC_X86_64
# ``MSVC``    List of source files to be compiled if the current target is  WINDOWS_MSVC_X86_64
# ``URL``     URL where the source code can be found.
# ``REPO``    Internal location where the, where the notice can be found.
# ``LICENSE`` SPDX License identifier.
# ``NOTICE``  Location where the NOTICE can be found
# ~~~
function(android_add_library)
  _register_target(${ARGN})
  set(options SHARED)
  set(oneValueArgs TARGET)
  set(multiValueArgs "")
  cmake_parse_arguments(build "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})
  if(${build_SHARED})
    add_library(${build_TARGET} SHARED ${REGISTERED_SRC})
  else()
    add_library(${build_TARGET} ${REGISTERED_SRC})
  endif()

  if(WINDOWS_MSVC_X86_64 AND NOT ${build_TARGET} STREQUAL "msvc-posix-compat")
    target_link_libraries(${build_TARGET} PRIVATE msvc-posix-compat)
  endif()
  android_clang_tidy(${build_TARGET})

  if(${build_SHARED})
    # We don't want cmake to binplace the shared libraries into the bin
    # directory As this can make them show up in unexpected places!
    if(ANDROID_TARGET_TAG MATCHES "windows.*")
      set_target_properties(
        ${build_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    endif()
    if(CROSSCOMPILE AND WINDOWS_MSVC_X86_64)
      # For windows-msvc build (on linux), this generates a dll and a lib
      # (import library) file. The files are being placed at
      # ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} which is correct for the dll, but the
      # lib file needs to be in the ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} or we
      # can't link to it. Most windows compilers, including clang, don't allow
      # you to directly link to a dll (unlike mingw), and instead, need to link
      # to it's import library.
      #
      # Another headache: it seems we attach a prefix to some of our shared
      # libraries, which make cmake unable to locate the import library later on
      # to whoever tries to link to it (e.g. OpenglRender -> lib64OpenglRender),
      # as it will look for an import library by <target_library_name>.lib. We
      # just symlink things to make it work.
      add_custom_command(
        TARGET ${build_TARGET}
        POST_BUILD
        COMMAND
          ${CMAKE_COMMAND} -E create_symlink
          $<TARGET_FILE_DIR:${build_TARGET}>/${CMAKE_SHARED_LIBRARY_PREFIX}$<TARGET_LINKER_FILE_NAME:${build_TARGET}>
          ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/$<TARGET_LINKER_FILE_NAME:${build_TARGET}>
        COMMENT
          "ln -sf $<TARGET_FILE_DIR:${build_TARGET}>/${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/$<TARGET_LINKER_FILE_NAME:${build_TARGET}> ${CMAKE_SHARED_LIBRARY_PREFIX}$<TARGET_LINKER_FILE_NAME:${build_TARGET}>"
      )
    endif()
  endif()
endfunction()

# Discovers all the targets that are registered by this subdirectory.
#
# result: The variable containing all the targets defined by this project
# subdir: The directory of interest
function(android_discover_targets result subdir)
  if(CMAKE_VERSION VERSION_LESS "3.7.0")
    message(
      FATAL_ERROR
        "This function cannot be used by older cmake versions (${CMAKE_VERSION}<3.7.0)"
    )
  endif()
  get_directory_property(subdirs DIRECTORY "${subdir}" SUBDIRECTORIES)
  foreach(subdir IN LISTS subdirs)
    android_discover_targets(${result} "${subdir}")
  endforeach()
  get_property(targets_in_dir DIRECTORY "${subdir}"
               PROPERTY BUILDSYSTEM_TARGETS)
  set(${result} ${${result}} ${targets_in_dir} PARENT_SCOPE)
endfunction()

# Adds an external project, transforming all external "executables" to include
# the runtime properties. On linux for example this will set the rpath to find
# libc++
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

function(android_clang_tidy name)
  if(${name} IN_LIST OPTION_CLANG_TIDY)
    message(STATUS "Tidying ${name}")
    if(OPTION_CLANG_TIDY_FIX)
      message(STATUS " ===> Applying fixes to ${name}")
    endif()
    set_target_properties(${name} PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
  endif()
endfunction()

# ~~~
# Registers the given library as an interface by calculating the source set and setting licensens.
# An INTERFACE library target does not directly create build output, though it may have properties
# set on it and it may be installed, exported and imported.
#
# ``TARGET``  The library/executable target. For example emulatory-libyuv
# ``LIBNAME`` Public library name, this is how it is known in the world. For example libyuv.
# ``URL``     URL where the source code can be found.
# ``REPO``    Internal location where the, where the notice can be found.
# ``LICENSE`` SPDX License identifier.
# ``NOTICE``  Location where the NOTICE can be found
# ~~~
function(android_add_interface)
  _register_target(${ARGN})
  set(options "")
  set(oneValueArgs TARGET)
  set(multiValueArgs "")
  cmake_parse_arguments(build "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})
  add_library(${build_TARGET} INTERFACE)
endfunction()

define_property(
  GLOBAL PROPERTY LICENSES_LST BRIEF_DOCS "Global list of license definitions"
  FULL_DOCS "Global list of license definitions")
define_property(
  GLOBAL PROPERTY INSTALL_TARGETS_LST
  BRIEF_DOCS "GLobal list of targets that are installed."
  FULL_DOCS "GLobal list of targets that are installed.")
set_property(GLOBAL PROPERTY LICENSES_LST " ")
set_property(GLOBAL PROPERTY INSTALL_TARGETS_LST " ")

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

function(android_add_default_test_properties name)
  # Configure the test to run with asan..
  file(READ "${ANDROID_QEMU2_TOP_DIR}/android/asan_overrides" ASAN_OVERRIDES)
  set_property(TEST ${name} PROPERTY ENVIRONMENT
                                     "ASAN_OPTIONS=${ASAN_OVERRIDES}")
  set_property(
    TEST ${name} APPEND
    PROPERTY ENVIRONMENT
             "LLVM_PROFILE_FILE=$<TARGET_FILE_NAME:${name}>.profraw")
  set_property(
    TEST ${name} APPEND
    PROPERTY ENVIRONMENT "ASAN_SYMBOLIZER_PATH=${ANDROID_LLVM_SYMBOLIZER}")
  set_property(TEST ${name} PROPERTY TIMEOUT 600)

  if(WINDOWS_MSVC_X86_64)
    # Let's include the .dll path for our test runner
    string(
      REPLACE
        "/"
        "\\"
        WIN_PATH
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY};${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_mesa;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/qt/lib"
    )
    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
                                              "PATH=${WIN_PATH};$ENV{PATH}")
  endif()
endfunction()

# ~~~
# Adds a test target. It will create and register the test with the given name.
# Test targets are marked as NODISTRIBUTE and hence cannot be installed
#
# ``TARGET``  The library/executable target. For example emulatory-libyuv
# ``SOURCES`` List of source files to be compiled, part of every target.
# ``LINUX``   List of source files to be compiled if the current target is LINUX_X86_64
# ``DARWIN``  List of source files to be compiled if the current target is DARWIN_X86_64
# ``WINDOWS`` List of source files to be compiled if the current target is WINDOWS_MSVC_X86_64
# ``MSVC``    List of source files to be compiled if the current target is  WINDOWS_MSVC_X86_64
# ~~~
function(android_add_test)
  android_add_executable(${ARGN} NODISTRIBUTE)

  set(options "")
  set(oneValueArgs TARGET)
  set(multiValueArgs "")
  cmake_parse_arguments(build "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  add_test(
    NAME ${build_TARGET}
    COMMAND
      $<TARGET_FILE:${build_TARGET}>
      --gtest_output=xml:$<TARGET_FILE_NAME:${build_TARGET}>.xml
      --gtest_catch_exceptions=0
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${build_TARGET}>)

  android_install_as_debug_info(${build_TARGET})
  # Let's not optimize our tests.
  target_compile_options(${build_TARGET} PRIVATE -O0)

  android_add_default_test_properties(${build_TARGET})
endfunction()

# Installs the given target into ${DBG_INFO}/tests/ directory.. This is mainly
# there so we can preserve it as part of our automated builds.
function(android_install_as_debug_info name)
  if(NOT DEFINED DBG_INFO)
    # Ignore when cross-compiling withouth dbg_info available.
    return()
  endif()

  # TODO(jansene): Would be nice if we could make this part of install.
  add_custom_command(
    TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${name}>
            ${DBG_INFO}/tests/$<TARGET_FILE_NAME:${name}>)
endfunction()

# ~~~
# Adds the given executable, by calculating the source set and registering the license
#
# ``SHARED``  Option indicating that this is a shared library.
# ``NODISTRIBUTE`` Option indicating that we will not distribute this.
# ``INSTALL`` Location where this executable should be installed if needed.
# ``TARGET``  The library/executable target. For example emulatory-libyuv
# ``LIBNAME`` Public library name, this is how it is known in the world. For example libyuv.
# ``SOURCES`` List of source files to be compiled, part of every target.
# ``LINUX``   List of source files to be compiled if the current target is LINUX_X86_64
# ``DARWIN``  List of source files to be compiled if the current target is DARWIN_X86_64
# ``WINDOWS`` List of source files to be compiled if the current target is WINDOWS_MSVC_X86_64
# ``MSVC``    List of source files to be compiled if the current target is WINDOWS_MSVC_X86_64
# ``URL``     URL where the source code can be found.
# ``REPO``    Internal location where the notice can be found.
# ``LICENSE`` SPDX License identifier.
# ``NOTICE``  Location where the NOTICE can be found relative to the source URL. Should be written
#             as REPO/path/to/license.
# ~~~
function(android_add_executable)
  _register_target(${ARGN})
  set(options "")
  set(oneValueArgs TARGET INSTALL)
  set(multiValueArgs "")
  cmake_parse_arguments(build "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  add_executable(${build_TARGET} ${REGISTERED_SRC})

  if(WINDOWS_MSVC_X86_64)
    target_link_libraries(${build_TARGET} PRIVATE msvc-posix-compat)
  endif()

  android_clang_tidy(${build_TARGET})
  android_target_dependency(${build_TARGET} all RUNTIME_OS_DEPENDENCIES)
  android_target_properties(${build_TARGET} all "${RUNTIME_OS_PROPERTIES}")

  if(ANDROID_CODE_COVERAGE)
    # TODO Clean out existing .gcda files.
  endif()

  if(DEFINED build_INSTALL)
    android_install_exe(${build_TARGET} ${build_INSTALL})
  endif()
endfunction()

# Adds a protobuf library with the given name. It will export all the needed
# headers, and libraries You can take a dependency on this by adding:
# target_link_libraries(my_target ${name}) for your target. The generated
# library will not use execeptions. Protobuf targets will be licensed under the
# Apache-2.0 license.
#
# name: The name of the generated library. You can take a dependency on this
# with setting target_linke_libraries(my_target ${name})
#
# protofiles: The set of protofiles to be included.
function(android_add_protobuf name protofiles)
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${protofiles})
  android_add_library(TARGET ${name} LICENSE Apache-2.0 SRC ${PROTO_SRCS}
                                                            ${PROTO_HDRS})
  target_link_libraries(${name} PUBLIC libprotobuf)
  target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  # Disable generation of information about every class with virtual functions
  # for use by the C++ runtime type identification features (dynamic_cast and
  # typeid). If you don't use those parts of the language, you can save some
  # space by using this flag. Note that exception handling uses the same
  # information, but it will generate it as needed. The  dynamic_cast operator
  # can still be used for casts that do not require runtime type information,
  # i.e. casts to void * or to unambiguous base classes.
  target_compile_options(${name} PRIVATE -fno-rtti)
  # This needs to be public, as we don't want the headers to start exposing
  # exceptions.
  target_compile_definitions(${name} PUBLIC -DGOOGLE_PROTOBUF_NO_RTTI)
  android_clang_tidy(${name})
endfunction()

# For adding big proto files that mingw can't handle.
function(android_add_big_protobuf name protofiles)
  android_add_protobuf(name protofiles)
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
    ${ANDROID_HW_CONFIG_H}
    PROPERTIES GENERATED TRUE SKIP_AUTOGEN ON HEADER_FILE_ONLY TRUE)
  target_include_directories(android-hw-config
                             PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/avd_config)
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
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/"
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
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/testdata"
    ABSOLUTE)

  add_custom_command(
    TARGET ${TGT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TESTDATA_ROOT}/${SRC_DIR}
            $<TARGET_FILE_DIR:${TGT}>/${DST_DIR})
endfunction()

# Append the given flags to the existing CMAKE_C_FLAGS. Be careful as these
# flags are global and used for every target! Note this will not do anything
# under vs for now
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
        "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR}, out: ${STD_OUT}, err: ${STD_ERR}"
    )
    if(NOT MSVC)
      execute_process(
        COMMAND "date" "+%Y-%m-%d" WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
        RESULT_VARIABLE DATE_RES OUTPUT_VARIABLE STD_OUT ERROR_VARIABLE STD_ERR)
      if(NOT "${DATE_RES}" STREQUAL "0")
        message(FATAL_ERROR "Unable to retrieve date!")
      endif()
    else()
      execute_process(
        COMMAND "date" "/T" WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
        RESULT_VARIABLE DATE_RES OUTPUT_VARIABLE STD_OUT ERROR_VARIABLE STD_ERR)
    endif()
  endif()

  # Clean up and make visibile
  string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
  set(${VER} ${STD_OUT} PARENT_SCOPE)
endfunction()

# VER The variable to set the sha in Sets ${VER} The latest sha as reported by
# git
function(get_git_sha VER)
  execute_process(
    COMMAND "git" "log" "-n" "1" "--pretty=format:'%H'"
    WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR} RESULT_VARIABLE GIT_RES
    OUTPUT_VARIABLE STD_OUT ERROR_VARIABLE STD_ERR)
  if(NOT "${GIT_RES}" STREQUAL "0")
    message(
      FATAL_ERROR
        "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR} : out: ${STD_OUT}, err: ${STD_ERR}"
    )
  endif()

  # Clean up and make visibile
  string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
  set(${VER} ${STD_OUT} PARENT_SCOPE)
endfunction()

# Constructs a linker command that will make sure the whole archive is included,
# not just the ones referenced.
#
# . LIBCMD The variable which will contain the complete linker command . LIBNAME
# The archive that needs to be included completely
function(android_complete_archive LIBCMD LIBNAME)
  if(DARWIN_X86_64)
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
    set(${LIBCMD} "-Wl,-whole-archive" ${LIBNAME} "-Wl,-no-whole-archive"
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
    SRC ${qemu_build_SOURCES} ${qemu-system-${QEMU_AARCH}_sources}
        ${qemu-system-${QEMU_AARCH}_generated_sources})
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
    android_install_exe(${qemu_build_EXE}
                        "./qemu/${ANDROID_TARGET_OS_FLAVOR}-${ANDROID_TARGET_ARCH}")
  endif()
endfunction()

# Constructs the qemu executable.
#
# ANDROID_AARCH The android architecture name STUBS The set of stub sources to
# use.
function(android_add_qemu_executable ANDROID_AARCH STUBS)
  if (WINDOWS_MSVC_X86_64)
    set(WINDOWS_LAUNCHER emulator-winqt-launcher)
  endif()
  android_build_qemu_variant(
    INSTALL
    EXE qemu-system-${ANDROID_AARCH}
    CPU ${ANDROID_AARCH}
    SOURCES android-qemu2-glue/main.cpp vl.c ${STUBS}
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
    SOURCES android-qemu2-glue/main.cpp vl.c ${STUBS}
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
  android_build_qemu_variant(
    # INSTALL We do not install this target.
    EXE qemu-upstream-${ANDROID_AARCH}
    CPU ${ANDROID_AARCH}
    SOURCES vl.c ${STUBS}
    DEFINITIONS -DNEED_CPU_H
    LIBRARIES android-emu
              libqemu2-glue
              libqemu2-glue-vm-operations
              libqemu2-util
              SDL2::SDL2
              android-qemu-deps
              android-qemu-deps-headful
              emulator-libusb)
endfunction()

# Copies a shared library
function(android_copy_shared_lib TGT SHARED_LIB NAME)
  android_copy_file(
    ${TGT} $<TARGET_FILE:${SHARED_LIB}>
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
    get_filename_component(
      DEST "${ANDROID_QEMU2_TOP_DIR}/../../device/generic/goldfish-opengl"
      ABSOLUTE)
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
  set(DEST "${ANDROID_SYMBOL_DIR}/${TGT}.sym")
  set(LOG "${ANDROID_SYMBOL_DIR}/${TGT}.log")
  install(
    CODE # Upload the symbols, with warnings/error logging only.
         "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\"
                             \"${ANDROID_QEMU2_TOP_DIR}/android/build/python/aemu/upload_symbols.py\"
                             \"-v\" \"0\"
                             \"--symbol_file\" \"${DEST}\"
                             \"--environment\" \"${OPTION_CRASHUPLOAD}\"
                             OUTPUT_FILE ${LOG}
                             ERROR_FILE ${LOG})\n
    if (EXISTS ${LOG})
      FILE(READ ${LOG} contents)
      STRING(STRIP \$\{contents\} contents)
    else()
        SET(contents \"No logfile in ${LOG} for ${DEST} was created\")
    endif()
    MESSAGE(STATUS \$\{contents\})")
endfunction()

# Installs the given target executable into the given destinations. Symbols will
# be extracted during build, and uploaded during install.
function(android_install_exe TGT DST)
  install(TARGETS ${TGT} RUNTIME DESTINATION ${DST})

  # Make it available on the build server
  android_extract_symbols(${TGT})
  android_upload_symbols(${TGT})
  android_install_license(${TGT} ${DST}/${TGT}${CMAKE_EXECUTABLE_SUFFIX})
endfunction()

# Installs the given shared library. The shared library will end up in ../lib64
# Symbols will be extracted during build, and uploaded during install.
function(android_install_shared TGT)
  install(
    TARGETS ${TGT} RUNTIME DESTINATION lib64 # We don't want windows to binplace
                                             # dlls in the exe dir
    LIBRARY DESTINATION lib64)
  android_extract_symbols(${TGT})
  android_upload_symbols(${TGT})
  android_install_license(${TGT} ${TGT}${CMAKE_SHARED_LIBRARY_SUFFIX})
endfunction()

# Strips the given prebuilt executable during install..
function(android_strip_prebuilt FNAME)
  # MSVC stores debug info in seperate file, so no need to strip
  if(NOT WINDOWS_MSVC_X86_64)
    install(
      CODE "if(CMAKE_INSTALL_DO_STRIP) \n
                        execute_process(COMMAND ${CMAKE_STRIP} \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${FNAME}\")\n
                      endif()\n
                     ")
  endif()
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

# Rule to build cf crosvm based on a combination of dependencies
# built locally / prebuilt
function(android_crosvm_build DEP)
    message(STATUS "building crosvm with dependency ${DEP}")
    set(CMAKE_CROSVM_HOST_PACKAGE_TOOLS_PATH
        "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/cf-host-package")
    set(CMAKE_CROSVM_BUILD_SCRIPT_PATH
        "${CMAKE_CROSVM_HOST_PACKAGE_TOOLS_PATH}/crosvm-build.sh")

    set(CMAKE_CROSVM_REPO_TOP_LEVEL_PATH
        "${ANDROID_QEMU2_TOP_DIR}/../..")
    set(CMAKE_CROSVM_BUILD_ENV_DIR
        "${CMAKE_BINARY_DIR}/crosvm-build-env")
    set(CMAKE_CROSVM_GFXSTREAM_BUILD_DIR
        "${CMAKE_BINARY_DIR}")
    set(CMAKE_CROSVM_DIST_DIR
        "${CMAKE_INSTALL_PREFIX}")
    add_custom_command(
        OUTPUT "${CMAKE_CROSVM_BUILD_ENV_DIR}/release/crosvm"
        COMMAND
            "${CMAKE_CROSVM_BUILD_SCRIPT_PATH}"
            "${CMAKE_CROSVM_REPO_TOP_LEVEL_PATH}"
            "${CMAKE_CROSVM_HOST_PACKAGE_TOOLS_PATH}"
            "${CMAKE_CROSVM_BUILD_ENV_DIR}"
            "${CMAKE_CROSVM_GFXSTREAM_BUILD_DIR}"
            "${CMAKE_CROSVM_DIST_DIR}"
        DEPENDS ${DEP})
endfunction()
