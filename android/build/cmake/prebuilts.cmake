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

set(PREBUILT_COMMON "BLUEZ;LZ4;X264")


# Internal function for simple poackages.
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
