#!/bin/sh

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

. $(realpath $(dirname "$0")/../../../scripts/utils)/common.shi
_SHU_PROGDIR=$(realpath $(dirname "$0")/../../../scripts/)

shell_import utils/aosp_dir.shi
shell_import utils/temp_dir.shi
shell_import utils/option_parser.shi

# Fancy colors in the terminal
if [ -t 1 ] ; then
  RED=`tput setaf 1`
  GREEN=`tput setaf 2`
  RESET=`tput sgr0`
else
  RED=
  GREEN=
  RESET=
fi


PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Runs all the cmake based unit tests."


OPT_OUT=objs
option_register_var "--out-dir=<dir>" OPT_OUT "Use specific output directory"

aosp_dir_register_option
option_parse "$@"
aosp_dir_parse_option


cd $PROGDIR/../../..

# We don't want to bail immediately in case of failures
set +e

QEMU2_TOP_DIR=${AOSP_DIR}/external/qemu
HOST_OS=$(get_build_os)
CMAKE_TOOL=${AOSP_DIR}/prebuilts/cmake/${HOST_OS}-x86/bin/cmake


echo "Running test on ${HOST_OS}"

# Prints a warning in red if possible
warn() {
    echo "${RED}$1${RESET}"
}
SDK_TOOLCHAIN_DIR=$OPT_OUT/build/toolchain
GEN_SDK=${QEMU2_TOP_DIR}/android/scripts/gen-android-sdk-toolchain.sh
BINPREFIX=$("$GEN_SDK" $GEN_SDK_FLAGS --print=binprefix "$SDK_TOOLCHAIN_DIR")
CC="$SDK_TOOLCHAIN_DIR/${BINPREFIX}gcc"
CXX="$SDK_TOOLCHAIN_DIR/${BINPREFIX}g++"
AR="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ar"
RANLIB="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ranlib"
LD=$CXX
OBJCOPY="$SDK_TOOLCHAIN_DIR/${BINPREFIX}objcopy"


function cmake_params {
    local CMAKE_PARAMS=-H${QEMU2_TOP_DIR}/android/build/cmake/test
    CMAKE_PARAMS="${CMAKE_PARAMS} -B$1"
    CMAKE_PARAMS="${CMAKE_PARAMS} -DCMAKE_AR=${AR} "
    CMAKE_PARAMS="${CMAKE_PARAMS} -DCMAKE_RANLIB=${RANLIB} "
    CMAKE_PARAMS="${CMAKE_PARAMS} -DCMAKE_OBJCOPY=${OBJCOPY} "
    CMAKE_PARAMS="${CMAKE_PARAMS} -DLOCAL_CXXFLAGS=${CXXFLAGS} "
    CMAKE_PARAMS="${CMAKE_PARAMS} -DLOCAL_QEMU2_TOP_DIR=${3} "
    CMAKE_PARAMS="${CMAKE_PARAMS} -DLOCAL_TARGET_TAG=$2 "
    CMAKE_PARAMS="${CMAKE_PARAMS} -DLOCAL_BUILD_OBJS_DIR=${BUILD_OBJS_DIR} "

    if [[ $2 = *"windows"* ]]; then
        CMAKE_PARAMS="${CMAKE_PARAMS} -DCMAKE_TOOLCHAIN_FILE=${QEMU2_TOP_DIR}/android/build/cmake/toolchain-win.cmake"
    fi
   echo $CMAKE_PARAMS
}

function run_cmake {
    echo "Running test: Find all prebuilts succeed for $2"
    CMAKE_PARAMS=$(cmake_params $1 $2 $QEMU2_TOP_DIR)

    run ${CMAKE_TOOL} ${CMAKE_PARAMS} \
    || panic "Failed to run cmake for target $2"
}

function failing_cmake {
    echo "Running test: Incorred prebuilt dir should fail for $2"
    CMAKE_PARAMS=$(cmake_params $1 $2 /a/dir/without/prebuilts)
    run ${CMAKE_TOOL} ${CMAKE_PARAMS} && panic "Unexpected success for cmake run on target $2"
}

function local_cmake {
    echo "Running test: Build outside android should succeed for $2"
    run ${CMAKE_TOOL} \
        -H${QEMU2_TOP_DIR}/android/build/cmake/test \
        -B$OPT_OUT/build/test/local &&
    run cd $OPT_OUT/build/test/local &&
    run make &&
    run $1 $OPT_OUT/build/test/local/hello ||
    warn "Failed to run local cmake (This is okay)"}
}

run_cmake $OPT_OUT/build/test/linux linux-x86_64
run_cmake $OPT_OUT/build/test/darwin darwin-x86_64
run_cmake $OPT_OUT/build/test/win32 windows-x86
run_cmake $OPT_OUT/build/test/win64 windows-x86_64

failing_cmake $OPT_OUT/build/test/fail-linux linux-x86_64
failing_cmake $OPT_OUT/build/test/fail-darwin darwin-x86_64
failing_cmake $OPT_OUT/build/test/fail-win32 windows-x86
failing_cmake $OPT_OUT/build/test/fail-win64 windows-x86_64

if [ "$HOST_OS" = "darwin" ]; then
   echo "Running official build"
   run cd $OPT_OUT/build/test/darwin && make || panic "Failed to make darwin project"

   echo "Running executable"
   run $OPT_OUT/build/test/darwin/hello || panic "Unable to execute hello"

   local_cmake
fi

if [ "$HOST_OS" = "linux" ]; then
   echo "Running official linux build"
   run cd $OPT_OUT/build/test/linux && make || panic "Failed to make linux project"

   echo "Running official win32 build"
   run cd $OPT_OUT/build/test/win32 &&
       make  &&
       wine $OPT_OUT/buid/test/win32/hello.exe ||
       panic "Failed to make win32 project"

   echo "Running official win64 build"
   run cd $OPT_OUT/build/test/win64 &&
       make  &&
       wine $OPT_OUT/buid/test/win64/hello.exe ||
       panic "Failed to make win64 project"

   local_cmake
fi;


echo "Constructed all cmake projects successfully!"
