# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This contains a set of definitions to make working with prebuilts easier
# and manageable.

set(PREBUILT_COMMON "BLUEZ;LZ4;X264")

# Internal function for simple packages.
function(simple_prebuilt Package)
    # A simple common package..
    string(TOLOWER ${Package} pkg)
    string(TOUPPER ${Package} PKG)
    get_filename_component(PREBUILT_ROOT "${LOCAL_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/${pkg}/${LOCAL_TARGET_TAG}" ABSOLUTE)

    set(${PKG}_INCLUDE_DIRS "${PREBUILT_ROOT}/include" PARENT_SCOPE)
    set(${PKG}_INCLUDE_DIR "${PREBUILT_ROOT}/include" PARENT_SCOPE)
    set(${PKG}_LIBRARIES "${PREBUILT_ROOT}/lib/lib${pgk}.a" PARENT_SCOPE)
    set(${PKG}_FOUND TRUE PARENT_SCOPE)
    set(PACKAGE_EXPORT "${PKG}_INCLUDE_DIR;${PKG}_INCLUDE_DIRS;${PKG}_LIBRARIES;${PKG}_FOUND" PARENT_SCOPE)
endfunction()

# This function is to be used as a replacement for find_package
# it will discover the internal prebuilts needed to compile the emulator

# It will revert to standard find_package if the TARGET_TAG and QEMU2_TOP_DIR properties
# are not defined.
#
# If they are defined the emu-{Package}-config.cmake will be executed.
# This script should mimic FindPackage for the specific Package, it should set
# all variables of relevance and declare these in PACKAG_EXPORT so they can be
# made visible.
#
# See: emu-curl-config.cmake for a simple example.
# See: emu-protobuf-config.cmake for a more complicated example.
#
# Package
#   The package which we wish to resolve
function(prebuilt Package)
    string(TOLOWER ${Package} pkg)
    string(TOUPPER ${Package} PKG)
    if (DEFINED LOCAL_QEMU2_TOP_DIR AND DEFINED LOCAL_TARGET_TAG)
        if (${PKG} IN_LIST PREBUILT_COMMON)
            simple_prebuilt(${Package})
        else ()
            get_filename_component(PREBUILT_ROOT "${LOCAL_QEMU2_TOP_DIR}/../.." ABSOLUTE)

            # We make sure we use our internal cmake directory to resolve
            # packages
            set(${Package}_DIR "${TOOLS_DIRECTORY}/cmake")

            # This will cause cmake to look for emu-${pkg}-config.cmake in
            # the directory above.
            find_package(${Package} REQUIRED NAMES "emu-${pkg}")

            # Oh oh.
            if (NOT ${PKG}_LIBRARIES MATCHES "${PREBUILT_ROOT}.*")
                message(STATUS "Discovered ${Package} lib  -l${${PKG}_LIBRARIES} and -I${${PKG}_INCLUDE_DIR} which is outside of tree ${PREBUILT_ROOT}!")
            endif ()
        endif ()

        if (DEFINED ${PKG}_INCLUDE_DIR AND NOT EXISTS ${${PKG}_INCLUDE_DIR})
            message(FATAL_ERROR "The include ${${PKG}_INCLUDE_DIR} directory of ${Package} does not exist..")
        endif ()
        message(STATUS "Found ${Package}: ${${PKG}_LIBRARIES} (Prebuilt version)")

        # Export the variables to parent
        foreach(TO_EXP  ${PACKAGE_EXPORT})
            set(${TO_EXP} "${${TO_EXP}}" PARENT_SCOPE)
        endforeach()
    else()

        # You are not using our make system!
        message("${Package} is outside of emulator build!")
        find_package(${PKG} REQUIRED)
    endif()
endfunction (prebuilt pkg)

# Add the runtime dependencies, i.e. the set of libs that need to be copied over to the proper location
# in order to run the executable.
#
# RUN_TARGET
#   The executable that needs these run time dependencies
#
# RUN_TARGET_DEPENDENCIES
#   List of SRC>DST pairs that contains the dependencies.
#   This SRC will be copied to the destination folder.
#   These should be made available through running prebuilt(xxxx).
#   note that you will always want to include RUNTIME_OS_DEPENDENCIES
#   on your target
#
# RUN_TARGET_PROPERTIES
#   List of properties that should be set on the target in order for it to
#   properly link and run with the prebuilt dependency. For example
#   LINK=--nmagic. Will set the linker flag --nmagic during compilation.
function (target_prebuilt_dependency RUN_TARGET RUN_TARGET_DEPENDENCIES RUN_TARGET_PROPERTIES)
    list(REMOVE_DUPLICATES RUN_TARGET_DEPENDENCIES)
    foreach(DEP ${RUN_TARGET_DEPENDENCIES})
        message(STATUS "Handling ${DEP}")
        # Turns src>dst into a list, so we can
        # split out SRC --> DST
        string(REPLACE ">" ";" SRC_DST ${DEP})
        list(GET SRC_DST 0 SRC)
        list(GET SRC_DST 1 DST)
        if (NOT EXISTS ${SRC})
            message(FATAL_ERROR "The target ${RUN_TARGET} depends on a prebuilt: ${SRC_DST},${SRC} that does not exist!")
        endif ()

        #set(TARGET_DIR "$<TARGET_FILE_DIR:${RUN_TARGET}>/${DST}")
        #get_filename_component(DIR "${TARGET_DIR}" DIRECTORY)
        #message(FATAL_ERROR "We need to make ${DST} -> ${DIR}")
        #add_custom_target(make-directory-${RUN_TARGET} POST_BUILD
        #    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIR})

        add_custom_command(TARGET ${RUN_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SRC}"
            "$<TARGET_FILE_DIR:${RUN_TARGET}>/${DST}")

    endforeach()

    list(REMOVE_DUPLICATES RUN_TARGET_PROPERTIES)
    foreach(PROP ${RUN_TARGET_PROPERTIES})
        message(STATUS "Exploring ${PROP}")
        string(REPLACE "=" ";" KEY_VAL ${PROP})
        list(GET KEY_VAL 0 KEY)
        list(GET KEY_VAL 1 VAL)

        set(${KEY}_VAL "${${KEY}_VAL} ${VAL}")
        message(STATUS "set_target_properties(${RUN_TARGET} PROPERTIES ${KEY} ${${KEY}_VAL})")
        set_target_properties(${RUN_TARGET} PROPERTIES "${KEY}" "${${KEY}_VAL}")
    endforeach()

