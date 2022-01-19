if (NOT DEFINED TINK_GENFILE_DIR)
  set(TINK_GENFILE_DIR "${CMAKE_CURRENT_BINARY_DIR}/__generated")
endif()

set(TINK_DIR ${ANDROID_QEMU2_TOP_DIR}/../tink)
set(RAPID_JSON_DIR ${ANDROID_QEMU2_TOP_DIR}/../rapidjson)
set(TINK_BUILD_TESTS TRUE)
include(${TINK_DIR}/tink_version.cmake)
include(${TINK_DIR}/cmake/TinkUtil.cmake)

set(TINK_CXX_STANDARD 17)
list(APPEND TINK_INCLUDE_DIRS "${TINK_GENFILE_DIR}")
set(TINK_IDE_FOLDER "Tink")

add_library(protobuf::libprotobuf-lite ALIAS libprotobuf_lite)
add_library(protobuf::libprotobuf ALIAS libprotobuf)

# Rapidjson is a header-only library with no explicit target. Here we create one.
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE "${RAPID_JSON_DIR}")

# Declare the beginning of a new Tink library namespace.
#
# As a rule of thumb, every CMakeLists.txt should be a different module, named
# after the directory that contains it, and this function should appear at the
# top of each CMakeLists script.
#
# This is not a requirement, though. Targets should be grouped logically, and
# multiple directories can be part of the same module as long as target names
# do not collide.
#
macro(tink_module NAME)
  set(TINK_MODULE ${NAME})
endmacro()

