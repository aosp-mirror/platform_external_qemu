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

set(PREBUILT_COMMON "BLUEZ;LZ4")
if(NOT WINDOWS_MSVC_X86_64)
  list(APPEND PREBUILT_COMMON "X264")
endif()

function(android_add_prebuilt_library PKG MODULE LOCATION INCLUDES DEFINTIONS)
  if(NOT TARGET ${PKG}::${MODULE})
    add_library(${PKG}::${MODULE} STATIC IMPORTED GLOBAL)
    set_target_properties(${PKG}::${MODULE}
                          PROPERTIES IMPORTED_LOCATION
                                     "${LOCATION}${CMAKE_STATIC_LIBRARY_SUFFIX}"
                                     INTERFACE_INCLUDE_DIRECTORIES
                                     "${INCLUDES}"
                                     INTERFACE_COMPILE_DEFINITIONS
                                     "${DEFINTIONS}")

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

# Installs the given src file into the given destination
function(internal_android_install_file SRC DST_DIR)
  # src could be a symlink, so let's resolve it.
  get_filename_component(REAL_SRC "${SRC}" REALPATH)

  # The names without directories of the src and resolved src.
  get_filename_component(FNAME "${SRC}" NAME)
  get_filename_component(FNAME_REAL "${REAL_SRC}" NAME)

  # Okay, we now need to determine if REAL_SRC is an executable, or file
  set(PYTHON_SCRIPT "import os; print os.path.isfile('${REAL_SRC}') and os.access('${REAL_SRC}', os.X_OK)")
  execute_process(COMMAND python -c "${PYTHON_SCRIPT}"
                  WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                  RESULT_VARIABLE SUCCESS
                  OUTPUT_VARIABLE STD_OUT
                  ERROR_VARIABLE STD_ERR)
  string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
  if(STD_OUT)
    android_log(STATUS "install(PROGRAMS ${SRC} ${REAL_SRC} DESTINATION ${DST_DIR})")
    install(PROGRAMS ${SRC} DESTINATION ${DST_DIR})
    android_strip_prebuilt("${DST_DIR}/${FNAME}")
    # Check if we have a symlink, gradle doesn't support symlinks, so we are
    # copying it 2x
    if(NOT ${SRC} STREQUAL ${REAL_SRC})
      android_log("${SRC} ==> ${REAL_SRC}")
      install(PROGRAMS ${REAL_SRC} DESTINATION ${DST_DIR})
      android_strip_prebuilt("${DST_DIR}/${FNAME_REAL}")
    endif()
  else()
    install(FILES ${SRC} DESTINATION ${DST_DIR})
    # Let's see if it is a shared library
    if (${SRC} MATCHES ".+${CMAKE_SHARED_LIBRARY_SUFFIX}(\\.[0-9]+)$")
        # Note we should eventually remove this: b/1381606
        android_strip_prebuilt("${DST_DIR}/${FNAME}")
    endif()
    if(NOT ${SRC} STREQUAL ${REAL_SRC})
      android_log("${SRC} ==> ${REAL_SRC}")
      install(FILES ${REAL_SRC} DESTINATION ${DST_DIR})
      # Check if we have a symlink, gradle doesn't support symlinks, so we are
      # copying it 2x
      if (${REAL_SRC} MATCHES ".+${CMAKE_SHARED_LIBRARY_SUFFIX}(\\.[0-9]+)$")
          android_strip_prebuilt("${DST_DIR}/${FNAME_REAL}")
      endif()
    endif()
  endif()
endfunction()

