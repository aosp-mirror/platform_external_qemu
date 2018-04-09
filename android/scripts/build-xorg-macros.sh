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
"Build prebuilt xorg-macros for Linux"

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/xorg-macros"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options util-macros
package_builder_parse_package_list

TARGET_ARCH="linux-x86_64"
builder_prepare_for_host_no_binprefix "$TARGET_ARCH" "$AOSP_DIR"

if [ "$(get_build_os)" != "linux" ]; then
  panic "Must build this on linux platform [$(get_build_os)]."
fi


dump "$(builder_text) Building xorg-macros"

builder_unpack_package_source macros

builder_build_autotools_package macros \
        --prefix=$_SHU_BUILDER_PREFIX

# Copy binaries necessary for the build itself as well as static
# libraries.
echo "builder_install_prefix=$(builder_install_prefix)"
echo "INSTALL_DIR=$INSTALL_DIR"

copy_directory \
        "$(builder_install_prefix)" \
        "$INSTALL_DIR/$TARGET_ARCH.new"

# Atomically update target directory $1 with the content of $2.
# This also removes $2 on success.
# $1: target directory.
# $2: source directory.
directory_atomic_update () {
    local DST_DIR="$1"
    local SRC_DIR="$2"
    if [ -d "$DST_DIR" ]; then
        run rm -f "$DST_DIR".old &&
        run mv "$DST_DIR" "$DST_DIR".old
    fi
    run mv "$SRC_DIR" "$DST_DIR" &&
    run rm -rf "$DST_DIR".old
}

directory_atomic_update \
        "$INSTALL_DIR/$TARGET_ARCH" \
        "$INSTALL_DIR/$TARGET_ARCH.new"

log "Done building xorg-macros."
