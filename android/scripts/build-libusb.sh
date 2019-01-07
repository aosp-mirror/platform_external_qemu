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
#  android/scripts/build-libusb.sh --host=linux-x86_64 --verbose --force
#

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/install_dir.shi
shell_import utils/option_parser.shi
shell_import utils/package_list_parser.shi
shell_import utils/package_builder.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION="Build prebuilt libusb Linux & Mac"

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/libusb"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
  panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options libusb
package_builder_parse_package_list


install_simple_package() {
  # Installs a package that does not need any special configuration
  local PKG=$1
  local FLAGS=$2
  dump "$(builder_text) Building ${PKG}"
  builder_unpack_package_source ${PKG}
  builder_build_autotools_package ${PKG} ${FLAGS}

}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote libusb build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
  (
  case $SYSTEM in
    linux-*)
      ;;
    darwin-*)
      ;;
    *)
      echo "Ignoring unsupported system ${SYSTEM}."
      continue
      ;;
  esac

  builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

  install_simple_package libusb "--disable-udev"

  copy_directory \
    "$(builder_install_prefix)/lib" \
    "$INSTALL_DIR/$SYSTEM/lib"

  copy_directory \
    "$(builder_install_prefix)/include" \
    "$INSTALL_DIR/$SYSTEM/include"

  ) || panic "[$SYSTEM] Could not build libusb!"

done

log "Done building libusb."
