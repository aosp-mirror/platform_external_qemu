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

# This contains a set of definitions to make working with prebuilts easier and manageable.

set(PREBUILT_COMMON "BLUEZ;LZ4;X264")


function(android_add_prebuilt_library PKG MODULE LOCATION INCLUDES DEFINTIONS)
  if (NOT TARGET ${PKG}::${MODULE})
    add_library(${PKG}::${MODULE} STATIC IMPORTED GLOBAL)
    set_target_properties(${PKG}::${MODULE} PROPERTIES
      IMPORTED_LOCATION "${LOCATION}${CMAKE_STATIC_LIBRARY_SUFFIX}"
      INTERFACE_INCLUDE_DIRECTORIES "${INCLUDES}"
      INTERFACE_COMPILE_DEFINITIONS "${DEFINTIONS}"
    )

    android_log("set_target_properties(${PKG}::${MODULE} PROPERTIES
        IMPORTED_LOCATION '${LOCATION}${CMAKE_STATIC_LIBRARY_SUFFIX}'
        INTERFACE_INCLUDE_DIRECTORIES '${INCLUDES}'
        INTERFACE_COMPILE_DEFINITIONS '${DEFINTIONS}'
    )")
  endif()
endfunction()


# Internal function for simple packages.
function(simple_prebuilt Package)
  # A simple common package..
  string(TOLOWER ${Package} pkg)
  string(TOUPPER ${Package} PKG)
  get_filename_component(
    PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/${pkg}/${ANDROID_TARGET_TAG}"
    ABSOLUTE)

  android_add_prebuilt_library("${PKG}" "${PKG}" "${PREBUILT_ROOT}/lib/lib${pkg}" "${PREBUILT_ROOT}/include" "" "")
  set(${PKG}_INCLUDE_DIRS "${PREBUILT_ROOT}/include" PARENT_SCOPE)
  set(${PKG}_INCLUDE_DIR "${PREBUILT_ROOT}/include" PARENT_SCOPE)
  set(${PKG}_LIBRARIES "${PREBUILT_ROOT}/lib/lib${pkg}${CMAKE_STATIC_LIBRARY_SUFFIX}" PARENT_SCOPE)
  set(${PKG}_FOUND TRUE PARENT_SCOPE)
  set(PACKAGE_EXPORT "${PKG}_INCLUDE_DIR;${PKG}_INCLUDE_DIRS;${PKG}_LIBRARIES;${PKG}_FOUND" PARENT_SCOPE)
endfunction()

# This function is to be used as a replacement for find_package it will discover the internal prebuilts needed to
# compile the emulator

