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

set(ANDROID_XCODE_SIGN_ADHOC FALSE)

if (APPLE)
    set(ANDROID_XCODE_SIGN_ADHOC TRUE)
endif()

# Checks to make sure the TAG is valid.
function(_check_target_tag TAG)
  set(VALID_TARGETS
      windows
      windows_msvc-x86_64
      linux-x86_64
      linux-aarch64
      darwin-x86_64
      darwin-aarch64
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
          "-DCMAKE_TOOLCHAIN_FILE=${ANDROID_CMAKE_TOOLCHAIN_DIR}/toolchain-${ANDROID_HOST_TAG}.cmake"
        BUILD_BYPRODUCTS ${BUILD_PRODUCT}
        TEST_BEFORE_INSTALL True
        INSTALL_COMMAND "")
    endif()
    set(${OUT_PATH} ${BUILD_PRODUCT} PARENT_SCOPE)
  endif()
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
# Prefix the given prefix path to a source file that is not absolute sets.
#
# ``OUT``     The variable containing the transformed paths.
# ``PREFIX``  The relative prefix path.
# ``SRC    `` List of source files to be prefixed
# ~~~
function(prefix_sources)
  set(oneValueArgs OUT PREFIX)
  set(multiValueArgs SRC)
  cmake_parse_arguments(pre "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})
  message("Prefix: ${pre_PREFIX} -> ${pre_OUT}")
  if(NOT "${pre_PREFIX}" STREQUAL "")
    set(ABSOLUTE_SOURCES "")
    foreach(FIL ${pre_SRC})
      if(IS_ABSOLUTE ${FIL})
        list(APPEND ABSOLUTE_SOURCES "${FIL}")
      else()
        list(APPEND ABSOLUTE_SOURCES "${pre_PREFIX}/${FIL}")
      endif()
    endforeach()
    set(${pre_OUT} ${ABSOLUTE_SOURCES} PARENT_SCOPE)
  else()
    set(${pre_OUT} ${pre_SRC} PARENT_SCOPE)
  endif()
endfunction()

# ~~~
# ~~~
# Registers the given target, by calculating the source set and setting
# licensens.
#
# ``TARGET``  The library/executable target. For example emulatory-libyuv
# ``LIBNAME`` Public library name, this is how it is known in the world. For
# example libyuv. ``SOURCES`` List of source files to be compiled, part of every
# target. ``LINUX``   List of source files to be compiled if the current target
# is LINUX_X86_64 ``DARWIN``  List of source files to be compiled if the current
# target is DARWIN_X86_64 ``WINDOWS`` List of source files to be compiled if the
# current target is WINDOWS_MSVC_X86_64 ``MSVC``    List of source files to be
# compiled if the current target is WINDOWS_MSVC_X86_64 ``URL``     URL where
# the source code can be found. ``REPO``    Internal location where the, where
# the notice can be found. ``LICENSE`` SPDX License identifier. ``NOTICE``
# Location where the NOTICE can be found ``SOURCE_DIR`` Root source directory.
# This will be prepended to every source
# ~~~
function(_register_target)
  set(options NODISTRIBUTE)
  set(oneValueArgs
      TARGET
      LICENSE
      LIBNAME
      REPO
      URL
      NOTICE
      SOURCE_DIR)
  set(multiValueArgs SRC LINUX MSVC WINDOWS DARWIN POSIX)
  cmake_parse_arguments(build "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})
  if(NOT DEFINED build_TARGET)
    message(FATAL_ERROR "Undefined target for library.")
  endif()

  set(src ${build_SRC})
  if(LINUX AND (build_LINUX OR build_POSIX))
    list(APPEND src ${build_LINUX} ${build_POSIX})
  elseif((DARWIN_X86_64 OR DARWIN_AARCH64) AND (build_DARWIN OR build_POSIX))
    list(APPEND src ${build_DARWIN} ${build_POSIX})
  elseif(WINDOWS_MSVC_X86_64 AND (build_MSVC OR build_WINDOWS))
    list(APPEND src ${build_MSVC} ${build_WINDOWS})
  endif()

  if(build_SOURCE_DIR)
    set(ABSOLUTE_SOURCES "")
    foreach(FIL ${src})
      if(IS_ABSOLUTE ${FIL})
        list(APPEND ABSOLUTE_SOURCES "${FIL}")
      else()
        list(APPEND ABSOLUTE_SOURCES "${build_SOURCE_DIR}/${FIL}")
      endif()
    endforeach()
    set(REGISTERED_SRC ${ABSOLUTE_SOURCES} PARENT_SCOPE)
  else()
    set(REGISTERED_SRC ${src} PARENT_SCOPE)
  endif()

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

function(android_sign path)
    if (ANDROID_XCODE_SIGN_ADHOC)
        install(
            CODE "message(\"android_sign ${path}\")\nexecute_process(COMMAND codesign --deep -s - --entitlements ${ANDROID_QEMU2_TOP_DIR}/entitlements.plist ${path})")
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
  # Clang on mac os does not get properly recognized by cmake
  if(NOT DARWIN_X86_64)
    target_compile_features(${build_TARGET} PRIVATE cxx_std_17)
  endif()

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
        "${CMAKE_LIBRARY_OUTPUT_DIRECTORY};${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gles_swiftshader;${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/qt/lib;$ENV{PATH}"
    )
    string(REPLACE ";" "\;" WIN_PATH "${WIN_PATH}")
    set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT "PATH=${WIN_PATH}")
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
  # Clang on mac os does not get properly recognized by cmake
  if(NOT DARWIN_X86_64)
    target_compile_features(${build_TARGET} PRIVATE cxx_std_17)
  endif()

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
  get_filename_component(
    DEST "${AOSP_ROOT}/device/generic/goldfish-opengl"
    ABSOLUTE)
  android_validate_sha(
    FILE ${FILE}
    EXPECTED ${EXPECTED}
    ERROR
      "You need to regenerate the cmake files by executing 'make' in ${DEST}")
endfunction()

function(android_validate_sha)
  include(CMakeParseArguments)
  set(_options)
  set(_singleargs FILE EXPECTED ERROR)
  set(_multiargs)

  cmake_parse_arguments(sha "${_options}" "${_singleargs}" "${_multiargs}"
                        "${ARGN}")

  file(SHA256 ${sha_FILE} CHECKSUM)
  if(NOT CHECKSUM STREQUAL "${sha_EXPECTED}")
    message(
      FATAL_ERROR
        "Checksum mismatch for sha256(${sha_FILE})=${CHECKSUM}, was expecting: ${sha_EXPECTED}. ${sha_ERROR}"
    )
  endif()
endfunction()


# Installs the given target executable into the given destinations. Symbols will
# be extracted during build, and uploaded during install.
function(android_install_exe TGT DST)
  install(TARGETS ${TGT} RUNTIME DESTINATION ${DST})

  # Make it available on the build server
  android_extract_symbols(${TGT})
  android_upload_symbols(${TGT})
  android_install_license(${TGT} ${DST}/${TGT}${CMAKE_EXECUTABLE_SUFFIX})
  android_sign(${CMAKE_INSTALL_PREFIX}/${DST}/${TGT}${CMAKE_EXECUTABLE_SUFFIX})
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
  # Account for lib prefix when signing
  android_sign(
    ${CMAKE_INSTALL_PREFIX}/lib64/lib${TGT}${CMAKE_SHARED_LIBRARY_SUFFIX})
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
