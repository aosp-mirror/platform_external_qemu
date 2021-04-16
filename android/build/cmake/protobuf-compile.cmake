

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
if(INCLUDE_PROTUBUF_CMAKE)
  return()
endif()

set(INCLUDE_PROTOBUF_CMAKE 1)


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
  android_add_library(TARGET ${name} LICENSE Apache-2.0 SRC ${PROTO_HDRS}
                                                            ${PROTO_SRCS})
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

function(protobuf_generate_with_plugin)
  include(CMakeParseArguments)
  set(_options APPEND_PATH)
  set(_singleargs
      LANGUAGE
      OUT_VAR
      EXPORT_MACRO
      PROTOC_OUT_DIR
      PLUGIN
      PLUGINOUT
      PROTOPATH
      HEADERFILEEXTENSION)
  if(COMMAND target_sources)
    list(APPEND _singleargs TARGET)
  endif()
  set(_multiargs PROTOS IMPORT_DIRS GENERATE_EXTENSIONS)

  cmake_parse_arguments(protobuf_generate_with_plugin "${_options}"
                        "${_singleargs}" "${_multiargs}" "${ARGN}")

  if(NOT protobuf_generate_with_plugin_PROTOS
     AND NOT protobuf_generate_with_plugin_TARGET)
    message(
      SEND_ERROR
        "Error: protobuf_generate_with_plugin called without any targets or source files"
    )
    return()
  endif()

  if(protobuf_generate_with_plugin_PLUGIN)
    set(protobuf_generate_with_plugin_PLUGIN
        "--plugin=${protobuf_generate_with_plugin_PLUGIN}")
  endif()

  if(protobuf_generate_with_plugin_PLUGINOUT)
    set(protobuf_generate_with_plugin_PLUGINOUT
        "--plugin_out=${protobuf_generate_with_plugin_PLUGINOUT}")
  endif()
  if(NOT protobuf_generate_with_plugin_OUT_VAR
     AND NOT protobuf_generate_with_plugin_TARGET)
    message(
      SEND_ERROR
        "Error: protobuf_generate called without a target or output variable")
    return()
  endif()

  if(NOT protobuf_generate_with_plugin_LANGUAGE)
    set(protobuf_generate_with_plugin_LANGUAGE cpp)
  endif()
  string(TOLOWER ${protobuf_generate_with_plugin_LANGUAGE}
                 protobuf_generate_with_plugin_LANGUAGE)

  if(NOT protobuf_generate_with_plugin_PROTOC_OUT_DIR)
    set(protobuf_generate_with_plugin_PROTOC_OUT_DIR
        ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  if(protobuf_generate_with_plugin_EXPORT_MACRO
     AND protobuf_generate_with_plugin_LANGUAGE STREQUAL cpp)
    set(_dll_export_decl
        "dllexport_decl=${protobuf_generate_with_plugin_EXPORT_MACRO}:")
  endif()

  if(NOT protobuf_generate_with_plugin_GENERATE_EXTENSIONS)
    if(protobuf_generate_with_plugin_LANGUAGE STREQUAL cpp)
      set(protobuf_generate_with_plugin_GENERATE_EXTENSIONS
          ${HEADERFILEEXTENSION} .pb.cc)
    elseif(protobuf_generate_with_plugin_LANGUAGE STREQUAL python)
      set(protobuf_generate_with_plugin_GENERATE_EXTENSIONS _pb2.py)
    else()
      message(
        SEND_ERROR
          "Error: protobuf_generatewith_plugin given unknown Language ${LANGUAGE}, please provide a value for GENERATE_EXTENSIONS"
      )
      return()
    endif()
  endif()

  if(protobuf_generate_with_plugin_TARGET)
    get_target_property(_source_list ${protobuf_generate_with_plugin_TARGET}
                        SOURCES)
    foreach(_file ${_source_list})
      if(_file MATCHES "proto$")
        list(APPEND protobuf_generate_with_plugin_PROTOS ${_file})
      endif()
    endforeach()
  endif()

  if(NOT protobuf_generate_with_plugin_PROTOS)
    message(
      SEND_ERROR
        "Error: protobuf_generate_with_plugin could not find any .proto files")
    return()
  endif()

  if(protobuf_generate_with_plugin_APPEND_PATH)
    # Create an include path for each file specified
    foreach(_file ${protobuf_generate_with_plugin_PROTOS})
      get_filename_component(_abs_file ${_file} ABSOLUTE)
      get_filename_component(_abs_path ${_abs_file} PATH)
      list(FIND _protobuf_include_path ${_abs_path} _contains_already)
      if(${_contains_already} EQUAL -1)
        list(APPEND _protobuf_include_path -I ${_abs_path})
      endif()
    endforeach()
  else()
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  foreach(DIR ${protobuf_generate_with_plugin_IMPORT_DIRS})
    get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
    list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
    if(${_contains_already} EQUAL -1)
      list(APPEND _protobuf_include_path -I ${ABS_PATH})
    endif()
  endforeach()

  separate_arguments(PROTOPATH)

  set(_generated_srcs_all)
  foreach(_proto ${protobuf_generate_with_plugin_PROTOS})
    get_filename_component(_abs_file ${_proto} ABSOLUTE)
    get_filename_component(_abs_dir ${_abs_file} DIRECTORY)
    get_filename_component(_basename ${_proto} NAME_WE)
    file(RELATIVE_PATH _rel_dir ${CMAKE_CURRENT_SOURCE_DIR} ${_abs_dir})

    set(_generated_srcs)
    foreach(_ext ${protobuf_generate_with_plugin_GENERATE_EXTENSIONS})
      list(
        APPEND _generated_srcs
        "${protobuf_generate_with_plugin_PROTOC_OUT_DIR}/${_basename}${_ext}")
    endforeach()
    list(APPEND _generated_srcs_all ${_generated_srcs})

    add_custom_command(
      OUTPUT ${_generated_srcs}
      COMMAND
        protobuf::protoc ARGS --${protobuf_generate_with_plugin_LANGUAGE}_out
        ${_dll_export_decl}${protobuf_generate_with_plugin_PROTOC_OUT_DIR}
        ${_protobuf_include_path} ${PROTOPATH} ${_abs_file}
        ${protobuf_generate_with_plugin_PLUGIN}
        ${protobuf_generate_with_plugin_PLUGINOUT}
      DEPENDS ${_abs_file} protobuf::protoc
      COMMENT
        "Running ${protobuf_generate_with_plugin_LANGUAGE} protocol buffer compiler on ${_proto}"
      VERBATIM)
  endforeach()

  set_source_files_properties(${_generated_srcs_all} PROPERTIES GENERATED TRUE)
  if(protobuf_generate_with_plugin_OUT_VAR)
    set(${protobuf_generate_with_plugin_OUT_VAR} ${_generated_srcs_all}
        PARENT_SCOPE)
  endif()
  if(protobuf_generate_with_plugin_TARGET)
    target_sources(${protobuf_generate_with_plugin_TARGET}
                   PRIVATE ${_generated_srcs_all})
    target_compile_options(${protobuf_generate_with_plugin_TARGET}
                           PRIVATE -fno-rtti)
    target_compile_definitions(${protobuf_generate_with_plugin_TARGET}
                               PUBLIC -DGOOGLE_PROTOBUF_NO_RTTI)
  endif()

endfunction()

function(
  PROTOBUF_GENERATE_CPP_WITH_PLUGIN
  HEADERFILEEXTENSION
  PLUGIN
  PLUGINOUT
  PROTOPATH
  SRCS
  HDRS)
  cmake_parse_arguments(protobuf_generate_cpp_with_plugin "" "EXPORT_MACRO" ""
                        ${ARGN})

  set(_proto_files "${protobuf_generate_cpp_with_plugin_UNPARSED_ARGUMENTS}")
  if(NOT _proto_files)
    message(
      SEND_ERROR
        "Error: PROTOBUF_GENERATE_CPP_WITH_PLUGIN() called without any proto files"
    )
    return()
  endif()

  if(PROTOBUF_GENERATE_CPP_WITH_PLUGIN_APPEND_PATH)
    set(_append_arg APPEND_PATH)
  endif()

  if(DEFINED Protobuf_IMPORT_DIRS)
    set(_import_arg IMPORT_DIRS ${Protobuf_IMPORT_DIRS})
  endif()

  set(_outvar)
  protobuf_generate_with_plugin(
    ${_append_arg}
    LANGUAGE cpp
    EXPORT_MACRO ${protobuf_generate_cpp_with_plugin_EXPORT_MACRO}
    OUT_VAR _outvar ${_import_arg}
    PROTOS ${_proto_files}
    PLUGIN ${PLUGIN}
    PLUGINOUT ${PLUGINOUT}
    PROTOPATH ${PROTOPATH}
    HEADERFILEEXTENSION ${HEADERFILEEXTENSION})

  set(${SRCS})
  set(${HDRS})
  foreach(_file ${_outvar})
    if(_file MATCHES "cc$")
      list(APPEND ${SRCS} ${_file})
    else()
      list(APPEND ${HDRS} ${_file})
    endif()
  endforeach()
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

# Adds a protozero library with the given plugin name.
function(
  android_add_protobuf_with_plugin
  name
  headerfileextension
  plugin
  pluginout
  protolib
  PROTOPATH
  protofile
  genccpath)
  protobuf_generate_cpp_with_plugin(
    ${headerfileextension}
    ${plugin}
    ${pluginout}
    ${PROTOPATH}
    PROTO_SRCS
    PROTO_HDRS
    ${protofile})
  get_filename_component(PROTO_SRCS_ABS ${PROTO_SRCS} ABSOLUTE)
  set(genccpath2 ${CMAKE_CURRENT_BINARY_DIR}/${genccpath})
  set_source_files_properties(${genccpath2} PROPERTIES GENERATED TRUE)
  android_add_library(TARGET ${name} LICENSE Apache-2.0 SRC ${genccpath2}
                                                            ${PROTO_HDRS})
  target_link_libraries(${name} PUBLIC ${protolib})
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
