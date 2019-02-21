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
"Build Mesa3D library dependencies if needed."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option mesa-deps

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options mesa-deps
package_builder_parse_package_list

if [ -z "$OPT_FORCE" ]; then
    builder_check_all_timestamps "$INSTALL_DIR" mesa-deps
fi

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote mesa-deps build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
        timestamp_set "$INSTALL_DIR/$SYSTEM" mesa-deps
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_enable_cxx11
        builder_prepare_for_host "$SYSTEM" "$AOSP_DIR" "$INSTALL_DIR/$SYSTEM"

        case $SYSTEM in
            linux-*)
                builder_unpack_package_source glproto
                builder_build_autotools_package glproto
                ;;

            windows-*)
                export LLVM_CROSS_COMPILING=1
                ;;
        esac

        case $SYSTEM in
            *-x86)
                LLVM_TARGETS=x86
                ;;
            *-x86_64)
                LLVM_TARGETS=x86_64
                ;;
            linux-aarch64)
                LLVM_TARGETS=aarch64
                ;;
            *)
                panic "Unknown system CPU architecture: $SYSTEM"
        esac

        builder_reset_configure_flags \
                --disable-shared \
                --with-pic

        builder_unpack_package_source llvm

        builder_build_autotools_package llvm \
                --disable-clang-arcmt \
                --disable-clang-plugin-support \
                --disable-clang-static-analyzer \
                --enable-optimized \
                --disable-assertions \
                --disable-libffi \
                --disable-docs \
                --disable-doxygen \
                --disable-timestamps \
                --enable-targets=$LLVM_TARGETS \
                --disable-bindings \

        for SUBDIR in include lib share/aclocal share/pkgconfig; do
            if [ -d "$(builder_install_prefix)/$SUBDIR" ]; then
                copy_directory \
                        "$(builder_install_prefix)/$SUBDIR" \
                        "$INSTALL_DIR/$SYSTEM/$SUBDIR"
            fi
        done

        run mkdir -p "$INSTALL_DIR/$SYSTEM/bin"
        case $SYSTEM in
            windows-*)
                # Create a dummy file to ensure that .../bin exists, since
                # the Mesa configure step will abort if it doesn't exist.
                echo "This file is required to build Mesa" > \
                        "$INSTALL_DIR/$SYSTEM/bin/DO_NOT_REMOVE"
                ;;
            *)
                # Copy llvm-config to .../bin.
                run cp -f "$(builder_install_prefix)/bin/llvm-config" \
                        "$INSTALL_DIR/$SYSTEM/bin/"
                ;;
        esac

        timestamp_set "$INSTALL_DIR/$SYSTEM" mesa-deps

    ) || panic "[$SYSTEM] Could not build Mesa dependencies!"

done

log "Done building mesa dependencies."