# Declare a Tink library. Produces a static library that can be linked into
# other test, binary or library targets. Tink libraries are mainly meant as
# a way to organise code and speed up compilation.
#
# Arguments:
#   NAME base name of the target. See below for target naming conventions.
#   SRCS list of source files, including headers.
#   DEPS list of dependency targets.
#   PUBLIC flag, signal that this target is intended for external use.
#
# If SRCS contains only headers, an INTERFACE rule is created. This rule carries
# include path and link library information, but is not directly buildable.
#
# The corresponding build target is named tink_<MODULE>_<NAME> if PUBLIC is
# specified, or tink_internal_<MODULE>_<NAME> otherwise. An alias is also
# defined for use in CMake scripts, in the tink::<MODULE>::<NAME> form.
#
# Unlike Bazel, CMake does not enforce the rule that all dependencies must be
# listed. CMake DEPS just carry include, build and link flags that are passed
# to the compiler. Because of this, a target might compile even if a dependency
# is not specified, but that could break at any time. So make sure that all
# dependencies are explicitly specified.
#
function(tink_cc_library)
  cmake_parse_arguments(PARSE_ARGV 0 tink_cc_library
    "PUBLIC"
    "NAME"
    "SRCS;DEPS"
  )

  if (NOT DEFINED TINK_MODULE)
    message(FATAL_ERROR
            "TINK_MODULE not defined, perhaps you are missing a tink_module() statement?")
  endif()

  # We replace :: with __ in targets, because :: may not appear in target names.
  # However, the module name should still span multiple name spaces.
  STRING(REPLACE "::" "__" _ESCAPED_TINK_MODULE ${TINK_MODULE})

  set(_is_headers_only_lib true)
  foreach(_src_file ${tink_cc_library_SRCS})
    if(${_src_file} MATCHES "\\.cc$")
      set(_is_headers_only_lib false)
      break()
    endif()
  endforeach()

  if (tink_cc_library_PUBLIC)
    set(_target_name "tink_${_ESCAPED_TINK_MODULE}_${tink_cc_library_NAME}")
  else()
    set(_target_name "tink_internal_${_ESCAPED_TINK_MODULE}_${tink_cc_library_NAME}")
  endif()

  if(NOT _is_headers_only_lib)
    # Workaround for bug b/215443895, remove once it is fixed and merged 
    if(WINDOWS_MSVC_X86_64)
      if ("${_target_name}" STREQUAL "tink_internal_internal_util")
       message(STATUS "Applying TINK patch for util.cc (see b/215443895)")
       list(REMOVE_ITEM tink_cc_library_SRCS util.cc)
       list(APPEND tink_cc_library_SRCS ${ANDROID_QEMU2_TOP_DIR}/android/third_party/tink/util_win_workaround.cc)
      endif()
      if ("${_target_name}" STREQUAL "tink_internal_util_test_util")
        message(STATUS "Applying TINK patch for test_util.cc (see b/215443895)")
        list(REMOVE_ITEM tink_cc_library_SRCS test_util.cc)
        list(APPEND tink_cc_library_SRCS ${ANDROID_QEMU2_TOP_DIR}/android/third_party/tink/test_util_win_workaround.cc)
      endif()
    endif()
    add_library(${_target_name} STATIC "")
    target_sources(${_target_name} PRIVATE ${tink_cc_library_SRCS})
    target_include_directories(${_target_name} PUBLIC ${TINK_INCLUDE_DIRS})
    target_link_libraries(${_target_name} PUBLIC ${tink_cc_library_DEPS})
    set_property(TARGET ${_target_name} PROPERTY CXX_STANDARD ${TINK_CXX_STANDARD})
    set_property(TARGET ${_target_name} PROPERTY CXX_STANDARD_REQUIRED true)
    if(WINDOWS_MSVC_X86_64)
         target_link_libraries(${_target_name} PRIVATE msvc-posix-compat)
    endif()
    if (tink_cc_library_PUBLIC)
      set_property(TARGET ${_target_name}
                   PROPERTY FOLDER "${TINK_IDE_FOLDER}")
    else()
      set_property(TARGET ${_target_name}
                   PROPERTY FOLDER "${TINK_IDE_FOLDER}/Internal")
    endif()
  else()
    add_library(${_target_name} INTERFACE)
    target_include_directories(${_target_name} INTERFACE ${TINK_INCLUDE_DIRS})
    target_link_libraries(${_target_name} INTERFACE ${tink_cc_library_DEPS})
  endif()

  add_library(
    tink::${TINK_MODULE}::${tink_cc_library_NAME} ALIAS ${_target_name})

    android_license(
        TARGET ${_target_name}
        URL https://android.googlesource.com/platform/external/tink/+/refs/heads/upstream-master
        LIBNAME Tink
        SPDX Apache-2.0
        LICENSE https://android.googlesource.com/platform/external/tink/+/refs/heads/upstream-master/LICENSE
        LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.APACHE2")
endfunction(tink_cc_library)

# set(TINK_DISABLED_TESTS "tink_test_util_file_output_stream_test;tink_test_util_ostream_output_stream_test")
if(WINDOWS_MSVC_X86_64)
  # Does not compile due to alignment error
  list(APPEND TINK_DISABLED_TESTS tink_test_util_secret_data_test) 
  # Does not work due to regex parsing
  list(APPEND TINK_DISABLED_TESTS tink_test_core_version_test)
endif()

# Declare a Tink test using googletest, with a syntax similar to Bazel.
#
# Parameters:
#   NAME base name of the test.
#   SRCS list of test source files, headers included.
#   DEPS list of dependencies, see tink_cc_library above.
#   DATA list of non-code dependencies, such as test vectors.
#
# Tests added with this macro are automatically registered.
# Each test produces a build target named tink_test_<MODULE>_<NAME>.
#
function(tink_cc_test)
  cmake_parse_arguments(PARSE_ARGV 0 tink_cc_test
    ""
    "NAME"
    "SRCS;DEPS;DATA"
  )

  if (NOT TINK_BUILD_TESTS)
    return()
  endif()

  if (NOT DEFINED TINK_MODULE)
    message(FATAL_ERROR "TINK_MODULE not defined")
  endif()

  # We replace :: with __ in targets, because :: may not appear in target names.
  # However, the module name should still span multiple name spaces.
  STRING(REPLACE "::" "__" _ESCAPED_TINK_MODULE ${TINK_MODULE})

  set(_target_name "tink_test_${_ESCAPED_TINK_MODULE}_${tink_cc_test_NAME}")


  # ignore disabled tests.
  if ("${_target_name}" IN_LIST TINK_DISABLED_TESTS)
    return()
  endif()

  android_add_test(TARGET ${_target_name}
    SRC ${tink_cc_test_SRCS}
  )
  target_compile_options(${_target_name} PRIVATE -frtti)
  target_link_libraries(${_target_name} PRIVATE
    gtest_main
    ${tink_cc_test_DEPS}
    Threads::Threads
  )

  set_property(TARGET ${_target_name}
               PROPERTY FOLDER "${TINK_IDE_FOLDER}/Tests")
               
  # Some tests appear to read/write to the same file causing issues.
  set_tests_properties("${_target_name}" PROPERTIES RESOURCE_LOCK TINK)
  set_property(TARGET ${_target_name} PROPERTY CXX_STANDARD ${TINK_CXX_STANDARD})
  set_property(TARGET ${_target_name} PROPERTY CXX_STANDARD_REQUIRED true)
endfunction(tink_cc_test)

# Declare a C++ Proto library.
#
# Parameters:
#   NAME base name of the library.
#   SRCS list of .proto source files.
#   DEPS list of proto library dependencies, produced by tink_cc_proto or not.
#
# The resulting library follows the same naming convention as tink_cc_library.
#
function(tink_cc_proto)
  cmake_parse_arguments(PARSE_ARGV 0 tink_cc_proto
    ""
    "NAME"
    "SRCS;DEPS"
  )

  get_filename_component(TINK_ABS_PATH "${TINK_DIR}" ABSOLUTE)
  set(tink_cc_proto_GEN_SRCS)
  foreach(_src_path ${tink_cc_proto_SRCS})
    get_filename_component(_src_absolute_path "${_src_path}" ABSOLUTE)
    get_filename_component(_src_basename "${_src_path}" NAME_WE)
    get_filename_component(_src_dir "${_src_absolute_path}" DIRECTORY)
    file(RELATIVE_PATH _src_rel_path "${TINK_DIR}" "${_src_dir}")

    set(_gen_srcs)
    foreach(_gen_ext .pb.h .pb.cc)
      list(APPEND _gen_srcs
           "${TINK_GENFILE_DIR}/${_src_rel_path}/${_src_basename}${_gen_ext}")
    endforeach()

    list(APPEND tink_cc_proto_GEN_SRCS ${_gen_srcs})

    add_custom_command(
      COMMAND protobuf::protoc
      ARGS
        --cpp_out "${TINK_GENFILE_DIR}"
        -I "${TINK_ABS_PATH}"
        "${_src_absolute_path}"
      OUTPUT
        ${_gen_srcs}
      DEPENDS
        protobuf::protoc
        ${_src_absolute_path}
      COMMENT "Running CXX protocol buffer compiler on ${_src_path}"
      VERBATIM
    )
  endforeach()

  set_source_files_properties(
    ${tink_cc_proto_GEN_SRCS} PROPERTIES GENERATED true)

  tink_cc_library(
    NAME ${tink_cc_proto_NAME}
    SRCS ${tink_cc_proto_GEN_SRCS}
    DEPS
      protobuf::libprotobuf
      ${tink_cc_proto_DEPS}
  )
endfunction()

# Declare an empty target, that depends on all those specified. Use this rule
# to group dependencies that are logically related and give them a single name.
#
# Parameters:
#   NAME base name of the target.
#   DEPS list of dependencies to group.
#
# Each tink_target_group produces a target named tink_<MODULE>_<NAME>.
function(tink_target_group)
  cmake_parse_arguments(PARSE_ARGV 0 tink_target_group "" "NAME" "DEPS")
  set(_target_name "tink_${TINK_MODULE}_${tink_target_group_NAME}")
  add_custom_target(${_target_name})
  add_dependencies(${_target_name} ${tink_target_group_DEPS})
endfunction()


set(TINK_INCLUDE_ALIAS_DIR "${CMAKE_CURRENT_BINARY_DIR}/__include_alias")
add_directory_alias(
  "${TINK_DIR}/cc" "${TINK_INCLUDE_ALIAS_DIR}/tink")
list(APPEND TINK_INCLUDE_DIRS "${TINK_INCLUDE_ALIAS_DIR}")

add_subdirectory(${TINK_DIR}/cc TINK_cc)
add_subdirectory(${TINK_DIR}/proto proto)

# Make sure we can find the protobuf includes.
add_directory_alias(
  "${CMAKE_CURRENT_BINARY_DIR}" "${TINK_INCLUDE_ALIAS_DIR}/proto")
add_directory_alias("${CMAKE_CURRENT_LIST_DIR}" "external")