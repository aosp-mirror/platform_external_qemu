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

# Build the binaries from sources.
cd `dirname $0`
rm -rf objs
echo "Configuring build."
run ./android-configure.sh --out-dir=$OUT_DIR "$@" ||
    panic "Configuration error, please run ./android-configure.sh to see why."

echo "Building sources."
run make -j$HOST_NUM_CPUS OBJS_DIR="$OUT_DIR" ||
    panic "Could not build sources, please run 'make' to see why."

RUN_64BIT_TESTS=true

TEST_SHELL=
EXE_SUFFIX=
if [ "$MINGW" ]; then
  RUN_64BIT_TESTS=
  TEST_SHELL=wine
  EXE_SUFFIX=.exe

  # Check for Wine on this machine.
  WINE_CMD=$(which $TEST_SHELL 2>/dev/null || true)
  if [ -z "$NO_TESTS" -a -z "$WINE_CMD" ]; then
    echo "WARNING: Wine is not installed on this machine!! Unit tests will be ignored!!"
    NO_TESTS=true
  fi
fi

if [ -z "$NO_TESTS" ]; then
    echo "Running 32-bit unit test suite."
    FAILURES=""
    for UNIT_TEST in emulator_unittests emugl_common_host_unittests; do
    echo "   - $UNIT_TEST"
    run $TEST_SHELL $OUT_DIR/$UNIT_TEST$EXE_SUFFIX || FAILURES="$FAILURES $UNIT_TEST"
    done

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
