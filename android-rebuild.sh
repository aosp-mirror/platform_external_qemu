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
for OPT; do
    case $OPT in
        --mingw)
            MINGW=true
            ;;
        --verbose)
            VERBOSE=$(( $VERBOSE + 1 ))
            ;;
        --help|-?)
            VERBOSE=2
            ;;
    esac
done

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
run ./android-configure.sh "$@"

echo "Building sources."
run make -j$HOST_NUM_CPUS

RUN_64BIT_TESTS=true

TEST_SHELL=
EXE_SUFFIX=
if [ "$MINGW" ]; then
  RUN_64BIT_TESTS=false
  TEST_SHELL=wine
  EXE_SUFFIX=.exe
fi

echo "Running 32-bit unit test suite."
run $TEST_SHELL objs/emulator_unittests$EXE_SUFFIX
if [ "$RUN_64BIT_TESTS" ]; then
    echo "Running 64-bit unit test suite."
    run $TEST_SHELL objs/emulator64_unittests$EXE_SUFFIX
fi

echo "Done. !!"
