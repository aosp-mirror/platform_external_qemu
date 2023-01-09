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

. $(dirname "$0")/utils/common.shi
# The posix way of getting the full path..
PROGDIR="$( cd "$(dirname "$0")" ; pwd -P )"

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
"Runs all the unit tests and basic checks."


option_register_var "-j<count>" OPT_NUM_JOBS "Run <count> parallel build jobs [$(get_build_num_cores)]"
option_register_var "--jobs=<count>" OPT_NUM_JOBS "Same as -j<count>."

OPT_OUT=objs
option_register_var "--out-dir=<dir>" OPT_OUT "Use specific output directory"

OPT_DIST=objs
option_register_var "--dist-dir=<dir>" OPT_DIST "Use specific output directory"

OPT_DEBS=
option_register_var "--debs" OPT_DEBS "Discover the debian package dependencies, needed to launch the emulator (Debian/Ubuntu only)."

option_register_var "--skip-emulator-check" OPT_SKIP_EMULATOR_CHECK "Skip emulator check"

option_register_var "--gfxstream" OPT_GFXSTREAM "Run with gfxstream configurations"
aosp_dir_register_option
option_parse "$@"
aosp_dir_parse_option

if [ "$OPT_NUM_JOBS" ]; then
    NUM_JOBS=$OPT_NUM_JOBS
    log2 "Parallel jobs count: $NUM_JOBS"
else
    NUM_JOBS=$(get_build_num_cores)
    log2 "Auto-config: --jobs=$NUM_JOBS"
fi

cd $PROGDIR/../../..

QEMU2_TOP_DIR=${AOSP_DIR}/external/qemu
HOST_OS=$(get_build_os)
CTEST=${AOSP_DIR}/prebuilts/cmake/${HOST_OS}-x86/bin/ctest
CONFIG_MAKE=${OPT_OUT}/target.tag
# Extract the target os from target.tag
echo "current_dir=$PWD"
TARGET_OS=$(cat ${CONFIG_MAKE})
FAILURES=""
OSX_DEPLOYMENT_TARGET=10.11
RUN_EMUGEN_TESTS=true
RUN_GEN_ENTRIES_TESTS=true
NINJA=ninja



echo "Running test on ${HOST_OS} for target: ${TARGET_OS}"

# Return the type of a given file, using the /usr/bin/file command.
# $1: executable path.
# Out: file type string, or empty if the path is wrong.
get_file_type () {
    /usr/bin/file "$1" 2>/dev/null
}

# Prints a warning in red if possible
warn() {
    echo "${RED}$1${RESET}"
}

# Returns true if
# $1: List
# $2: element we are looking for
contains() {
    for item in $1; do
        if [ "$item" = "$2" ]; then
            echo "True"
            return
        fi
    done
    echo "False"
}

run_integration_test() {
    BUILDERNAME="Linux_gce"
    OS="linux"
    if [[ $OSTYPE == *"darwin"* ]]; then
        BUILDERNAME="Mac"
        OS="darwin"
    else
        # Activate vnc, so we have an X surface for the emulator.
        ps cax | grep vnc >/dev/null
        if [ $? -eq 1 ]; then
            log "Start VNC server"
            vncserver
        fi
    fi

    export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python
    export SDK_EMULATOR=$HOME/android-sdk
    export ANDROID_AVD_HOME=$HOME/.android/avd
    export ANDROID_SDK_ROOT=$HOME/android-sdk
    export ANDROID_HOME=$HOME/android-sdk
    export ANDROID_EMU_ENABLE_CRASH_REPORTING=NO
    export PYTHON=python3
    export DISPLAY=:1
    export AOSP_DIR

    mkdir -p $OPT_DIST/session
    run ${AOSP_DIR}/external/adt-infra/pytest/test_embedded/run_tests.sh --session_dir $OPT_DIST/session --emulator $OPT_OUT/emulator --generate
}