# Installs the given src file into the given destination
function(internal_android_install_file_force_exec SRC DST_DIR)
  # src could be a symlink, so let's resolve it.
  get_filename_component(REAL_SRC "${SRC}" REALPATH)

  # The names without directories of the src and resolved src.
  get_filename_component(FNAME "${SRC}" NAME)
  get_filename_component(FNAME_REAL "${REAL_SRC}" NAME)

  # Okay, we now need to determine if REAL_SRC is an executable, or file
  set(PYTHON_SCRIPT "import os; print os.path.isfile('${REAL_SRC}') and os.access('${REAL_SRC}', os.X_OK)")
  execute_process(COMMAND python -c "${PYTHON_SCRIPT}"
                  WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
                  RESULT_VARIABLE SUCCESS
                  OUTPUT_VARIABLE STD_OUT
                  ERROR_VARIABLE STD_ERR)
  string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
  android_log(STATUS "install(PROGRAMS ${SRC} ${REAL_SRC} DESTINATION ${DST_DIR})")
  install(PROGRAMS ${SRC} DESTINATION ${DST_DIR})
  android_strip_prebuilt("${DST_DIR}/${FNAME}")
  # Check if we have a symlink, gradle doesn't support symlinks, so we are
  # copying it 2x
  if(NOT ${SRC} STREQUAL ${REAL_SRC})
    android_log("${SRC} ==> ${REAL_SRC}")
    install(PROGRAMS ${REAL_SRC} DESTINATION ${DST_DIR})
    android_strip_prebuilt("${DST_DIR}/${FNAME_REAL}")
  endif()
endfunction()

