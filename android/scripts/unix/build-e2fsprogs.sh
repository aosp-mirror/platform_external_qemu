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

# This script cannot build windows-x86_64 binaries - only 32-bit support
# is achievable as the 64-bit distribution of e2fsprogs requires a newer
# version of a dll dependency than is available.  The available dependencies
# (version 1.42.7) do not export the ext2fs_close_free function that was
# added to a later version of e2fsprogs (at version 1.42.11), and no early
# enough executable version of e2fsprogs is readily available from Cygwin.
# case $(get_build_os)-$(get_build_arch) in
#     linux-x86_64)
#         DEFAULT_HOST_SYSTEMS="linux-x86_64,windows-x86"
#         ;;
#     linux-aarch64)
#         DEFAULT_HOST_SYSTEMS="linux-aarch64"
#         ;;
#     darwin-aarch64)
#         DEFAULT_HOST_SYSTEMS="darwin-aarch64"
#         ;;
#     *)
#         panic "Unsupported build system : [$(get_build_os)-$(get_build_arch)]"
#         ;;
# esac


PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build prebuilt e2fsprogs for Linux and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option common/e2fsprogs

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options e2fsprogs
package_builder_parse_package_list

# For windows we have already downloaded the executables
# so just uncompress them to the correct directory in
# preparation for android/rebuild.sh.
# $1: Destination directory of dependencies
WINDOWS_DEPENDENCIES="e2fsprogs-windows cygwin libcom_err2\
    libe2p2 libblkid1 libuuid1 libext2fs2 libgcc1 libiconv2 libintl8"
unpack_windows_dependencies () {
    local DEP DSTDIR
    DSTDIR=$1
    for DEP in $WINDOWS_DEPENDENCIES; do
        run mkdir -p "$(builder_src_dir)/$(package_list_get_unpack_src_dir $DEP)"
        builder_unpack_package_source "$DEP"
    done

    copy_directory_files \
                        "$(builder_src_dir)/usr/bin" \
                        "$DSTDIR/sbin" \
                        cygblkid-1.dll \
                        cygcom_err-2.dll \
                        cyge2p-2.dll \
                        cygext2fs-2.dll \
                        cyggcc_s-1.dll \
                        cygiconv-2.dll \
                        cygintl-8.dll \
                        cyguuid-1.dll \
                        cygwin1.dll

    copy_directory_files \
                        "$(builder_src_dir)/usr" \
                        "$DSTDIR" \
                        sbin/e2fsck.exe \
                        sbin/resize2fs.exe \
                        sbin/tune2fs.exe
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote e2fsprogs build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        dump "$(builder_text) Building e2fsprogs"

        CONFIGURE_FLAGS=
        var_append CONFIGURE_FLAGS \
                --disable-nls \
                --disable-defrag \
                --disable-jbd-debug \
                --disable-profile \
                --disable-testio-debug \
                --disable-rpath \

        case $SYSTEM in
            windows-x86)
                unpack_windows_dependencies "$INSTALL_DIR/$SYSTEM"
                ;;
            windows-x86_64)
                dump "WARNING: windows-x86_64 isn't supported with this script!"
                ;;
            *)
                builder_unpack_package_source e2fsprogs

                builder_build_autotools_package_full_install e2fsprogs \
                        "install install-libs" \
                        $CONFIGURE_FLAGS \
                        'CFLAGS=-g -O2 -fpic' \

                # Copy binaries necessary for the build itself as well as static
                # libraries.
                copy_directory_files \
                        "$(builder_install_prefix)" \
                        "$INSTALL_DIR/$SYSTEM" \
                        sbin/e2fsck \
                        sbin/fsck.ext4 \
                        sbin/mkfs.ext4 \
                        sbin/resize2fs \
                        sbin/tune2fs \

                # Copy the libuuid files on linux. Mac has its own libuuid.
                case $SYSTEM in
                    linux*)
                        copy_directory_files \
                            "$(builder_install_prefix)" \
                            "$INSTALL_DIR/$SYSTEM" \
                            include/uuid/uuid.h \
                            lib/libuuid.a
                    ;;
                esac
                ;;
        esac


    ) || panic "[$SYSTEM] Could not build e2fsprogs!"

done

log "Done building e2fsprogs."
