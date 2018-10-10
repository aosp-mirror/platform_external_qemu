#!/bin/sh
#
# this script is used to rebuild all QEMU binaries for the host
# platforms.
#
# assume that the device tree is in TOP
#

set -e
export LANG=C
export LC_ALL=C

PROGDIR=$(dirname "$0")
. $(dirname "$0")/scripts/utils/common.shi
VERBOSE=1

MINGW=
WINDOWS_MSVC=
NO_TESTS=
OUT_DIR=objs
HELP=


for OPT; do
    case $OPT in
        --sdk-build-number=*)
            ANDROID_SDK_TOOLS_BUILD_NUMBER=${OPT##--sdk-build-number=}
            ;;
        --aosp-prebuilts-dir=*)
            ANDROID_EMULATOR_PREBUILTS_DIR=${OPT##--aosp-prebuilts-dir=}
            ;;
        --mingw)
            if [ "$WINDOWS_MSVC" ]; then
              panic "Choose either mingw or windows-msvc, not both."
            fi
            MINGW=true
            ;;
        --windows-msvc)
            if [ "$MINGW" ]; then
              panic "Choose either mingw or windows-msvc, not both."
            fi
            WINDOWS_MSVC=true
            ;;
        --verbose)
            VERBOSE=$(( $VERBOSE + 1 ))
            ;;
        --verbosity=*)
            VERBOSE=${OPT##--verbosity=}
            ;;
        --no-tests)
            NO_TESTS=true
            ;;
        --out-dir=*)
            OUT_DIR=${OPT##--out-dir=}
            ;;
        --help|-?)
            android/scripts/config-cmake.sh  --help
            exit 0
            ;;
        --debug)
            OPTDEBUG=true
            ;;
    esac
done


HOST_OS=$(uname -s)
case $HOST_OS in
    Linux)
        HOST_NUM_CPUS=`cat /proc/cpuinfo | grep processor | wc -l`
        ;;
    Darwin|FreeBSD)
        HOST_NUM_CPUS=`sysctl -n hw.ncpu`
        ;;
    CYGWIN*|*_NT-*)
        HOST_NUM_CPUS=$NUMBER_OF_PROCESSORS
        ;;
    *)  # let's play safe here
        HOST_NUM_CPUS=1
esac


log_invocation

$PROGDIR/scripts/config-cmake.sh  "$@" ||
    panic "Configuration error, please run ./android/scripts/config-cmake.sh to see why."



OLD_DIR=$PWD
cd  $OUT_DIR
if [ -f build.ninja ]; then
    echo "Building sources with ninja."
    ninja || panic "Could not build sources, please run 'cd $OUT_DIR && ninja' to see why."
else
    echo "Building sources with make."
    make -j$HOST_NUM_CPUS ||
        panic "Could not build sources, please run 'cd $OUT_DIR && make' to see why."
fi

echo "Finished building sources."
cd $OLD_DIR

if [ -z "$NO_TESTS" ]; then
    # Let's not run all the tests parallel..
   android/scripts/run_tests.sh --verbose --verbose --out-dir=$OUT_DIR --jobs=1
else
    echo "Ignoring unit tests suite."
fi

echo "Done. !!"

if [ "$OPTDEBUG" = "true" ]; then
    overrides=`cat android/asan_overrides`
    echo "Debug build enabled."
    echo "ASAN may be in use; recommend disabling some ASAN checks as build is not"
    echo "universally ASANified. This can be done with"
    echo ""
    echo ". android/envsetup.sh"
    echo ""
    echo "or export ASAN_OPTIONS=$overrides"
fi