# Return true iff the file type string |$1| contains the expected
# substring |$2|.
# $1: executable file type
# $2: expected file type substring
check_file_type_substring () {
    printf "%s\n" "$1" | grep -q -E -e "$2"
}

# Return the minimum OS X version that a darwin binary targets.
# $1: executable path
# Out: minimum version (e.g. '10.8') or empty string on error.
darwin_min_version () {
    otool -l "$1" 2>/dev/null | awk \
    'BEGIN { CMD="" } $1 == "cmd" { CMD=$2 } $1 == "version" && CMD == "LC_VERSION_MIN_MACOSX" { print $2 }'
}

# List all executables to check later.
EXECUTABLES="emulator"

# Define EXPECTED_32BIT_FILE_TYPE and EXPECTED_64BIT_FILE_TYPE depending
# on the current target platform. Then EXPECTED_EMULATOR_BITNESS and
# EXPECTED_EMULATOR_FILE_TYPE accordingly.
case $TARGET_OS in
    darwin-aarch64)
        EXPECTED_64BIT_FILE_TYPE="Mach-O 64-bit executable arm64"
        EXPECTED_EMULATOR_BITNESS=64
        EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
    ;;
    darwin*)
        EXPECTED_64BIT_FILE_TYPE="Mach-O 64-bit executable x86_64"
        EXPECTED_EMULATOR_BITNESS=64
        EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
    ;;
    linux*)
        EXPECTED_64BIT_FILE_TYPE="ELF 64-bit LSB +executable, x86-64"
        EXPECTED_EMULATOR_BITNESS=64
        EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
    ;;
    *)
        panic "FAIL: Unsupported target platform: [$TARGET_OS]"
    ;;
esac


if [ ! "$OPT_GFXSTREAM" ] ; then
    # TODO(jansene): disable this once we have collected all crucial parts
    log "Integration tests are disabled."
    # run_integration_test
fi

export CTEST_OUTPUT_ON_FAILURE=1
export CTEST_PROGRESS_OUTPUT=1
OLD_DIR=$PWD
cd $OPT_OUT

# gfxstream relies on VK_ICD_FILENAMES to switch between different vulkan
# libraries. We want to use swiftshader_vk in unittests.
if [ "$OPT_GFXSTREAM" ] ; then
    export VK_ICD_FILENAMES=$PWD/lib64/vulkan/vk_swiftshader_icd.json
    # Path to the vulkan ICD loader used for testing, libvulkan.so
    export LD_LIBRARY_PATH=$PWD/lib64/vulkan:$PWD/lib64/gles_swiftshader
fi

# Flags for SwANGLE
export ANGLE_DEFAULT_PLATFORM=swiftshader
# Mac CTest fails if ANDROID_EMU_VK_ICD is not set explicitly
export ANDROID_EMU_VK_ICD=swiftshader

${CTEST} -j ${NUM_JOBS} --output-on-failure || ${CTEST} --rerun-failed --output-on-failure 1>&2 || panic "Failures in unittests"

# Generate the coverage report if profraw files are in $OPT_OUT after running
# the unittests.
PROFRAWS=$(ls *.profraw)
CLANG_BINDIR=$AOSP_DIR/$(aosp_prebuilt_clang_dir_for $OS)
PROFDATA=qemu.profdata
if [ ! -z "$PROFRAWS" ]; then
    echo "Generating code coverage report"
    run $CLANG_BINDIR/llvm-profdata merge -sparse *.profraw -o $PROFDATA
    COV_OBJS=$(ls *.profraw | awk -F '.profraw' '{print "--object="$1}')
    $CLANG_BINDIR/llvm-cov export --format=lcov --instr-profile=$PROFDATA $COV_OBJS > lcov
    echo "Generated coverage file $OPT_OUT/lcov"
fi

cd $OLD_DIR

EMULATOR_CHECK=$OPT_OUT/emulator-check
if [ ! -f "$EMULATOR_CHECK" ]; then
    panic "    - FAIL: $EMULATOR_CHECK is missing!"
