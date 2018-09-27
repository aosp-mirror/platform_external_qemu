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
VERBOSE=1

MINGW=
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
            MINGW=true
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
            VERBOSE=2
            HELP=y
            ;;
        --debug)
            OPTDEBUG=true
            ;;
    esac
done

panic () {
    # don't print error message if we just came back from printing the help message
   if [ -z "$HELP" ]; then
       echo "ERROR: $@"
   fi
   exit 1
}

run () {
    if [ "$VERBOSE" -gt 2 ]; then
        echo "COMMAND: $@"
        "$@"
    elif [ "$VERBOSE" -gt 1 ]; then
        "$@"
    else
        "$@" >/dev/null 2>&1
    fi
}

log () {
    if [ "$VERBOSE" -gt 1 ]; then
        printf "%s\n" "$*"
    fi
}

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

# Let's not remove the build directory when someone asks for help
if [ -z "$HELP" ]; then
    cd "$PROGDIR"/..
    rm -rf "$OUT_DIR"
fi
# If the user only wants to print the help message and exit
# there is no point of printing Configuring Build
if [ "$VERBOSE" -ne 2 ]; then
    echo "Configuring build."
fi
export IN_ANDROID_REBUILD_SH=1
run android/configure.sh --out-dir=$OUT_DIR "$@" $OPT_CLANG ||
    panic "Configuration error, please run ./android/configure.sh to see why."

CONFIG_MAKE=$OUT_DIR/build/config.make
if [ ! -f "$CONFIG_MAKE" ]; then
    panic "Cannot find: $CONFIG_MAKE"
fi

echo "Building sources."
run make -j$HOST_NUM_CPUS BUILD_OBJS_DIR="$OUT_DIR" ||
    panic "Could not build sources, please run 'make' to see why."


if [ -z "$NO_TESTS" ]; then
    # Let's not run all the tests parallel..
   run android/scripts/run_tests.sh --verbose --verbose --out-dir=$OUT_DIR --jobs=1
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
