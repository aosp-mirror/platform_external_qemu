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

PROGRAM_DESCRIPTION="Runs all the cmake based unit tests."

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
SDK_FLAGS=
OPT_OUT=${OPT_OUT}/cmake_tests


echo "Running test on ${HOST_OS}"

rm -rf ${OPT_OUT}; mkdir -p ${OPT_OUT}

# Prints a warning in red if possible
warn() {
    echo "${RED}$1${RESET}"
}

# Generates the toolchain to be used by cmake. You can the G

function gen_toolchain {
    SDK_TOOLCHAIN_DIR=$OPT_OUT/build/toolchain
    GEN_SDK_FLAGS="$SDK_FLAGS --aosp-dir=$AOSP_DIR"
    GEN_SDK=${QEMU2_TOP_DIR}/android/scripts/gen-android-sdk-toolchain.sh
    run "$GEN_SDK" $GEN_SDK_FLAGS "--verbosity=$(get_verbosity)" "$SDK_TOOLCHAIN_DIR" || panic "Cannot generate SDK toolchain!"
    BINPREFIX=$("$GEN_SDK" $GEN_SDK_FLAGS --print=binprefix "$SDK_TOOLCHAIN_DIR")
    CC="$SDK_TOOLCHAIN_DIR/${BINPREFIX}gcc"
    CXX="$SDK_TOOLCHAIN_DIR/${BINPREFIX}g++"
    AR="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ar"
    RANLIB="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ranlib"
    LD=$CXX
    OBJCOPY="$SDK_TOOLCHAIN_DIR/${BINPREFIX}objcopy"
}


# Configures the cmake paramaters and generates the toolchain.
# $1  - The output location of the generated makefiles
# $2  - Target tag used to build (linux-x86_64, darwin-x86_64, windows-x86, windows-x86_64)
# $3  - Qemu2 top directory (../external/qemu)
function cmake_params {
    # Generate the toolchain..
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

# Runs the cmake generator, this should resolve all prebuilts and succeed.
# $1: The output directory
# $2: Target tag used to build (linux-x86_64, darwin-x86_64, windows-x86, windows-x86_64)
function run_cmake {
    echo "Running test: Find all prebuilts succeed for $2"
    gen_toolchain
    export CC
    export CXX
    CMAKE_PARAMS=$(cmake_params $1 $2 $QEMU2_TOP_DIR)
    run ${CMAKE_TOOL} ${CMAKE_PARAMS} \
        || panic "Failed to run cmake for target $2"
}

# Runs the cmake generator by passing a bad ${QEMU2_TOP_DIR) parameter. It is
# expected that cmake will fail, as depenecies will be missing.
# $1: The output directory
# $2: Target tag used to build (linux-x86_64, darwin-x86_64, windows-x86, windows-x86_64)
function failing_cmake {
    echo "Running test: Incorred prebuilt dir should fail for $2"
    gen_toolchain
    CMAKE_PARAMS=$(cmake_params $1 $2 /a/dir/without/prebuilts)
    run ${CMAKE_TOOL} ${CMAKE_PARAMS} && panic "Unexpected success for cmake run on target $2"
}

# Attempts to build the cmake project using the local make environment
# These tests are not expected to succeed, as they depened on you having
# the right packages (Protobuf, etc) installed.
#
# $1: Test runner (wine) used to launch the sample program
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

function run_windows_tests {
    SDK_FLAGS=" --host=windows-x86_64"

    echo "Running failure case for win64"
    failing_cmake $OPT_OUT/build/test/fail-win64 windows-x86_64

    echo "Running official win64 build"
    run_cmake $OPT_OUT/build/test/win64 windows-x86_64
    run cd $OPT_OUT/build/test/win64 &&
        make &&
        wine $OPT_OUT/build/test/win64/hello.exe ||
        panic "Failed to make win64 project"

    SDK_FLAGS=" --host=windows-x86"

    # Lets do the windows things!
    echo "Running failure case for win32"
    failing_cmake $OPT_OUT/build/test/fail-win32 windows-x86

    echo "Running official win32 build"
    run_cmake $OPT_OUT/build/test/win32 windows-x86
    run cd $OPT_OUT/build/test/win32 &&
        make  &&
        wine $OPT_OUT/build/test/win32/hello.exe ||
        panic "Failed to make win32 project"
}

function run_linux_tests {
    SDK_FLAGS=""
    echo "Running failure case"
    failing_cmake $OPT_OUT/build/test/fail-linux linux-x86_64

    echo "Running official linux build"
    run_cmake $OPT_OUT/build/test/linux linux-x86_64
    run cd $OPT_OUT/build/test/linux &&
        make &&
        $OPT_OUT/build/test/linux/hello ||
        panic "Failed to make linux project"

    local_cmake
}

function run_darwin_tests {
    SDK_FLAGS=""
    echo "Running failure case"
    failing_cmake $OPT_OUT/build/test/fail-darwin darwin-x86_64

    echo "Running official build"
    run_cmake $OPT_OUT/build/test/darwin darwin-x86_64
    run cd $OPT_OUT/build/test/darwin && make || panic "Failed to make darwin project"

    echo "Running executable"
    run $OPT_OUT/build/test/darwin/hello || panic "Unable to execute hello"

    local_cmake
}

# Run tests on mac
if [ "$HOST_OS" = "darwin" ]; then
    run_darwin_tests
fi

# Run tests on linux
if [ "$HOST_OS" = "linux" ]; then
    run_windows_tests
    run_linux_tests
fi;


echo "Constructed all cmake projects successfully!"