fi

# Check that we can run emulator-check
log "Checking emulator-check, this will produce some output"
$EMULATOR_CHECK accel || panic "This should provide hypervisor information!"
log "Success, android studio can probe the emulator accelerator config."

if [ "$OPT_SKIP_EMULATOR_CHECK" ] ; then
    log "Skipping check for 'emulator' launcher program."
else
    log "Checking for 'emulator' launcher program."
    EMULATOR=$OPT_OUT/emulator
    if [ ! -f "$EMULATOR" ]; then
        panic "    - FAIL: $EMULATOR is missing!"
    fi

    log "Checking that 'emulator' is a $EXPECTED_EMULATOR_BITNESS-bit program."
    EMULATOR_FILE_TYPE=$(get_file_type "$EMULATOR")
    if ! check_file_type_substring "$EMULATOR_FILE_TYPE" "$EXPECTED_EMULATOR_FILE_TYPE"; then
        warn "    - FAIL: $EMULATOR is not a 32-bit executable!"
        warn "        File type: $EMULATOR_FILE_TYPE"
        warn "        Expected : $EXPECTED_EMULATOR_FILE_TYPE"

        panic "emulator-bitness-check failed"
    fi
fi

if [ "$TARGET_OS" = "darwin-x86_64" ]; then
    log "*** Skipping library presence check ***"
    log2 "In macOS Big Sur >11.0.1, the system ships with a built-in dynamic
          linker cache of all system-provided libraries. As part of this change,
          copies of dynamic libraries are no longer present on the filesystem.
          Code that attempts to check for dynamic library presence by looking for a
          file at a path or enumerating a directory will fail. Instead, check for
          library presence by attempting to dlopen() the path, which will correctly check
    for the library in the cache."
fi

if [ "$TARGET_OS" = "linux-x86_64" ]; then
    log "Checking that all the .so dependencies are met"
    if [ -d $OPT_OUT/distribution ]; then
        log "Checking that linux binaries have all needed dependencies in the lib64 dir"
        # Make sure we can load all dependencies of every dylib/executable we have.
        cache=$(ldconfig --print-cache | awk '{ print $1; }')
        files=$(find $OPT_OUT/distribution \( -type f -and \( -executable -or -name '*.so.*' \) \))
        for file in $files; do
            log2 "Checking $file for dependencies on ld path, or our tree.."
            case "$file" in
                *"libprotobuf-lite"*)
                    panic "$file should not be released alongside libprotobuf.so"
            esac
            needed=$(readelf -d $file | grep "Shared" | cut -d "[" -f2 | cut -d "]" -f1)
            for need in $needed; do
                libs=$(find $OPT_OUT/distribution/emulator/lib64 -name $need);
                if [ $libs ]; then
                    log2 "  Found $need in our release"
                else
                    ldpath=$(contains "$cache" $need)
                    if [ $ldpath = "True" ]; then
                        log2 "  -- Found $need in default ld_library_path (os dependency)"
                        if [ $OPT_DEBS ]; then
                            log2 "  -- Discovering which packages provide $file with $need"
                            actual=$(ldd $file | grep $need | cut -d " " -f3)
                            if [ $actual ]; then
                                wanted=$(dpkg -S $actual | cut -d " " -f1 | cut -d ":" -f1)
                                log "apt-get install $wanted"
                            fi;
                        fi
                    else
                        panic "Unable to locate $need, needed by $file"
                    fi
                fi
            done
        done
        log "Dependencies are looking good!"
    else
        warn "No release found in $OPT_OUT, not validating dependencies."
    fi
fi

# Check the gen-entries.py script.
if [ "$RUN_GEN_ENTRIES_TESTS" ]; then
    log "Running gen-entries.py test suite."
    cd ${QEMU2_TOP_DIR}
    run ./android/scripts/tests/gen-entries/run-tests.sh ||
    panic "Failed gen-entries_tests"
    cd $OLD_DIR
fi

exit 0
