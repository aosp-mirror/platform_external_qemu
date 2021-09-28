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

OPT_DEBS=
option_register_var "--debs" OPT_DEBS "Discover the debian package dependencies, needed to launch the emulator (Debian/Ubuntu only)."

option_register_var "--skip-emulator-check" OPT_SKIP_EMULATOR_CHECK "Skip emulator check"

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
EXE_SUFFIX=
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
case $TARGET_OS in
    windows*)
        EXE_SUFFIX=.exe
        ;;
esac

# Define EXPECTED_32BIT_FILE_TYPE and EXPECTED_64BIT_FILE_TYPE depending
# on the current target platform. Then EXPECTED_EMULATOR_BITNESS and
# EXPECTED_EMULATOR_FILE_TYPE accordingly.
case $TARGET_OS in
    windows_msvc*)
        EXPECTED_64BIT_FILE_TYPE="PE32\+ executable \(console\) x86-64"
        EXPECTED_EMULATOR_BITNESS=64
        EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
        EMUGEN_DIR=android/android-emugl/emugen/src/emugen-build/
        ;;
    windows*)
        EXPECTED_64BIT_FILE_TYPE="PE32\+ executable \(console\) x86-64"
        EXPECTED_EMULATOR_BITNESS=64
        EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_32BIT_FILE_TYPE
        EMUGEN_DIR=android/android-emugl/emugen/src/emugen-build/
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
        echo "FAIL: Unsupported target platform: [$TARGET_OS]"
        FAILURES="$FAILURES out-dir-config-make"
        ;;
esac


export CTEST_OUTPUT_ON_FAILURE=1
OLD_DIR=$PWD
cd $OPT_OUT
${CTEST} -j ${NUM_JOBS}  --output-on-failure || ${CTEST} --rerun-failed --output-on-failure || FAILURES="$FAILURES unittests"
cd ..

if [ "$OPT_SKIP_EMULATOR_CHECK" ] ; then
    log "Skipping check for 'emulator' launcher program."
else
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
fi

if [ "$TARGET_OS" = "darwin-x86_64" ]; then
#   log "Checking that darwin binaries target OSX $OSX_DEPLOYMENT_TARGET"
#   for EXEC in $EXECUTABLES; do
#       MIN_VERSION=$(darwin_min_version "$OPT_OUT/$EXEC")
#       if [ "$MIN_VERSION" != "$OSX_DEPLOYMENT_TARGET" ]; then
#           echo "   - WARN: $EXEC targets [$MIN_VERSION], expected [$OSX_DEPLOYMENT_TARGET]"
#           FAILURES="$FAILURES $EXEC-darwin-target"
#       fi
#   done
    # Let's make sure all our dependencies exist in a release.. So we don't fall over
    # during launch.
    if [ -d $OPT_OUT/gradle-release ]; then
        log "Checking that darwin binaries have all needed dependencies in the lib64 dir"
        # Make sure we can load all dependencies of every dylib/executable we have.
        find $OPT_OUT/gradle-release \( -type f -and \( -perm +111 -or -name '*.dylib' \) \) -print0 | while read -d $'\0' file; do
            log2 "Checking $file for dependencies"
            # We start looking at the 3rd line, as we
            needed=$(otool -L $file | tail -n +3 | awk '{print $1}')
            for need in $needed; do
                log2 "  Looking for $need"
                case $need in
                    @rpath/*)
                        # We will accept an rpath it if we can find it under lib64/
                        dylib="${need#@rpath/}"
                        libs=$(find $OPT_OUT/gradle-release/lib64 -name $dylib);
                        # Xcode interjects a bogus path, so we skip that one
                        if [ -z $libs ] && [ ! $dylib = "libclang_rt.asan_osx_dynamic.dylib" ]; then
                            panic "Unable to locate $need [$dylib], needed by $file"
                        fi
                        ;;
                    *)
                        # Should be on the system path..
                        test -f $need || panic "Unable to locate $need, needed by $file"
                        ;;
                esac
            done
        done
    else
        log "$OPT_OUT/gradle-release not found, not checking dependencies"
    fi
fi

if [ "$TARGET_OS" = "linux-x86_64" ]; then
   log "Checking that all the .so dependencies are met"
   if [ -d $OPT_OUT/gradle-release ]; then
        log "Checking that linux binaries have all needed dependencies in the lib64 dir"
        # Make sure we can load all dependencies of every dylib/executable we have.
        cache=$(ldconfig --print-cache | awk '{ print $1; }')
        files=$(find $OPT_OUT/gradle-release \( -type f -and \( -executable -or -name '*.so.*' \) \))
        for file in $files; do
            log2 "Checking $file for dependencies on ld path, or our tree.."
            needed=$(readelf -d $file | grep "Shared" | cut -d "[" -f2 | cut -d "]" -f1)
            for need in $needed; do
                libs=$(find $OPT_OUT/gradle-release/lib64 -name $need);
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
        FAILURES="$FAILURES gen-entries_tests"
    cd $OLD_DIR
fi

case "TARGET_OS" in

    windows*)
        # Check that the windows executables all have icons.
        # First need to locate the windres tool.
        log "Checking windows executables icons."
        if [ ! -f "$CONFIG_MAKE" ]; then
            echo "FAIL: Could not find \$CONFIG_MAKE !?"
            FAILURES="$FAILURES out-dir-config-make"
        else
            WINDRES=$SDK_TOOLCHAIN_DIR/${BINPREFIX}windres
            if [ ! -f $WINRES ]; then
                echo "FAIL: Could not find host 'windres' program"
                FAILURES="$FAILURES host-windres"
            fi
            EXPECTED_ICONS=14
            for EXEC in $EXECUTABLES; do
                EXEC=${EXEC}.exe
                if [ ! -f "$OPT_OUT"/$EXEC ]; then
                    warn "FAIL: Missing windows executable: $EXEC"
                    FAILURES="$FAILURES $TARGET_OS-$EXEC"
                else
                    NUM_ICONS=$($WINDRES --input-format=coff --output-format=rc $OPT_OUT/$EXEC | grep RT_ICON | wc -l)
                    if [ "$NUM_ICONS" != "$EXPECTED_ICONS" ]; then
                        warn "FAIL: Invalid icon count in $EXEC ($NUM_ICONS, expected $EXPECTED_ICONS)"
                        FAILURES="$FAILURES $TARGET_OS-$EXEC-icons"
                    fi
                fi
            done
        fi
        ;;
esac
if [ "$FAILURES" ]; then
    panic "Unit test failures: ${RED}$FAILURES${RESET}"
fi

