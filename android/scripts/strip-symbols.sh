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
"Strips all the binaries"

OPT_OUT=objs
option_register_var "--out-dir=<dir>" OPT_OUT "Use specific output directory"

OPT_MINGW=
option_register_var "--mingw" OPT_MINGW "Use mingw symbols stripper"

aosp_dir_register_option
option_parse "$@"
aosp_dir_parse_option

log_invocation

# Import the packagee builder
shell_import utils/package_builder.shi

# Build breakpad symbols from unstripped binaries
# Builds a symbols directory at the destination root dir and populates it with
# *.sym extension symbol files mirroring the binary file paths
#
# Expects Breakpad prebuilt to be built, panics otherwise.
# $1: Src root directory
# $2: Target root directory
# $3: Host target
# $4+: List of binary paths relative to Src root
build_symbols () {
    local SRC_ROOT="$1"
    local DEST_ROOT="$2"
    local HOST="$3"
    shift
    shift
    shift
    local BINARY_FILES="$@"
    local SYMBOL_DIR="$DEST_ROOT"/symbols
    local DSYM_DIR="$DEST_ROOT"/debug_info
    local BINARY_FULLNAME=
    local BINARY_DIR=
    local SYMTOOL="$(aosp_prebuilt_breakpad_dump_syms_for $HOST)"
    if [ ! -f "$SYMTOOL" ]; then
        panic "Breakpad prebuilt symbol dumper could not be found at $SYMTOOL"
    fi
    run mkdir -p "$SYMBOL_DIR"
    for BINARY in $BINARY_FILES; do
        if [ -L "$SRC_ROOT"/$BINARY ] || [ ! -f "$SRC_ROOT"/$BINARY ]; then
            log "Skipping $BINARY"
            continue
        fi
        BINARY_FULLNAME="$(basename "$BINARY")"
        BINARY_DIR="$(dirname "$BINARY")"
        silent_run mkdir -p $SYMBOL_DIR/$BINARY_DIR
        silent_run mkdir -p $DSYM_DIR/$BINARY_DIR
        log "Processing symbols for: ${BINARY}"
        if [ "$(get_build_os)" = "darwin" ]; then
            DSYM_BIN_DIR=$DSYM_DIR/$BINARY.dsym
            mkdir -p $DSYM_BIN_DIR
            log2 dsymutil --out=$DSYM_BIN_DIR $SRC_ROOT/$BINARY
            log2 $SYMTOOL -g $DSYM_BIN_DIR $SRC_ROOT/$BINARY > $SYMBOL_DIR/$BINARY.sym 2> $SYMBOL_DIR/$BINARY.err

            dsymutil --out=$DSYM_BIN_DIR $SRC_ROOT/$BINARY 2>&1 | grep -q 'no debug symbols' ||
            strip -S $DEST_ROOT/$LIB || panic "Could not strip $DEST_ROOT" &&
            $SYMTOOL -g $DSYM_BIN_DIR $SRC_ROOT/$BINARY > $SYMBOL_DIR/$BINARY.sym 2> $SYMBOL_DIR/$BINARY.err ||
                echo "Failed to create symbol file for $SRC_ROOT/$BINARY"
        else
            case $HOST in
                linux*)
                    OBJCOPY=$OPT_OUT/toolchain/x86_64-linux-objcopy
                    ;;
                windows*)
                    OBJCOPY=$OPT_OUT/toolchain/x86_64-w64-mingw32-objcopy
                    ;;
            esac
            cp -f $SRC_ROOT/$BINARY $DSYM_DIR/$BINARY
            $SYMTOOL $SRC_ROOT/$BINARY > $SYMBOL_DIR/$BINARY.sym 2> $SYMBOL_DIR/$BINARY.err ||
                panic "Failed to create symbol file for $SRC_ROOT/$BINARY"
            run "$OBJCOPY" --strip-unneeded $SRC_ROOT/$BINARY || \
                panic "Could not strip $SRC_ROOT/$BINARY"
        fi
    done
}


OLD_DIR=$PWD
cd $OPT_OUT
case $(get_build_os) in
    linux)
        ROOT=$(find . -maxdepth 1 -type f -executable | sed "s|\./||")
        LIBS=$(find lib64 -type f -executable | sed "s|\./||")
        QEMU=$(find qemu -type f -executable | sed "s|\./||")
        EXES="$ROOT $LIBS $QEMU"
        OPT_HOST=linux-x86_64
        ;;
    darwin)
        ROOT=$(find . -maxdepth 1 -type f -perm +111 | sed "s|\./||")
        LIBS=$(find lib64 -type f -perm +111 | sed "s|\./||")
        QEMU=$(find qemu -type f -perm +111 | sed "s|\./||")
        EXES="$ROOT $LIBS $QEMU"
        OPT_HOST=darwin-x86_64
        ;;
    *)
        panic "Unable to process binaries on this system [$(get_build_os)]"
        ;;
esac

if [ "$OPT_MINGW" ]; then
    OPT_HOST=windows-x86_64
fi


cd $OLD_DIR
# Build the symbols, and strip them.
build_symbols $OPT_OUT $OPT_OUT/build $OPT_HOST $EXES
