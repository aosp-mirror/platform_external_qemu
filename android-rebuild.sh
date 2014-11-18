#!/bin/bash
#
# this script is used to rebuild all QEMU binaries for the host
# platforms.
#
# assume that the device tree is in TOP
#

set -e
export LANG=C
export LC_ALL=C

VERBOSE=0

MINGW=
NO_TESTS=
OUT_DIR=objs

for OPT; do
    case $OPT in
        --mingw)
            MINGW=true
            ;;
        --verbose)
            VERBOSE=$(( $VERBOSE + 1 ))
            ;;
        --no-tests)
            NO_TESTS=true
            ;;
        --out-dir=*)
            OUT_DIR=${OPT##--out-dir=}
            ;;
        --help|-?)
            VERBOSE=2
            ;;
    esac
done

panic () {
    echo "ERROR: $@"
    exit 1
}

run () {
  if [ "$VERBOSE" -ge 1 ]; then
    "$@"
  else
    "$@" >/dev/null 2>&1
  fi
}

HOST_OS=$(uname -s)
case $HOST_OS in
    Linux)
        HOST_NUM_CPUS=`cat /proc/cpuinfo | grep processor | wc -l`
        ;;
    Darwin|FreeBsd)
        HOST_NUM_CPUS=`sysctl -n hw.ncpu`
        ;;
    CYGWIN*|*_NT-*)
        HOST_NUM_CPUS=$NUMBER_OF_PROCESSORS
        ;;
    *)  # let's play safe here
        HOST_NUM_CPUS=1
esac


# Return the type of a given file, using the /usr/bin/file command.
# $1: executable path.
# Out: file type string, or empty if the path is wrong.
get_file_type () {
    /usr/bin/file "$1" 2>/dev/null
}

# Return true iff the file type string |$1| contains the expected
# substring |$2|.
# $1: executable file type
# $2: expected file type substring
check_file_type_substring () {
    printf "%s\n" "$1" | grep -q -E -e "$2"
}

# Define EXPECTED_32BIT_FILE_TYPE and EXPECTED_64BIT_FILE_TYPE depending
# on the current target platform. Then EXPECTED_EMULATOR_BITNESS and
# EXPECTED_EMULATOR_FILE_TYPE accordingly.
if [ "$MINGW" ]; then
    EXPECTED_32BIT_FILE_TYPE="PE32 executable \(console\) Intel 80386"
    EXPECTED_64BIT_FILE_TYPE="PE32\+ executable \(console\) x86-64"
    EXPECTED_EMULATOR_BITNESS=32
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_32BIT_FILE_TYPE
elif [ "$HOST_OS" = "Darwin" ]; then
    EXPECTED_32BIT_FILE_TYPE="Mach-O executable i386"
    EXPECTED_64BIT_FILE_TYPE="Mach-O 64-bit executable x86_64"
    EXPECTED_EMULATOR_BITNESS=64
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_64BIT_FILE_TYPE
else
    EXPECTED_32BIT_FILE_TYPE="ELF 32-bit LSB +executable, Intel 80386"
    EXPECTED_64BIT_FILE_TYPE="ELF 32-bit LSB +executable, x86-64"
    EXPECTED_EMULATOR_BITNESS=32
    EXPECTED_EMULATOR_FILE_TYPE=$EXPECTED_32BIT_FILE_TYPE
fi

# Build the binaries from sources.
cd `dirname $0`
rm -rf objs
echo "Configuring build."
run ./android-configure.sh --out-dir=$OUT_DIR "$@" ||
    panic "Configuration error, please run ./android-configure.sh to see why."

echo "Building sources."
run make -j$HOST_NUM_CPUS OBJS_DIR="$OUT_DIR" ||
    panic "Could not build sources, please run 'make' to see why."

RUN_32BIT_TESTS=
RUN_64BIT_TESTS=true

TEST_SHELL=
EXE_SUFFIX=
if [ "$MINGW" ]; then
  TEST_SHELL=wine
  EXE_SUFFIX=.exe

  # Check for Wine on this machine.
  WINE_CMD=$(which $TEST_SHELL 2>/dev/null || true)
  if [ -z "$NO_TESTS" -a -z "$WINE_CMD" ]; then
    echo "WARNING: Wine is not installed on this machine!! Unit tests will be ignored!!"
    NO_TESTS=true
  fi

  RUN_32BIT_TESTS=true
fi

if [ "$HOST_OS" = "Linux" ]; then
    # Linux 32-bit binaries are deprecated but not removed yet!
    RUN_32BIT_TESTS=true
fi


if [ -z "$NO_TESTS" ]; then
    FAILURES=""

    echo "Checking for 'emulator' launcher program."
    EMULATOR=$OUT_DIR/emulator$EXE_SUFFIX
    if [ ! -f "$EMULATOR" ]; then
        echo "    - FAIL: $EMULATOR is missing!"
        FAILURES="$FAILURES emulator"
    fi

    echo "Checking that 'emulator' is a $EXPECTED_EMULATOR_BITNESS-bit program."
    EMULATOR_FILE_TYPE=$(get_file_type "$EMULATOR")
    if ! check_file_type_substring "$EMULATOR_FILE_TYPE" "$EXPECTED_EMULATOR_FILE_TYPE"; then
        echo "    - FAIL: $EMULATOR is not a 32-bit executable!"
        echo "        File type: $EMULATOR_FILE_TYPE"
        echo "        Expected : $EXPECTED_EMULATOR_FILE_TYPE"
        FAILURES="$FAILURES emulator-bitness-check"
    fi

    if [ "$RUN_32BIT_TESTS" ]; then
        echo "Running 32-bit unit test suite."
        for UNIT_TEST in emulator_unittests emugl_common_host_unittests; do
        echo "   - $UNIT_TEST"
        run $TEST_SHELL $OUT_DIR/$UNIT_TEST$EXE_SUFFIX || FAILURES="$FAILURES $UNIT_TEST"
        done
    fi

    if [ "$RUN_64BIT_TESTS" ]; then
        echo "Running 64-bit unit test suite."
        for UNIT_TEST in emulator64_unittests emugl64_common_host_unittests; do
            echo "   - $UNIT_TEST"
            run $TEST_SHELL $OUT_DIR/$UNIT_TEST$EXE_SUFFIX || FAILURES="$FAILURES $UNIT_TEST"
        done
    fi

    if [ "$FAILURES" ]; then
        panic "Unit test failures: $FAILURES"
    fi
else
    echo "Ignoring unit tests suite."
fi

echo "Done. !!"
