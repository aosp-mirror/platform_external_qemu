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

# The posix way of getting the full path..
PROGDIR="$( cd "$(dirname "$0")" ; pwd -P )"
. $(dirname "$0")/scripts/utils/common.shi
VERBOSE=1

MINGW=
WINDOWS_MSVC=
FORCE_FETCH_WINTOOLCHAIN=
NO_TESTS=
OUT_DIR=objs
HELP=
OPT_INSTALL=
NO_QTWEBENGINE=true

for OPT; do
    case $OPT in
        --sdk-build-number=*)
            ANDROID_SDK_TOOLS_BUILD_NUMBER=${OPT##--sdk-build-number=}
            OPT_INSTALL=install
            ;;
        --aosp-prebuilts-dir=*)
            ANDROID_EMULATOR_PREBUILTS_DIR=${OPT##--aosp-prebuilts-dir=}
            OPT_INSTALL=install
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
        --force-fetch-wintoolchain)
            WINTOOLCHAIN=true
            ;;
        --with-qtwebnengine)
            NO_QTWEBENGINE=
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
        --symbols)
            OPT_SYMBOLS=true
            ;;
        --debug)
            OPTDEBUG=true
            ;;
    esac
done

NINJA=ninja
HOST_OS=$(uname -s)
case $HOST_OS in
    Linux)
        HOST_NUM_CPUS=`cat /proc/cpuinfo | grep processor | wc -l`
        NINJA=$PROGDIR/third_party/chromium/depot_tools/ninja
        # Make sure our ninja version is on the path, so cmake can actually generate
        # the needed files.
        PATH=$PROGDIR/third_party/chromium/depot_tools:$PATH
        ;;
    Darwin|FreeBSD)
        HOST_NUM_CPUS=`sysctl -n hw.ncpu`
        # Built in ninja should work on mac as well.
        NINJA=$PROGDIR/third_party/chromium/depot_tools/ninja
        # Make sure our ninja version is on the path, so cmake can actually generate
        # the needed files.
        PATH=$PROGDIR/third_party/chromium/depot_tools:$PATH
        BREAKPAD=${PROGDIR}/../../../prebuilts/android-emulator-build/common/breakpad/darwin-x86_64/bin/dump_syms
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



if [ -f $OUT_DIR/build.ninja ]; then
    echo "Building sources with ninja."
    $NINJA -C $OUT_DIR $OPT_INSTALL || panic "Could not build sourcis, please run 'ninja -C $OUT_DIR' to see why."
else
    echo "Building sources with make."
    make -C $OUT_DIR -j$HOST_NUM_CPUS $OPT_INSTALL ||
        panic "Could not build sources, please run 'make -C $OUT_DIR' to see why."
fi

echo "Finished building sources."

if [ -z "$NO_TESTS" ]; then
    # Let's not run all the tests parallel..
   $PROGDIR/scripts/run_tests.sh --verbose --verbose --out-dir=$OUT_DIR --jobs=1
else
    echo "Ignoring unit tests suite."
fi


# Strip out symbols if requested
if [ "$OPT_SYMBOLS" ]; then
    echo "Generating symbols"
    if [ "$MINGW" ]; then
       $PROGDIR/scripts/strip-symbols.sh --out-dir=$OUT_DIR --mingw --verbosity=$VERBOSE
    else
       $PROGDIR/scripts/strip-symbols.sh --out-dir=$OUT_DIR --verbosity=$VERBOSE
    fi
fi

echo "Done. !!"

if [ "$OPTDEBUG" = "true" ]; then
    overrides=$(cat ${PROGDIR}/asan_overrides)
    echo "Debug build enabled."
    echo "ASAN may be in use; recommend disabling some ASAN checks as build is not"
    echo "universally ASANified. This can be done with"
    echo ""
    echo ". android/envsetup.sh"
    echo ""
    echo "or export ASAN_OPTIONS=$overrides"
fi
