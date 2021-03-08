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
"Build prebuilt bluez for Linux"

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/bluez"

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

install_simple_package() {
  # Installs a package that does not need any special configuration
  local PKG="$1"
  local FLAGS="$2"
  dump "$(builder_text) Building ${PKG}"
  builder_unpack_package_source ${PKG}
  builder_build_autotools_package ${PKG} ${FLAGS}

}

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        case $SYSTEM in
        linux-x86_64)
            ;;
        linux-aarch64)
            ;;
        *)
            echo "${RED}Ignoring unsupported system ${SYSTEM}.${RESET}"
            continue
            ;;
        esac

        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        # Simple dependencies
        for dep in readline expat dbus libffi
        do
         install_simple_package $dep
        done


        install_simple_package  ncurses "--without-progs \
              --without-manpages \
              --without-tests \
              --with-shared"

        install_simple_package glib "\
              --disable-always-build-tests \
              --disable-compile-warnings \
              --disable-debug \
              --disable-dependency-tracking \
              --disable-fam \
              --disable-included-printf \
              --disable-installed-tests \
              --disable-libelf \
              --disable-man \
              --disable-selinux \
              --disable-xattr \
              --enable-static"


        # Bluez will eventually start linking against readline
        # and will need to link against ncurses
        export LIBS=-lncurses

        install_simple_package bluez "\
                --enable-static \
                --enable-pie \
                --enable-threads \
                --enable-library \
                --enable-testing \
                --disable-udev \
                --disable-obex \
                --disable-systemd" \

        # Copy binaries necessary for the build itself as well as static
        # libraries.
        copy_directory \
                "$(builder_install_prefix)/lib" \
                "$INSTALL_DIR/$SYSTEM/lib"

        copy_directory \
                "$(builder_install_prefix)/include" \
                "$INSTALL_DIR/$SYSTEM/include"

    ) || panic "[$SYSTEM] Could not build bluez!"

done

log "Done building bluez."
