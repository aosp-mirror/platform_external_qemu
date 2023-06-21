# Copyright 2023 The Android Open Source Project
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
if(INCLUDE_SYMBOLS_CMAKE)
  return()
endif()

set(INCLUDE_SYMBOLS_CMAKE 1)

# This function uploads and processes symbols for Android applications.
# Parameters: - TARGET: The name of the target library. - DIRECTORY: The
# directory to locally store the symbols.
#
# The following are optional:
#
# * API_KEY: The API key to use for uploading symbols.
# * URI: The URI of the server to upload the symbols to.
function(android_upload_symbols)
  if (OPTION_ASAN)
    message(STATUS "Santizer is enabled. Skipping symbol upload")
    return()
  endif()

  # Parse arguments
  set(options)
  set(oneValueArgs TARGET API_KEY DIRECTORY URI)
  cmake_parse_arguments(symbols "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  if(NOT symbols_DIRECTORY)
    message(
      STATUS "No output specificied, not creating symbols for ${symbols_TARGET}"
    )
    return()
  endif()

  # Check for cross compilation, without Rosetta.
  if(CROSSCOMPILE AND NOT (APPLE AND DARWIN_X86_64))
    message(
      STATUS
        "Unable to create symbols when cross compiling ${symbols_TARGET}, host: ${ANDROID_HOST_TAG}, target: ${ANDROID_TARGET_TAG}"
    )
    return()
  endif()

  # Make sure we have a python executable (should already be set.)
  if(NOT Python_EXECUTABLE)
    message(WARNING "No python interpreter set! Finding default.")
    find_package(Python COMPONENTS Interpreter)
    set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
  endif()

  # Make sure we have dump_syms and sym_upload available, otherwise we will need
  # to add the target.
  if(NOT TARGET breakpad_common)
    message("Importing breakpad. [${INCLUDE_BREAKPAD_CMAKE}]")
    add_subdirectory("${ANDROID_QEMU2_TOP_DIR}/android/third_party/breakpad"
                     symbols-cross-breakpad)
  endif()

  # Configure the upload executable, it works differently on all platforms
  set(DEST "${symbols_DIRECTORY}/${symbols_TARGET}.sym")
  if(NOT symbols_API_KEY OR NOT symbols_URI)
    set(UPLOAD_CMD echo "Not uploading symbols.")
  else()
    if(WIN32)
      set(UPLOAD_CMD sym_upload -p $<TARGET_FILE:${symbols_TARGET}>
                     ${symbols_URI} ${symbols_API_KEY})
    else()
      set(UPLOAD_CMD
          sym_upload
          -f
          -p
          sym-upload-v2
          -k
          ${symbols_API_KEY}
          ${DEST}
          ${symbols_URI})
    endif()
  endif()

  if(APPLE)
    add_custom_command(
      TARGET ${symbols_TARGET}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${symbols_DIRECTORY}
      COMMAND dump_syms -d -m "$<TARGET_FILE:${symbols_TARGET}>" > ${DEST}
      COMMAND ${UPLOAD_CMD}
      COMMAND
        "${Python_EXECUTABLE}"
        "${ANDROID_QEMU2_TOP_DIR}/android/build/python/aemu/symbol_processor.py"
        "-o" "${symbols_DIRECTORY}" "${DEST}"
      COMMENT "Processing symbols for ${symbols_TARGET}"
      VERBATIM DEPENDS sym_upload dump_syms)
  else()
    add_custom_command(
      TARGET ${symbols_TARGET}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${symbols_DIRECTORY}
      COMMAND dump_syms "$<TARGET_FILE:${symbols_TARGET}>" > ${DEST}
      COMMAND ${UPLOAD_CMD}
      COMMAND
        "${Python_EXECUTABLE}"
        "${ANDROID_QEMU2_TOP_DIR}/android/build/python/aemu/symbol_processor.py"
        "-o" "${symbols_DIRECTORY}" "${DEST}"
      COMMENT "Processing symbols for ${symbols_TARGET}"
      VERBATIM DEPENDS sym_upload dump_syms)
  endif()
endfunction()
