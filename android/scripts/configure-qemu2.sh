#!/bin/sh

PROGDIR=$(dirname "$0")
. $PROGDIR/utils/common.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/option_parser.shi
shell_import utils/package_builder.shi

case $(uname -s) in
    Linux) BUILD_OS=linux;;
    Darwin) BUILD_OS=darwin;;
    *) panic "Unsupported build system: $(uname -s)"
esac

OPT_CACHE_DIR=
option_register_var "--cache-dir=<dir>" OPT_CACHE_DIR \
    "Specify config cache directory."

QEMU2_SRC_DIR=$PROGDIR/../../../qemu-android
OPT_QEMU2_SRC_DIR=
option_register_var "--qemu2-src-dir=<dir>" OPT_QEMU2_SRC_DIR \
    "Specify QEMU2 source directory"

BUILD_PREBUILTS_DIR=$PROGDIR/../../../../prebuilts/android-emulator-build
OPT_BUILD_PREBUILTS_DIR=
option_register_var "--prebuilts-dir=<dir>" OPT_BUILD_PREBUILTS_DIR \
    "Specify emulator build prebuilts directory"

OPT_GENERATE=
option_register_var "--generate" OPT_GENERATE \
    "Run raw 'configure' script then cache configuration"

OPT_BUILD_DIR=
option_register_var "--build-dir=<dir>" OPT_BUILD_DIR \
    "Specify temporary build directory."

case $BUILD_OS in
    linux) VALID_SYSTEMS="linux windows";;
    darwin) VALID_SYSTEMS="darwin";;
esac
OPT_SYSTEM=$BUILD_OS
option_register_var "--system=<names>" OPT_SYSTEM \
    "Specify host system to target"

PROGRAM_PARAMETERS="<build-dir>"

PROGRAM_DESCRIPTION=\
"A small script used to speed-up the QEMU2 'configure' step by using a
cached version of the results."

option_parse "$@"

if [ "$PARAMETER_COUNT" != "1" ]; then
    panic "This script takes one argument. See --help."
fi
DST_DIR=$PARAMETER_1

BUILD_QEMU2=$PROGDIR/build-qemu-android.sh
if [ ! -f "$BUILD_QEMU2" ]; then
    panic "Missing $BUILD_QEMU2 script"
fi

if [ "$OPT_QEMU2_SRC_DIR" ]; then
    QEMU2_SRC_DIR=$OPT_QEMU2_SRC_DIR
else
    log "Auto-config: --qemu2-src-dir=$QEMU2_SRC_DIR"
fi
if [ ! -d "$QEMU2_SRC_DIR" ]; then
    panic "Missing QEMU2 source directory: $QEMU2_SRC_DIR"
fi
QEMU2_SRC_DIR=$(cd "$QEMU2_SRC_DIR" && pwd -P)

if [ "$OPT_BUILD_PREBUILTS_DIR" ]; then
    BUILD_PREBUILTS_DIR=$OPT_BUILD_PREBUILTS_DIR
else
    log "Auto-config: --prebuilts-dir=$BUILD_PREBUILTS_DIR"
fi
if [ ! -d "$BUILD_PREBUILTS_DIR" ]; then
    panic "Missing Emulator build prebuilts directory: $BUILD_PREBUILTS_DIR"
fi
BUILD_PREBUILTS_DIR=$(cd "$BUILD_PREBUILTS_DIR" && pwd -P)

if [ "$OPT_BUILD_DIR" ]; then
    TEMP_BUILD_DIR="$OPT_BUILD_DIR"
else
    TEMP_BUILD_DIR=/tmp/aaa
fi

if [ "$OPT_CACHE_DIR" ]; then
    CACHE_DIR=$OPT_CACHE_DIR
else
    CACHE_DIR=$PROGDIR/qemu2-config-cache
    log "Auto-config: --cache-dir=$CACHE_DIR"
fi

if [ "$OPT_SYSTEM" ]; then
    IS_VALID=
    for SYSTEM in $VALID_SYSTEMS; do
        if [ "$SYSTEM" = "$OPT_SYSTEM" ]; then
            IS_VALID=true
            break
        fi
    done
    if [ -z "$IS_VALID" ]; then
        panic "Invalid --system value [$OPT_SYSTEM], use one of: $VALID_SYSTEMS"
    fi
    HOST_OS=$OPT_SYSTEM
