#!/bin/sh

# Copyright 2015 The Android Open Source Project
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

shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt ANGLE for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/ANGLE"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options ANGLE

# Clone a git repository, and checkout a specific branch & commit.
# $1: Source git URL.
# $2: Destination directory.
# $3: Branch / Commit
git_clone () {
    local GIT_URL DST_DIR BRANCH CHECK_REV
    GIT_URL=$1
    DST_DIR=$2
    BRANCH=$3

    if [ -d "$DST_DIR" ]; then
        panic "Git destination directory already exists: $DST_DIR"
    fi
    run git clone "$GIT_URL" "$DST_DIR" ||
            panic "Could not clone git repository: $GIT_URL"
    run git -C "$DST_DIR" checkout $BRANCH ||
            panic "Could not checkout $GIT_URL - $BRANCH"
}

# Build the ANGLE packages.
#
# $1: Source directory.
# $2: The directory the output libraries are placed in.
build_angle_package () {
    local PKG_FULLNAME="$(basename "$1")"
    dump "$(builder_text) Building $PKG_FULLNAME"

    local PKG_BUILD_DIR="$1"
    local PKG_LIB_DIR="$2"

    # Bolierplate setup
    run mkdir -p "$PKG_BUILD_DIR" &&
    run cd "$PKG_BUILD_DIR" &&
    log "ANGLE build: git init" &&

    export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
    export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
    export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
    export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&

    # Ensure the gclient command is available.
    log "ANGLE build: clone depot_tools" &&
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools &&
    export PATH=`pwd`/depot_tools:"$PATH" &&

    log "ANGLE build: run bootstrap script"
    # Create gclient file and sync the build files
    run python scripts/bootstrap.py &&

    # Sync the appropriate build files
    # The first sync fails, however the second will work
    log "ANGLE build: gclient sync" &&
    run gclient sync || run gclient sync &&

    log "ANGLE build: apply patch for linux-aarch64 (skipping for now)" &&
    # apply patch for linux-aarch64 host build
    # run patch -p1 -i "linux-aarch64/angle-depo_tools-ninja-aarch64.patch" &&

    log "ANGLE build: gn gen" &&
    run mkdir -p out/Debug &&
    echo "is_debug = false" >> out/Debug/args.gn &&
    echo "use_custom_libcxx = true" >> out/Debug/args.gn &&
    run gn gen out/Debug &&

    log "ANGLE build: actual build" &&
    # ninja is provided for each platform in the previously cloned depot_tools
    run ninja -C out/Debug &&

    log "ANGLE build: exit"
}

# Unpack package source into $(builder_src_dir) if needed.
# $1: Package basename.
copy_angle_source () {
    local PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_SRC_TIMESTAMP=$PKG_BUILD_DIR/timestamp-angle
    if [ ! -f "$PKG_SRC_TIMESTAMP" ]; then
        cp -r $AOSP_DIR/external/angle $PKG_BUILD_DIR
        rm -rf $PKG_BUILD_DIR/.git
        touch $PKG_SRC_TIMESTAMP
    fi
}

# $1: Package basename (e.g. 'libpthread-stubs-0.3')
# $2+: Extra configuration options.
build_angle_libraries () {
    local PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP PKG_LIB_DIR DYN_LIB_SUFFIX ST_LIB_NAME
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/angle
    copy_angle_source
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/angle-timestamp
    PKG_LIB_DIR=$PKG_BUILD_DIR/out/Debug
    ST_LIB_NAME=libshadertranslator

    # Windows: Not tested; this is just a rough guide to the build steps
    # on Windows, which should be the same as that for other platforms,
    # except for setting DEPOT_TOOLS_WIN_TOOLCHAIN=0
    case $SYSTEM in
        win*)
            DYN_LIB_SUFFIX=dll
            export DEPOT_TOOLS_WIN_TOOLCHAIN=0
            ;;
        linux*)
            DYN_LIB_SUFFIX=so
            ;;
        darwin*)
            DYN_LIB_SUFFIX=dylib
            ;;
    esac


    if [ ! -f "$PKG_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
        log "Beginning ANGLE build. Dyn lib suffix: ${DYN_LIB_SUFFIX}"

        build_angle_package \
            "$PKG_BUILD_DIR" \
            "$PKG_LIB_DIR" \
            "$@"

        log "Copying files"
        run ls $PKG_LIB_DIR

        # We don't have a make install command at all since the Makefile is
        # auto-generated, so we just copy the files we want directly.
        copy_directory_files \
                    "$PKG_LIB_DIR" \
                    "$(builder_install_prefix)/lib" \
                    libEGL.${DYN_LIB_SUFFIX} \
                    libGLESv2.${DYN_LIB_SUFFIX} \
                    libGLESv1_CM.${DYN_LIB_SUFFIX} \
                    ${ST_LIB_NAME}.${DYN_LIB_SUFFIX} \

        log "Copying includes"
        copy_directory_files \
                "$PKG_BUILD_DIR/src/libShaderTranslator" \
                "$(builder_install_prefix)/include" \
                ShaderTranslator.h

        touch "$PKG_TIMESTAMP"
    fi
}

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        build_angle_libraries

        # Copy binaries necessary for the build itself as well as static libraries.
        copy_directory \
                "$(builder_install_prefix)/" \
                "$INSTALL_DIR/$SYSTEM/"

        dump "[$SYSTEM] Done building ANGLE libraries"

    ) || panic "[$SYSTEM] Could not build ANGLE libraries!"

done

log "Done building ANGLE libraries."
