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


# This function retrieves th git_version, or reverting to the date if
# we cannot fetch it.
#
# VER
#   The variable to set the version in
# Sets ${VER}
#   The version as reported by git, or the date
function(get_git_version VER)
    execute_process(COMMAND "git" "describe" "--match" "v*"
        WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
        RESULT_VARIABLE GIT_RES
        OUTPUT_VARIABLE STD_OUT
        ERROR_VARIABLE STD_ERR)
    if(NOT "${GIT_RES}" STREQUAL "0")
        message(WARNING "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR}, out: ${STD_OUT}, err: ${STD_ERR}")
        execute_process(COMMAND "date" "+%Y-%m-%d"
            WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
            RESULT_VARIABLE DATE_RES
            OUTPUT_VARIABLE STD_OUT
            ERROR_VARIABLE STD_ERR)
        if(NOT "${DATE_RES}" STREQUAL "0")
            message(FATAL_ERROR "Unable to retrieve date!")
        endif ()
    endif()

    # Clean up and make visibile
    string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
    set(${VER} ${STD_OUT} PARENT_SCOPE)
endfunction ()

# VER
#   The variable to set the sha in
# Sets ${VER}
#   The latest sha as reported by git
function(get_git_sha VER)
    execute_process(COMMAND "git" "log" "-n" "1" "--pretty=format:'%H'"
        WORKING_DIRECTORY ${ANDROID_QEMU2_TOP_DIR}
        RESULT_VARIABLE GIT_RES
        OUTPUT_VARIABLE STD_OUT
        ERROR_VARIABLE STD_ERR)
    if(NOT "${GIT_RES}" STREQUAL "0")
        message(FATAL_ERROR "Unable to retrieve git version from ${ANDROID_QEMU2_TOP_DIR} : out: ${STD_OUT}, err: ${STD_ERR}")
    endif()

    # Clean up and make visibile
    string(REPLACE "\n" "" STD_OUT "${STD_OUT}")
    set(${VER} ${STD_OUT} PARENT_SCOPE)
endfunction ()