# It will revert to standard find_package if the TARGET_TAG and QEMU2_TOP_DIR properties are not defined.
#
# If they are defined the emu-{Package}-config.cmake will be executed. This script should mimic FindPackage for the
# specific Package, it should set all variables of relevance and declare these in PACKAG_EXPORT so they can be made
# visible.
#
# See: emu-curl-config.cmake for a simple example. See: emu-protobuf-config.cmake for a more complicated example.
#
# Package The package which we wish to resolve
function(prebuilt Package)
  string(TOLOWER ${Package} pkg)
  string(TOUPPER ${Package} PKG)
  if(${PKG}_FOUND)
    android_log("${Package} has already been found")
    return()
  endif()
  if(DEFINED ANDROID_QEMU2_TOP_DIR AND DEFINED ANDROID_TARGET_TAG)
    if(${PKG} IN_LIST PREBUILT_COMMON)
      simple_prebuilt(${Package})
    else()
      get_filename_component(PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../.." ABSOLUTE)

      # We make sure we use our internal cmake directory to resolve packages
      get_filename_component(ANDROID_TOOLS_DIRECTORY "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/config" ABSOLUTE)
      set(${Package}_DIR "${ANDROID_TOOLS_DIRECTORY}")

      # This will cause cmake to look for emu-${pkg}-config.cmake in the directory above.
      find_package(${Package} REQUIRED NAMES "emu-${pkg}")

      # Oh oh, this could be bad.. (But usually isn't)
      foreach(LIB ${${PKG}_LIBRARIES})
        if(NOT ${LIB} MATCHES "-l.*|-L.*|${PREBUILT_ROOT}.*")
          android_log("Discovered ${Package} lib  ${LIB}  which is outside of tree ${PREBUILT_ROOT}!")
        endif()
      endforeach()
    endif()
    foreach(INC ${${PKG}_INCLUDE_DIRS})
      if(NOT EXISTS ${INC})
        message(FATAL_ERROR "The include directory ${INC} of ${Package} does not exist..")
      endif()
    endforeach()
    android_log("Found ${Package}: ${${PKG}_LIBRARIES} (Prebuilt version)")

    # Export the variables to parent
    foreach(TO_EXP ${PACKAGE_EXPORT})
      set(${TO_EXP} "${${TO_EXP}}" PARENT_SCOPE)
    endforeach()
  else()

    # You are not using our make system!
    android_log("${Package} is outside of emulator build!")
    find_package(${PKG} REQUIRED)
  endif()
endfunction(prebuilt pkg)

# Add the runtime dependencies, i.e. the set of libs that need to be copied over to the proper location in order to run
# the executable.
#
# RUN_TARGET The executable that needs these run time dependencies
#
# TARGET_TAG The android target tag for which these are applicable, or all for all targets.
#
# RUN_TARGET_DEPENDENCIES:
# .   List of SRC>DST pairs that contains individual file dependencies.
# .   List of SRC_DIR>>DST pairs that contains directorie depedencies.
#
# For example:
#   set(MY_FOO_DEPENDENCIES "/tmp/a/b.txt>lib/some/dir/c.txt;" . # copy from /tmp/a/b.txt to lib/some/dir/c.txt
#                           "/tmp/dirs>>lib/x")  # recursively copy /tmp/dirs to lib/x
#
# TODO: It would be nice if we could get these to propagate through link_libraries, so we would have
# proper dependency resolution.
function(android_target_dependency RUN_TARGET TARGET_TAG RUN_TARGET_DEPENDENCIES)
  if(TARGET_TAG STREQUAL "${ANDROID_TARGET_TAG}" OR TARGET_TAG STREQUAL "all")
    list(REMOVE_DUPLICATES RUN_TARGET_DEPENDENCIES)
    foreach(DEP ${RUN_TARGET_DEPENDENCIES})
      # Are we copying a directory or a file?
      if(DEP MATCHES ".*>>.*")
        string(REPLACE ">>" ";" SRC_DST ${DEP})
        list(GET SRC_DST 0 SRC)
        list(GET SRC_DST 1 DST)
        file(GLOB GLOBBED ${SRC})
        if(NOT GLOBBED)
          message(
            FATAL_ERROR
            "The target ${RUN_TARGET} depends on a dependency: [${SRC}]/[${GLOBBED}] that does not exist. Full list ${RUN_TARGET_DEPENDENCIES}!"
            )
        endif()
        if(HOST_TAG STREQUAL "windows_msvc-x86_64")
          add_custom_command(TARGET ${RUN_TARGET} POST_BUILD
                             COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC}"
                                     "$<TARGET_FILE_DIR:${RUN_TARGET}>/${DST}"
                             MAIN_DEPENDENCY ${SRC})
        else()
          # We want to keep symlinks intact, so we just rsync the thing
          add_custom_command(TARGET ${RUN_TARGET} POST_BUILD
              # Make sure the directory exists
              COMMAND mkdir -p $$\( dirname "$<TARGET_FILE_DIR:${RUN_TARGET}>/${DST}" \)
              # And copy all, leaving the symlinks intact.
              COMMAND rsync -rl "${SRC}" "$<TARGET_FILE_DIR:${RUN_TARGET}>/${DST}"
                             MAIN_DEPENDENCY ${SRC})
        endif()
      else()
        # Turns src>dst into a list, so we can split out SRC --> DST
        string(REPLACE ">" ";" SRC_DST ${DEP})
        list(GET SRC_DST 0 SRC)
        list(GET SRC_DST 1 DST)
        if(NOT EXISTS ${SRC})
          message(
            FATAL_ERROR
              "The target ${RUN_TARGET} depends on a dependency: ${SRC} that does not exist. Full list ${RUN_TARGET_DEPENDENCIES}!"
            )
        endif()
        add_custom_command(TARGET ${RUN_TARGET} POST_BUILD
                           COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC}"
                                   "$<TARGET_FILE_DIR:${RUN_TARGET}>/${DST}"
                           MAIN_DEPENDENCY ${SRC})
      endif()
    endforeach()
  endif()
endfunction()

# Add the runtime dependencies, i.e. the set of libs that need to be copied over to the proper location in order to run
# the executable.
#
# RUN_TARGET The executable that needs these run time dependencies
#
# TARGET_TAG The android target tag for which these are applicable, or all for all targets.
#
# RUN_TARGET_PROPERTIES List of properties that should be set on the target in order for it to properly link and run
# with the prebuilt dependency. For example LINK=--nmagic. Will set the linker flag --nmagic during compilation.
function(android_target_properties RUN_TARGET TARGET_TAG RUN_TARGET_PROPERTIES)
  if(TARGET_TAG STREQUAL "${ANDROID_TARGET_TAG}" OR TARGET_TAG STREQUAL "all")
    foreach(PROP ${RUN_TARGET_PROPERTIES})
      if(PROP MATCHES ".*>=.*")
        # We are appending
        string(REPLACE ">=" ";" KEY_VAL ${PROP})
        list(GET KEY_VAL 0 KEY)
        list(GET KEY_VAL 1 VAL)
        # Note we are not treating the propery as a list!
        get_property(CURR_VAL TARGET ${RUN_TARGET} PROPERTY ${KEY})
        # Of course we deal with lists differently on different platforms!
        if (APPLE)
          set_property(TARGET ${RUN_TARGET} PROPERTY "${KEY}" ${CURR_VAL};${VAL})
          android_log("set_property(TARGET ${RUN_TARGET} PROPERTY '${KEY}' ${CURR_VAL};${VAL})")
        else()
          set_property(TARGET ${RUN_TARGET} PROPERTY "${KEY}" "${CURR_VAL} ${VAL}")
          android_log("set_property(TARGET ${RUN_TARGET} PROPERTY '${KEY}' ${CURR_VAL} ${VAL})")
        endif()
      else()
        # We are replacing
        string(REPLACE "=" ";" KEY_VAL ${PROP})
        list(GET KEY_VAL 0 KEY)
        list(GET KEY_VAL 1 VAL)
        set_property(TARGET ${RUN_TARGET} PROPERTY "${KEY}" "${VAL}")
        android_log("set_property(TARGET ${RUN_TARGET} PROPERTY '${KEY}' '${VAL}')")
      endif()
    endforeach()
  endif()
endfunction()