else
    HOST_OS=$BUILD_OS
    log "Auto-config: --host=$HOST_SYSTEMS"
fi

case $HOST_OS in
    linux)
        HOST_SYSTEMS="linux-x86,linux-x86_64"
        ;;
    windows)
        HOST_SYSTEMS="windows-x86,windows-x86_64"
        ;;
    darwin)
        HOST_SYSTEMS="darwin-x86_64"
        ;;
    *)
        panic "Unsupported system [$HOST_OS]"
        ;;
esac

# Run the QEMU2 'configure' script and cache the result.
# $1: Cache destination directory.
gen_configure_cache_dir () {
    local BUILD_FILES BUILD_LINKS
    local CACHE_DIR="$1"

    log "Configuring QEMU2 build."
    $BUILD_QEMU2 --config-only --build-dir=$CACHE_DIR --host=$HOST_SYSTEMS ||
        panic "Could not rebuild QEMU2 !"

    # Remove un-necessary files.
    rm -rf "$CACHE_DIR"/build-*/env.sh
    rm -rf "$CACHE_DIR"/build-*/toolchain-wrapper

    # For all build files, replace $QEMU2_SRC_DIR with @SRC_DIR@, and
    # $BUILD_PREBUILTS_DIR with @PREBUILTS_DIR@
    BUILD_FILES=$(find "$CACHE_DIR" -type f)
    echo "$BUILD_FILES" | xargs sed -i "" \
            -e 's|'$(regex_escape "$QEMU2_SRC_DIR")'|@SRC_DIR@|g'
    echo "$BUILD_FILES" | xargs sed -i "" \
            -e 's|'$(regex_escape "$BUILD_PREBUILTS_DIR")'|@PREBUILTS_DIR@|g'

    # For all build links, replace $QEMU2_SRC_DIR by @SRC_DIR@ too.
    BUILD_LINKS=$(find "$CACHE_DIR" -type l)
    echo "$BUILD_LINKS" | while read LINK; do
        DST=$(readlink $LINK)
        NEW_DST=$(printf "%s" "$DST" | subst "$QEMU2_SRC_DIR" "@SRC_DIR@")
        ln -sf $NEW_DST $LINK
    done
}

# Generate a configured build directory from cache.
# $1: Source directory, containing cached build file templates.
# $2: Destination directory, will contain a copy of the files in $1 with
#     paths adjusted
gen_configure_dir_from_cache () {
    local SRC_DIR DST_DIR SRC_FILES SRC_LINKS SRC DST_FILE LINK DST_LINK
    SRC_DIR="$1"
    DST_DIR="$2"
    SRC_FILES=$(find "$SRC_DIR" -type f | subst "$SRC_DIR/" "")
    for SRC in $SRC_FILES; do
        DST_FILE=$DST_DIR/$SRC
        mkdir -p $(dirname "$DST_FILE")
        cat "$SRC_DIR/$SRC" | subst "@SRC_DIR@" "$QEMU2_SRC_DIR" | \
            subst "@PREBUILTS_DIR@" "$BUILD_PREBUILTS_DIR" > "$DST_FILE"
    done
    
    SRC_LINKS=$(find "$SRC_DIR" -type l | subst "$SRC_DIR/" "")
    for LINK in $SRC_LINKS; do
        DST_LINK=$(readlink "$SRC_DIR"/$LINK | \
            subst "@SRC_DIR@" "$QEMU2_SRC_DIR")
        mkdir -p $(dirname "$DST_DIR"/$LINK)
        ln -sf "$DST_LINK" "$DST_DIR"/$LINK
    done
}

for SYSTEM in $(commas_to_spaces $HOST_SYSTEMS); do
    if [ "$OPT_GENERATE" ]; then
        mkdir -p "$CACHE_DIR/$SYSTEM"
        gen_configure_cache_dir "$CACHE_DIR/$SYSTEM"
    fi

    mkdir -p "$DST_DIR"
    gen_configure_dir_from_cache "$CACHE_DIR/$SYSTEM" "$DST_DIR"
done
