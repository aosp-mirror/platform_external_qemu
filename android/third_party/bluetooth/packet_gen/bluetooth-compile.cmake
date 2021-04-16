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

# ~~~
# Enable the compilation of pdl files using packet_gen. This relies on
# syste/b being available
#
# The following parameters are accepted
#
# ``GENERATED`` The set of generated sources
# ``SRC`` List of source files to be compiled.
# ``INCLUDES`` Include directory.
# ``NAMESPACE`` Root name space to use
# ``OUTPUT_DIR``
#      Directory where the generated sources will be placed, defaults to CMAKE_CURRENT_BINARY_DIR/gens
#  ``SOURCE_DIR``
#      Root directory where sources can be found, defaults to CMAKE_CURRENT_SOURCE_DIR
# ~~~
function(android_bluetooth_packet_gen)
  # Parse arguments
  set(options)
  set(oneValueArgs OUTPUT_DIR GENERATED SOURCE_DIR INCLUDES NAMESPACE)
  set(multiValueArgs SRC)
  cmake_parse_arguments(packet_gen "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  if(NOT packet_gen_OUTPUT_DIR)
    set(packet_gen_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/packet_gen)
  endif()
  if(packet_gen_NAMESPACE)
    set(packet_gen_NAMESPACE "--root_namespace=${packet_gen_NAMESPACE}")
  endif()
  if(NOT packet_gen_SOURCE_DIR)
    set(packet_gen_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  if(NOT packet_gen_SRC)
    message(
      SEND_ERROR
        "Error: android_packet_gen_compile() called without any .yy files")
    return()
  endif()

  # Configure packet_gen
  android_compile_for_host(
    bluetooth_packetgen
    ${ANDROID_QEMU2_TOP_DIR}/android/third_party/bluetooth/packet_gen
    bluetooth_packetgen_EXECUTABLE)

  set(BLUE_GEN "")
  file(MAKE_DIRECTORY ${packet_gen_OUTPUT_DIR})
  foreach(FIL ${packet_gen_SRC})
    get_filename_component(
      ABS_FIL ${packet_gen_SOURCE_DIR}/${packet_gen_INCLUDES}/${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(FIL_DIR ${FIL} DIRECTORY)
    set(FIL_GEN "${packet_gen_OUTPUT_DIR}/${FIL_DIR}/${FIL_WE}.h")
    add_custom_command(
      OUTPUT "${FIL_GEN}"
      COMMAND
        ${bluetooth_packetgen_EXECUTABLE} ${packet_gen_NAMESPACE}
        "--include=${packet_gen_INCLUDES}" "--out=${packet_gen_OUTPUT_DIR}"
        ${packet_gen_INCLUDES}/${FIL}
      COMMENT "Creating bluetooth packet headers from ${ABS_FIL}"
      WORKING_DIRECTORY ${packet_gen_SOURCE_DIR}
      VERBATIM
      DEPENDS ${bluetooth_packetgen_EXECUTABLE} ${ABS_FIL})
    list(APPEND BLUE_GEN ${FIL_GEN})
    set_source_files_properties(${FIL_GEN} PROPERTIES GENERATED TRUE)
  endforeach()

  # Make the library available
  if(packet_gen_GENERATED)
    set(${packet_gen_GENERATED} "${BLUE_GEN}" PARENT_SCOPE)
  endif()
endfunction()

# ~~~
# Enable the compilation of .l / .ll files using flex. This relies on
# ./external/flex being available
#
# The following parameters are accepted
#
# ``GENERATED`` The set of generated files
# ``SRC`` List of source files to be compiled.
# ``OUTPUT_DIR``
#      Directory where the generated sources will be placed, defaults to CMAKE_CURRENT_BINARY_DIR/flex
#  ``SOURCE_DIR``
#      Root directory where sources can be found, defaults to CMAKE_CURRENT_SOURCE_DIR
# ~~~
function(android_flex_compile)
  # Parse arguments
  set(options)
  set(oneValueArgs OUTPUT_DIR GENERATED SOURCE_DIR)
  set(multiValueArgs SRC)
  cmake_parse_arguments(flex "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  if(NOT flex_OUTPUT_DIR)
    set(flex_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/flex)
  endif()
  if(NOT flex_SOURCE_DIR)
    set(flex_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  if(NOT flex_SRC)
    message(
      SEND_ERROR "Error: android_flex_compile() called without any .l/.ll files"
    )
    return()
  endif()

  # Configure flex
  android_compile_for_host(
    flex ${ANDROID_QEMU2_TOP_DIR}/android/third_party/flex flex_EXECUTABLE)

  set(FLEX_GEN "")
  file(MAKE_DIRECTORY ${flex_OUTPUT_DIR})
  foreach(FIL ${flex_SRC})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    file(RELATIVE_PATH REL_FIL ${flex_SOURCE_DIR} ${ABS_FIL})
    get_filename_component(REL_DIR ${REL_FIL} DIRECTORY)
    set(RELFIL_WE "${REL_DIR}/${FIL_WE}")
    add_custom_command(
      OUTPUT "${flex_OUTPUT_DIR}/${RELFIL_WE}.cpp"
      COMMAND ${flex_EXECUTABLE} -o ${flex_OUTPUT_DIR}/${RELFIL_WE}.cpp
              ${ABS_FIL}
      COMMENT "Lexing ${ABS_FIL} with flex"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
      DEPENDS ${flex_EXECUTABLE} ${ABS_FIL})
    list(APPEND FLEX_GEN ${flex_OUTPUT_DIR}/${RELFIL_WE}.cpp)
    set_source_files_properties(${flex_OUTPUT_DIR}/${RELFIL_WE}.cpp
                                PROPERTIES GENERATED TRUE)
  endforeach()

  # Make the library available
  if(flex_GENERATED)
    set(${flex_GENERATED} "${FLEX_GEN}" PARENT_SCOPE)
  endif()
endfunction()

# ~~~
# Enable the compilation of .yy files using bison. This relies on
# ./external/bison
#
# The following parameters are accepted
#
# ''TYPE`` Generate c or cpp, defaults to cpp
# ``TARGET`` The library target to generate.
# ``SRC`` List of source files to be compiled.
# ``OUTPUT_DIR``
#      Directory where the generated sources will be placed, defaults to CMAKE_CURRENT_BINARY_DIR/gens
#  ``SOURCE_DIR``
#      Root directory where sources can be found, defaults to CMAKE_CURRENT_SOURCE_DIR
# ~~~
function(android_yacc_compile)
  # Parse arguments
  set(options)
  set(oneValueArgs OUTPUT_DIR GENERATED SOURCE_DIR EXT)
  set(multiValueArgs SRC)
  cmake_parse_arguments(yacc "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  if(NOT yacc_EXT)
    set(yacc_EXT "cpp")
  endif()
  if(NOT yacc_OUTPUT_DIR)
    set(yacc_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/yacc)
  endif()
  if(NOT yacc_SOURCE_DIR)
    set(yacc_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  if(NOT yacc_SRC)
    message(
      SEND_ERROR "Error: android_yacc_compile() called without any .yy files")
    return()
  endif()

  set(BISON_DIR ${ANDROID_QEMU2_TOP_DIR}/../bison)
  # Configure bison & m4
  android_compile_for_host(
    bison ${ANDROID_QEMU2_TOP_DIR}/android/third_party/bison bison_EXECUTABLE)
  android_compile_for_host(m4 ${ANDROID_QEMU2_TOP_DIR}/android/third_party/m4
                           m4_EXECUTABLE)

  if(MSVC AND WINDOWS_MSVC_X86_64)
    set(PKG_DIR "set BISON_PKGDATADIR=${BISON_DIR}/data &")
  else()
    set(PKG_DIR "BISON_PKGDATADIR=${BISON_DIR}/data")
    set(M4 "M4=${m4_EXECUTABLE}")
  endif()

  set(YACC_GEN "")
  file(MAKE_DIRECTORY ${yacc_OUTPUT_DIR})
  foreach(FIL ${yacc_SRC})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(FIL_EX ${FIL} EXT)
    file(RELATIVE_PATH REL_FIL ${yacc_SOURCE_DIR} ${ABS_FIL})
    get_filename_component(REL_DIR ${REL_FIL} DIRECTORY)
    set(RELFIL_WE "${REL_DIR}/${FIL_WE}")
    add_custom_command(
      OUTPUT "${yacc_OUTPUT_DIR}/${RELFIL_WE}.h"
             "${yacc_OUTPUT_DIR}/${RELFIL_WE}.${yacc_EXT}"
      COMMAND
        ${PKG_DIR} ${M4} ${bison_EXECUTABLE} -d
        --defines=${yacc_OUTPUT_DIR}/${RELFIL_WE}.h -o
        ${yacc_OUTPUT_DIR}/${RELFIL_WE}.${yacc_EXT} ${ABS_FIL}
      COMMENT "Yacc'ing ${ABS_FIL} with a bison"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
      DEPENDS ${bison_EXECUTABLE} ${ABS_FIL} ${m4_EXECUTABLE})
    list(APPEND YACC_GEN ${yacc_OUTPUT_DIR}/${RELFIL_WE}.h)
    list(APPEND YACC_GEN ${yacc_OUTPUT_DIR}/${RELFIL_WE}.${yacc_EXT})
    set_source_files_properties(
      ${yacc_OUTPUT_DIR}/${RELFIL_WE}.h
      ${yacc_OUTPUT_DIR}/${RELFIL_WE}.${yacc_EXT} PROPERTIES GENERATED TRUE)
  endforeach()

  # Make the library available
  if(yacc_GENERATED)
    set(${yacc_GENERATED} "${YACC_GEN}" PARENT_SCOPE)
  endif()
endfunction()
