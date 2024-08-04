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
PROGDIR=$(dirname $0)
AOSP_DIR=$PROGDIR/../../..

# Return the build machine's operating system tag.
# Valid return values are:
#    linux
#    darwin
#    freebsd
#    windows   (really MSys)
#    cygwin
get_build_os () {
    if [ -z "$_SHU_BUILD_OS" ]; then
        _SHU_BUILD_OS=$(uname -s)
        case $_SHU_BUILD_OS in
            Darwin)
                _SHU_BUILD_OS=darwin
                ;;
            FreeBSD)  # note: this is not tested
                _SHU_BUILD_OS=freebsd
                ;;
            Linux)
                # note that building  32-bit binaries on x86_64 is handled later
                _SHU_BUILD_OS=linux
                ;;
            CYGWIN*|*_NT-*)
                _SHU_BUILD_OS=windows
                if [ "x$OSTYPE" = xcygwin ] ; then
                    _SHU_BUILD_OS=cygwin
                fi
                ;;
        esac
    fi
    echo "$_SHU_BUILD_OS"
}

aosp_find_python() {
    local AOSP_PREBUILTS_DIR=$AOSP_DIR/prebuilts
    local OS_NAME=$(get_build_os)
    local PYTHON=$AOSP_PREBUILTS_DIR/python/$OS_NAME-x86/bin/python3
    $PYTHON --version >/dev/null || panic "Unable to get python version from $PYTHON"
    printf "$PYTHON"
}

PYTHON=$(aosp_find_python)
$PYTHON $PROGDIR/build/python/cmake.py --ccache auto "$@"