# Installs the given dependency, this will make it part of the final release.
function(android_install_dependency TARGET_TAG INSTALL_DEPENDENCIES)
  if(TARGET_TAG STREQUAL "${ANDROID_TARGET_TAG}" OR TARGET_TAG STREQUAL "all")
    # Link to existing target if there.
    if(NOT TARGET ${INSTALL_DEPENDENCIES})
      message(FATAL_ERROR "Dependencies ${INSTALL_DEPENDENCIES} has not been declared (yet?)")
    endif()
    get_target_property(FILE_LIST ${INSTALL_DEPENDENCIES} SOURCES)
    foreach(FNAME ${FILE_LIST})
      if(NOT FNAME MATCHES ".*dummy.c")
        string(REPLACE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/" "" DST_FILE ${FNAME})
        get_filename_component(DST_DIR "${DST_FILE}" DIRECTORY)
        internal_android_install_file(${FNAME} ${DST_DIR})
      endif()
    endforeach()
  endif()
endfunction()

function(android_install_dependency_force_exec TARGET_TAG INSTALL_DEPENDENCIES)
  if(TARGET_TAG STREQUAL "${ANDROID_TARGET_TAG}" OR TARGET_TAG STREQUAL "all")
    # Link to existing target if there.
    if(NOT TARGET ${INSTALL_DEPENDENCIES})
      message(FATAL_ERROR "Dependencies ${INSTALL_DEPENDENCIES} has not been declared (yet?)")
    endif()
    get_target_property(FILE_LIST ${INSTALL_DEPENDENCIES} SOURCES)
    foreach(FNAME ${FILE_LIST})
      if(NOT FNAME MATCHES ".*dummy.c")
        string(REPLACE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/" "" DST_FILE ${FNAME})
        get_filename_component(DST_DIR "${DST_FILE}" DIRECTORY)
        internal_android_install_file_force_exec(${FNAME} ${DST_DIR})
      endif()
    endforeach()
  endif()
endfunction()

# Binplaces the given source file to the given destination as part of the target
#
# It will be registered as a generated source.
function(android_binplace TARGET SRC_FILE DEST_FILE)
  if(NOT EXISTS ${SRC_FILE})
    message(FATAL_ERROR "The target ${TARGET} depends on a dependency: ${SRC_FILE} that does not exist!")
  endif()

  if(DEST_FILE MATCHES ".*\\.[O|o][B|b][J|j]" AND MSVC)
    # We don't want to expose .OBJ files to the MSVC linker, so we will post copy those.
    message(STATUS "Marking ${DEST_FILE} as post copy for windows. Modifications to this file will not be detected.")
    add_custom_command(TARGET ${TARGET} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${DEST_FILE}")
  else()
    target_sources(${TARGET} PRIVATE ${DEST_FILE})
    add_custom_command(OUTPUT "${DEST_FILE}" COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${DEST_FILE}")
  endif()
endfunction()

# Add the runtime dependencies, i.e. the set of libs that need to be copied over to the proper location in order to run
# the executable.
#
# RUN_TARGET The executable that needs these run time dependencies
#
# TARGET_TAG The android target tag for which these are applicable, or all for all targets.
#
# RUN_TARGET_DEPENDENCIES: .   List of SRC>DST pairs that contains individual file dependencies. .   List of
# SRC_DIR>>DST pairs that contains directorie depedencies.
#
# For example: set(MY_FOO_DEPENDENCIES # copy from /tmp/a/b.txt to lib/some/dir/c.txt "/tmp/a/b.txt>lib/some/dir/c.txt;"
# # recursively copy /tmp/dirs to lib/x lib/some/dir/c.txt "/tmp/dirs/*>>lib/x")
#
# Note that this relies on every binary being binplaced in the root, which implies that this will not work with every
# generator.
#
# This works by creating an empty dummy target that relies on a set of sources that are the files we want to copy over.
# We construct the list of files we want to copy over and create a custom command that does the copying. next we feed
# the generated sources as a target to our dummy lib. Since they are not .c/.cpp files they are required to be there but
# result in no action. The generated archives will be empty, i.e: nm -a archives/libSWIFTSHADER_DEPENDENCIES.a
#
# dummy.c.o: 0000000000000000 a dummy.c
#
# Now this target can be used by others to take a dependency on. So we get: 1. Fast rebuilds (as we use ninja/make file
# out sync detection) 2. Binplace only once per dependency (no more concurrency problems) 3. No more target dependent
# binplacing (i.e. if a target does not end up in the root) which likely breaks the xcode generator.
function(android_target_dependency RUN_TARGET TARGET_TAG RUN_TARGET_DEPENDENCIES)
  if(TARGET_TAG STREQUAL "${ANDROID_TARGET_TAG}" OR TARGET_TAG STREQUAL "all")

    # Link to existing target if there.
    if(TARGET ${RUN_TARGET_DEPENDENCIES})
      target_link_libraries(${RUN_TARGET} PRIVATE ${RUN_TARGET_DEPENDENCIES})
      return()
    endif()

    # Okay let's construct our target.
    add_library(${RUN_TARGET_DEPENDENCIES} ${ANDROID_QEMU2_TOP_DIR}/dummy.c)

    set(DEPENDENCIES "${${RUN_TARGET_DEPENDENCIES}}")
    set(DEP_SOURCES "")
    list(REMOVE_DUPLICATES DEPENDENCIES)
    foreach(DEP ${DEPENDENCIES})
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

        # Let's calculate the destination directory, so we can use this as our generated sources parameters.
        get_filename_component(SRC_DIR "${SRC}" DIRECTORY)
        set(DEST_SRC "")
        set(DEST_DIR "${CMAKE_BINARY_DIR}/${DST}")
        foreach(FNAME ${GLOBBED})
          string(REPLACE "${SRC_DIR}" "${DEST_DIR}" DEST_FILE "${FNAME}")
          android_binplace(${RUN_TARGET_DEPENDENCIES} ${FNAME} ${DEST_FILE})
        endforeach()
      else()
        # We are doing single file copies. Turns src>dst into a list, so we can split out SRC --> DST
        string(REPLACE ">" ";" SRC_DST ${DEP})
        list(GET SRC_DST 0 SRC)
        list(GET SRC_DST 1 DST)
        set(DEST_FILE "${CMAKE_BINARY_DIR}/${DST}")
        android_binplace(${RUN_TARGET_DEPENDENCIES} ${SRC} ${DEST_FILE})
      endif()
    endforeach()
    target_link_libraries(${RUN_TARGET} PRIVATE ${RUN_TARGET_DEPENDENCIES})
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
        if(APPLE)
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
