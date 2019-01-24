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

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION="Strips all the binaries"

OPT_OUT=objs
option_register_var "--out-dir=<dir>" OPT_OUT "Use specific output directory"


OPT_INSTALL=gradle-release
option_register_var "--install-dir=<dir>" OPT_INSTALL "CMake install directory used"

OPT_MINGW=
option_register_var "--mingw" OPT_MINGW "Use mingw symbols stripper"

aosp_dir_register_option
option_parse "$@"
aosp_dir_parse_option

log_invocation

if [ ! -d $OPT_OUT/$OPT_INSTALL ]; then
  panic "The install directory $OPT_OUT/$OPT_INSTALL does not exist."
fi

# Import the package builder
shell_import utils/package_builder.shi

# Build breakpad symbols from unstripped binaries
# Builds a symbols directory at the destination root dir and populates it with
# *.sym extension symbol files mirroring the binary file paths
#
# Expects Breakpad prebuilt to be built, panics otherwise.
# $1: Src root directory
# $2: Target root directory
# $3: Host target
# $4: The binary to generate the symbol for
build_symbols () {
    local DEST_ROOT="$1"
    local HOST="$2"
    local BINARY="$3"

    local SYMBOL_DIR="$DEST_ROOT/symbols"
    local DSYM_DIR="$DEST_ROOT/debug_info"
    local BINARY_FULLNAME=
    local BINARY_DIR=
    local SYMTOOL="$(aosp_prebuilt_breakpad_dump_syms_for $HOST)"

    if [ ! -f "$SYMTOOL" ]; then
        panic "Breakpad prebuilt symbol dumper could not be found at $SYMTOOL"
    fi
    silent_run mkdir -p "$SYMBOL_DIR"
    if [ -L "$BINARY" ] || [ ! -f "$BINARY" ]; then
        log "Skipping $BINARY"
        return
    fi
    BINARY_FULLNAME="$(basename "$BINARY")"
    BINARY_DIR="$(dirname "$BINARY")"

    # First we are going to create the directories (if needed)
    # where the symbols/debug information ends up, this is mostly
    # a nop, so let's not spam the logs.
    silent_run mkdir -p $SYMBOL_DIR/$BINARY_DIR
    silent_run mkdir -p $DSYM_DIR/$BINARY_DIR
    if [ "$(get_build_os)" = "darwin" ]; then
        # Darwin places symbols in a directory, APFS and HPFS will treat these
        # directories as packages. Be careful here, mac ships versions of these
        # filesystems that can be case insensitive (default). This is not the case
        # on our build bots
        DSYM_BIN_DIR=$DSYM_DIR/$BINARY.dSYM
        mkdir -p $DSYM_BIN_DIR
        # Extract the symbols
        run dsymutil --out=$DSYM_BIN_DIR $BINARY || panic "Unable to extract debug symbols"
        # Extract our breakpad information
        $SYMTOOL $BINARY > $SYMBOL_DIR/$BINARY.sym 2> $SYMBOL_DIR/$BINARY.err ||
            warn "Failed to create symbol file for $BINARY"
        # And strip the binary
        run strip -S $BINARY || panic "Could not strip $BINARY"A
    else
        case $HOST in
            linux*)
                OBJCOPY=toolchain/x86_64-linux-objcopy
                ;;
            windows*)
                OBJCOPY=toolchain/x86_64-w64-mingw32-objcopy
                ;;
        esac
        # On win/lin debug_info cannot be seperated from the executable.
        run cp -f $BINARY $DSYM_DIR/$BINARY
        # Generate the breakpad information
        $SYMTOOL $BINARY > $SYMBOL_DIR/$BINARY.sym 2> $SYMBOL_DIR/$BINARY.err ||
            warn "Failed to create symbol file for $BINARY"
        # And strip out the debug information
        run "$OBJCOPY" --strip-unneeded $BINARY || \
            warn "Could not strip $BINARY"
    fi
}

process_dir() {
    local DIR=$1
    case $(get_build_os) in
        linux)
            OPT_HOST=linux-x86_64
            if [ "$OPT_MINGW" ]; then
               OPT_HOST=windows-x86_64
               files=$(find $DIR -name \*.exe -or -name \*.dll)
            else
               files=$(find $DIR -type f -and \( -executable -or -name \*.so\* \))
            fi
            ;;
        darwin)
            OPT_HOST=darwin-x86_64
            files=$(find $DIR \( -type f -and \( -perm +111 -or -name '*.dylib' \) \))
            ;;
        *)
            panic "Unable to process binaries on this system [$(get_build_os)]"
            ;;
    esac
    log2 "Stripping ${files}"

    for file in $files; do
        line=$(echo $file| sed 's|\./||')
        build_symbols build $OPT_HOST "$line"
    done
}

warn "The cmake build is capable of stripping during release, there's no need to run this."
OLD_DIR=$PWD
run cd $OPT_OUT
process_dir $OPT_INSTALL
run cd $OLD_DIR
