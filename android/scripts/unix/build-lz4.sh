#!/bin/sh

# Copyright 2017 The Android Open Source Project
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
"Build prebuilt lz4 for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/lz4"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options lz4
package_builder_parse_package_list

# Build a given autotools based package and run customized install commands
#
# $1: Package basename
# $2: Make command for building
# $3: Make command for installing
build_make_package() {
    local PKG_NAME=$(package_list_get_src_dir $1)
    local PKG_SRC_DIR=$(builder_src_dir)/$PKG_NAME
    local PKG_BUILD_DIR=$(builder_build_dir)/$PKG_NAME
    local PKG_TIMESTAMP=${PKG_BUILD_DIR}-timestamp
    local BUILD_COMMAND=$2
    local INSTALL_COMMAND=$3
    if [ -z "$INSTALL_COMMAND" ]; then
        INSTALL_COMMAND="install"
    fi

    if [ -f "$PKG_TIMESTAMP" -a -z "$OPT_FORCE" ]; then
        # Return early if the package was already built.
        return 0
    fi

    case $_SHU_BUILDER_CURRENT_HOST in
        darwin*)
            # Required for proper Autotools builds on Darwin
            builder_disable_verbose_install
            ;;
    esac

    local PKG_FULLNAME="$(basename $PKG_NAME)"
    dump "$(builder_text) Building $PKG_FULLNAME"

    local INSTALL_FLAGS
    if [ -z "$_SHU_BUILDER_DISABLE_PARALLEL_INSTALL" ]; then
        var_append INSTALL_FLAGS "-j$NUM_JOBS"
    fi
    if [ -z "$_SHU_BUILDER_DISABLE_VERBOSE_INSTALL" ]; then
        var_append INSTALL_FLAGS "V=1";
    fi
    (
        run mkdir -p "$PKG_BUILD_DIR" &&
        run cd "$PKG_BUILD_DIR" &&
        export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
        export CFLAGS="-O3 -g -fpic" &&
        export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
        export PREFIX=$(builder_install_prefix) &&
        export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
        export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&
        run make clean -C "$PKG_SRC_DIR" &&
        run make "$BUILD_COMMAND" -C "$PKG_SRC_DIR" -j$NUM_JOBS V=1 &&
        run make "$INSTALL_COMMAND" -C "$PKG_SRC_DIR" $INSTALL_FLAGS
    ) ||
    panic "Could not build and install $PKG_FULLNAME"

    touch "$PKG_TIMESTAMP"
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote lz4 build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        dump "$(builder_text) Building lz4"

        builder_unpack_package_source lz4

        build_make_package lz4 "lib" "install"

        # Copy binaries necessary for the build itself as well as static
        # libraries.
        copy_directory_files \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM" \
                lib/liblz4.a

        copy_directory \
                "$(builder_install_prefix)/include/" \
                "$INSTALL_DIR/$SYSTEM/include/"

    ) || panic "[$SYSTEM] Could not build lz4!"

done

log "Done building lz4."