endfunction()


# Configure the OS dependencies, if we are in android make system..
if ( ${LOCAL_TARGET_TAG} MATCHES "windows-x86_64")
    set(GEN_SDK "${LOCAL_QEMU2_TOP_DIR}/android/scripts/gen-android-sdk-toolchain.sh")
    execute_process(COMMAND ${GEN_SDK} "--host=windows-x86_64" "--print=sysroot" "unused_param"
                    RESULT_VARIABLE GEN_SDK_RES
                    OUTPUT_VARIABLE MINGW_SYSROOT)
    string(REPLACE "\n" "" MINGW_SYSROOT "${MINGW_SYSROOT}")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/bin/libwinpthread-1.dll>lib64/libwinpthread-1.dll")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/lib32/libwinpthread-1.dll>lib/libwinpthread-1.dll")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/lib/libgcc_s_seh-1.dll>lib64/libgcc_s_seh-1.dll")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/lib32/libgcc_s_sjlj-1.dll>lib/libgcc_s_sjlj-1.dll")

    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-m64")
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-static-libgcc")
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-Xlinker")
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=--build-id")
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-mcx16")
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-Wl,-Bstatic -lstdc++")
elseif ( ${LOCAL_TARGET_TAG} MATCHES "windows-x86")
    set(GEN_SDK "${LOCAL_QEMU2_TOP_DIR}/android/scripts/gen-android-sdk-toolchain.sh")
    execute_process(COMMAND ${GEN_SDK} "--host=windows-x86" "--print=sysroot" "unused_param"
                    RESULT_VARIABLE GEN_SDK_RES
                    OUTPUT_VARIABLE MINGW_SYSROOT)
    string(REPLACE "\n" "" MINGW_SYSROOT "${MINGW_SYSROOT}")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/bin/libwinpthread-1.dll>lib64/libwinpthread-1.dll")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/lib32/libwinpthread-1.dll>lib/libwinpthread-1.dll")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/lib/libgcc_s_seh-1.dll>lib64/libgcc_s_seh-1.dll")
    list(APPEND RUNTIME_OS_DEPENDENCIES "${MINGW_SYSROOT}/lib32/libgcc_s_sjlj-1.dll>lib/libgcc_s_sjlj-1.dll")

    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-m32 -Xlinker --large-address-aware -mcx16 -Xlinker")
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=--stack -Xlinker 1048576 -static-libgcc -Xlinker --build-id -mcx16")
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-Wl,-Bstatic -lstdc++")
elseif ( ${LOCAL_TARGET_TAG} MATCHES "linux-x86_64")
    set(GEN_SDK "${LOCAL_QEMU2_TOP_DIR}/android/scripts/gen-android-sdk-toolchain.sh")
    execute_process(COMMAND ${GEN_SDK} "--print=libcplusplus" "unused_param"
                    RESULT_VARIABLE GEN_SDK_RES
                    OUTPUT_VARIABLE LIBCPLUSPLUS)

    # Resolve the files.
    string(REPLACE "\n" "" LIBCPLUSPLUS "${LIBCPLUSPLUS}")
    get_filename_component(RESOLVED_SO "${LIBCPLUSPLUS}" REALPATH)
    get_filename_component(RESOLVED_FILENAME "${RESOLVED_SO}" NAME)

    list(APPEND RUNTIME_OS_DEPENDENCIES "${LIBCPLUSPLUS}>lib64/${RESOLVED_FILENAME}")

    # Configure the RPATH be dynamic..
    list(APPEND RUNTIME_OS_PROPERTIES "LINK_FLAGS=-Wl,-rpath,'$ORIGIN/lib64'")
elseif (${LOCAL_TARGET_TAG} MATCHES "darwin-x86")
    list(APPEND RUNTIME_OS_PROPERTIES "")
    list(APPEND RUNTIME_OS_DEPENDENCIES "")
endif ()

message("OS_DEPS: [${RUNTIME_OS_DEPENDENCIES}] - [${RUNTIME_OS_PROPERTIES}]")


