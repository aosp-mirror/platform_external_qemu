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
PROGDIR=`dirname $0`

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

cd $PROGDIR/../..

QEMU2_TOP_DIR=${AOSP_DIR}/external/qemu
HOST_OS=$(get_build_os)
CONFIG_MAKE=${OPT_OUT}/build/config.make
# Extract the target os from config.make.
TARGET_OS=$(grep BUILD_TARGET_OS ${CONFIG_MAKE} | awk '{ print $3; }')
FAILURES=""
EXE_SUFFIX=
OSX_DEPLOYMENT_TARGET=10.8
RUN_EMUGEN_TESTS=true
RUN_GEN_ENTRIES_TESTS=true


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
EXECUTABLES="emulator emulator64-arm emulator64-x86"
if [ "$TARGET_OS" = "windows" ]; then
    EXE_SUFFIX=.exe
    EXECUTABLES="$EXECUTABLES emulator-arm emulator-x86"
fi

# Define EXPECTED_32BIT_FILE_TYPE and EXPECTED_64BIT_FILE_TYPE depending
# on the current target platform. Then EXPECTED_EMULATOR_BITNESS and
# EXPECTED_EMULATOR_FILE_TYPE accordingly.
if [ "$TARGET_OS" = "windows" ]; then
    EXPECTED_32BIT_FILE_TYPE="PE32 executable \(console\) Intel 80386"
    EXPECTED_64BIT_FILE_TYPE="PE32\+ executable \(console\) x86-64"
    EXPECTED_EMULATOR_BITNESS=32
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_32BIT_FILE_TYPE
    TARGET_OS=windows
elif [ "$TARGET_OS" = "darwin" ]; then
    EXPECTED_64BIT_FILE_TYPE="Mach-O 64-bit executable x86_64"
    EXPECTED_EMULATOR_BITNESS=64
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
    TARGET_OS=darwin
else
    EXPECTED_64BIT_FILE_TYPE="ELF 64-bit LSB +executable, x86-64"
    EXPECTED_EMULATOR_BITNESS=64
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
    TARGET_OS=linux
fi


run make tests BUILD_OBJS_DIR="$OPT_OUT" -j${NUM_JOBS} ||
    FAILURES="$FAILURES unittests"

log "Checking for 'emulator' launcher program."
EMULATOR=$OPT_OUT/emulator$EXE_SUFFIX
if [ ! -f "$EMULATOR" ]; then
    warn "    - FAIL: $EMULATOR is missing!"
    FAILURES="$FAILURES emulator"
fi

log "Checking that 'emulator' is a $EXPECTED_EMULATOR_BITNESS-bit program."
EMULATOR_FILE_TYPE=$(get_file_type "$EMULATOR")
if ! check_file_type_substring "$EMULATOR_FILE_TYPE" "$EXPECTED_EMULATOR_FILE_TYPE"; then
    warn "    - FAIL: $EMULATOR is not a 32-bit executable!"
    warn "        File type: $EMULATOR_FILE_TYPE"
    warn "        Expected : $EXPECTED_EMULATOR_FILE_TYPE"
    FAILURES="$FAILURES emulator-bitness-check"
fi

if [ "$HOST_OS" = "darwin" ]; then
    log "Checking that darwin binaries target OSX $OSX_DEPLOYMENT_TARGET"
    for EXEC in $EXECUTABLES; do
        MIN_VERSION=$(darwin_min_version "$OPT_OUT/$EXEC")
        if [ "$MIN_VERSION" != "$OSX_DEPLOYMENT_TARGET" ]; then
            echo "   - FAIL: $EXEC targets [$MIN_VERSION], expected [$OSX_DEPLOYMENT_TARGET]"
            FAILURES="$FAILURES $EXEC-darwin-target"
        fi
    done
fi


if [ "$RUN_EMUGEN_TESTS" ]; then
    EMUGEN_UNITTESTS=$OPT_OUT/emugen_unittests
    if [ ! -f "$EMUGEN_UNITTESTS" ]; then
        warn "FAIL: Missing binary: $EMUGEN_UNITTESTS"
        FAILURES="$FAILURES emugen_unittests-binary"
    else
        log "Running emugen_unittests."
        run $EMUGEN_UNITTESTS ||
            FAILURES="$FAILURES emugen_unittests"
    fi
    log "Running emugen regression test suite."
    # Note that the binary is always built for the 'build' machine type,
    # I.e. if --mingw is used, it's still a Linux executable.
    EMUGEN=$OPT_OUT/emugen
    if [ ! -f "$EMUGEN" ]; then
        echo "FAIL: Missing 'emugen' binary: $EMUGEN"
        FAILURES="$FAILURES emugen-binary"
    else
        # The first case is for a remote build with package-release.sh
        TEST_SCRIPT=$PROGDIR/../../opengl/host/tools/emugen/tests/run-tests.sh
        if [ ! -f "$TEST_SCRIPT" ]; then
            # This is the usual location.
            TEST_SCRIPT=$QEMU2_TOP_DIR/android/android-emugl/host/tools/emugen/tests/run-tests.sh
        fi
        if [ ! -f "$TEST_SCRIPT" ]; then
            echo " FAIL: Missing script: $TEST_SCRIPT"
            FAILURES="$FAILURES emugen-test-script"
        else
            run $TEST_SCRIPT --emugen=$EMUGEN ||
                FAILURES="$FAILURES emugen-test-suite"
        fi
    fi
fi

# Check the gen-entries.py script.
if [ "$RUN_GEN_ENTRIES_TESTS" ]; then
    log "Running gen-entries.py test suite."
    run android/scripts/tests/gen-entries/run-tests.sh ||
        FAILURES="$FAILURES gen-entries_tests"
fi

# Check that the windows executables all have icons.
# First need to locate the windres tool.
if [ "$TARGET_OS" = "windows" ]; then
    log "Checking windows executables icons."
    if [ ! -f "$CONFIG_MAKE" ]; then
        echo "FAIL: Could not find \$CONFIG_MAKE !?"
        FAILURES="$FAILURES out-dir-config-make"
    else
        WINDRES=$(awk '/^BUILD_TARGET_WINDRES:=/ { print $2; } $1 == "BUILD_TARGET_WINDRES" { print $3; }' $CONFIG_MAKE) ||
        if true; then
            echo "FAIL: Could not find host 'windres' program"
            FAILURES="$FAILURES host-windres"
        fi
        EXPECTED_ICONS=14
        for EXEC in $EXECUTABLES; do
            EXEC=${EXEC}.exe
            if [ ! -f "$OPT_OUT"/$EXEC ]; then
                warn "FAIL: Missing windows executable: $EXEC"
                FAILURES="$FAILURES windows-$EXEC"
            else
                NUM_ICONS=$($WINDRES --input-format=coff --output-format=rc $OPT_OUT/$EXEC | grep RT_ICON | wc -l)
                if [ "$NUM_ICONS" != "$EXPECTED_ICONS" ]; then
                    warn "FAIL: Invalid icon count in $EXEC ($NUM_ICONS, expected $EXPECTED_ICONS)"
                    FAILURES="$FAILURES windows-$EXEC-icons"
                fi
            fi
        done
    fi
fi
if [ "$FAILURES" ]; then
    panic "Unit test failures: ${RED}$FAILURES${RESET}"
fi

