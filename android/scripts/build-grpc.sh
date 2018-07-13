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

# how to use:
#  android/scripts/build-bluez.sh --host=linux-x86_64 --verbose --force
#  android/scripts/build-bluez.sh --host=linux-x86 --verbose --force
#

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt grpc for Linux, Windows and Darwin"

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option grpc

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options bluez
package_builder_parse_package_list

# Fancy colors
RED=`tput setaf 1`
RESET=`tput sgr0`

install_grpc() {
    # Installs a package that does not need any special configuration
    local PKG=grpc
    local FLAGS=
    dump "$(builder_text) Building ${PKG}"
    local ARCHIVE_DIR=$(builder_archive_dir)
    local PKG_NAME=$(package_list_get_src_dir ${PKG})
    local INSTALL_COMMANDS="install"
    local PKG_SRC_DIR=$(builder_src_dir)/$PKG_NAME
    local PKG_BUILD_DIR=$(builder_build_dir)/$PKG_NAME
    local PKG_TIMESTAMP=${PKG_BUILD_DIR}-timestamp

    package_list_unpack_and_patch "${PKG}" "$ARCHIVE_DIR" "$(builder_build_dir)"

    shift; shift;

    if [ -f "$PKG_TIMESTAMP" -a -z "$OPT_FORCE" ]; then
        # Return early if the package was already built.
        return 0
    fi


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
    run cd "$PKG_BUILD_DIR" &&
        export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
        export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
        export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
        export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&
        run make $INSTALL_FLAGS &&
        copy_directory \
        "${PKG_BUILD_DIR}/bins/opt/" \
        "$INSTALL_DIR/$SYSTEM/bin" &&
        copy_directory \
        "${PKG_BUILD_DIR}/libs/opt/" \
        "$INSTALL_DIR/$SYSTEM/lib" &&
        copy_directory \
        "${PKG_BUILD_DIR}/include/" \
        "$INSTALL_DIR/$SYSTEM/include"
    ) ||
        panic "Could not build and install $PKG_FULLNAME"

    touch "$PKG_TIMESTAMP"
}


for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        case $SYSTEM in
        linux-x86_64)
          LIBCPLUSPLUS=$($(program_directory)/gen-android-sdk-toolchain.sh \
            --host=$_SHU_BUILDER_CURRENT_HOST \
            --print=libcplusplus unused_parameter)
          export LD_LIBRARY_PATH=$(dirname $LIBCPLUSPLUS)
          ;;
        windows-*)
            dump "Not needed!"
            ;;
        esac

       install_grpc
    ) || panic "[$SYSTEM] Could not build grpc!"

done

log "Done building grpc."
