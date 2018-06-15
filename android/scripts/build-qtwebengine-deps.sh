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
"Build qtwebengine dependencies for Linux"

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option "common/qtwebengine-deps"

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options qtwebengine-deps
package_builder_parse_package_list

# Fancy colors
RED=`tput setaf 1`
RESET=`tput sgr0`

install_simple_package() {
  # Installs a package that does not need any special configuration
  local PKG=$1
  local FLAGS=$2
  dump "$(builder_text) Building ${PKG}"
  builder_unpack_package_source ${PKG}
  builder_build_autotools_package ${PKG} ${FLAGS}
}

install_simple_package_autogen() {
  # Installs a package that does not need any special configuration
  local PKG=$1
  local FLAGS=$2
  dump "$(builder_text) Building ${PKG}"
  builder_unpack_package_source ${PKG}
  builder_build_autotools_package_autogen ${PKG} ${FLAGS}
}

install_nss_package() {
  builder_unpack_package_source nss

  local PKG_NAME=$(package_list_get_src_dir nss)
  local PKG_SRC_DIR=$(builder_src_dir)/$PKG_NAME
  local PKG_BUILD_DIR=$(builder_build_dir)/$PKG_NAME
  local PKG_TIMESTAMP=${PKG_BUILD_DIR}-timestamp

  if [ -f "$PKG_TIMESTAMP" -a -z "$OPT_FORCE" ]; then
      # Return early if the package was already built.
      return 0
  fi

  local PKG_FULLNAME="$(basename $PKG_NAME)"
  dump "$(builder_text) Building $PKG_FULLNAME"

  local INSTALL_FLAGS
  var_append INSTALL_FLAGS "-j1"

  if [ -z "$_SHU_BUILDER_DISABLE_VERBOSE_INSTALL" ]; then
      var_append INSTALL_FLAGS "V=1";
  fi
  (
      run mkdir -p "$PKG_BUILD_DIR" &&
      run cd "$PKG_SRC_DIR" &&
      export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
      echo "LDFLAGS=$LDFLAGS" &&
      export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
      echo "CPPFLAGS=$CPPFLAGS" &&
      export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
      echo "PKG_CONFIG_LIBDIR=$PKG_CONFIG_LIBDIR" &&
      export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&
      echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH" &&
      run make -j1 V=1 \
          USE_64=1 \
          NSPR_INCLUDE_DIR="$(builder_install_prefix)/include/nspr" \
          NSPR_LIB_DIR="$(builder_install_prefix)/lib" \
          NSS_DISABLE_GTESTS=1 \
          PREFIX=$(builder_install_prefix) &&

      # install to the install directory
      run cd "../dist" &&
      run install -v -m755 Linux*/lib/*.so "$(builder_install_prefix)/lib" &&
      run install -v -m644 Linux*/lib/{*.chk,libcrmf.a} "$(builder_install_prefix)/lib" &&
      run install -v -m755 -d "$(builder_install_prefix)/include/nss" &&
      run cp -v -RL {public,private}/nss/* "$(builder_install_prefix)/include/nss" &&
      run chmod -v 644 $(builder_install_prefix)/include/nss/* &&
      run install -v -m755 Linux*/bin/{certutil,nss-config,pk12util} "$(builder_install_prefix)/bin" &&
      run install -v -m644 Linux*/lib/pkgconfig/nss.pc "$(builder_install_prefix)/lib/pkgconfig"
  ) ||
  panic "Could not build and install $PKG_FULLNAME"

  touch "$PKG_TIMESTAMP"
}

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        case $SYSTEM in
        linux-x86_64)
            ;;
        linux-x86)
            ;;
        *)
            echo "${RED}Ignoring unsupported system ${SYSTEM}.${RESET}"
            continue
            ;;
        esac

        builder_prepare_for_host_no_binprefix "$SYSTEM" "$AOSP_DIR"

        # Simple dependencies
        for dep in expat dbus freetype
        do
         install_simple_package $dep
        done

        install_simple_package_autogen macros

        export ACLOCAL="aclocal -I $(builder_install_prefix)/share/aclocal"

        for dep in xproto libpthread-stubs
        do
         install_simple_package $dep
        done

        for dep in kbproto fixesproto xextproto compositeproto \
                   libXcomposite renderproto libXrender libXcursor \
                   randrproto libXrandr inputproto recordproto libXtst \
                   damageproto libXdamage
        do
          install_simple_package_autogen $dep
        done

        install_simple_package nspr \
                --enable-64bit

        CONFIGURE_FLAGS=
        var_append CONFIGURE_FLAGS \
                --disable-nls \
                --disable-defrag \
                --disable-jbd-debug \
                --disable-profile \
                --disable-testio-debug \
                --disable-rpath \

        builder_unpack_package_source e2fsprogs

        builder_build_autotools_package_full_install e2fsprogs \
                "install install-libs" \
                $CONFIGURE_FLAGS \
                'CFLAGS=-g -O2 -fpic' \

        install_simple_package fontconfig

        # build and install nss package
        install_nss_package

        # Copy binaries necessary for the build itself as well as static
        # libraries.
        copy_directory \
                "$(builder_install_prefix)/lib" \
                "$INSTALL_DIR/$SYSTEM/lib"

        copy_directory \
                "$(builder_install_prefix)/include" \
                "$INSTALL_DIR/$SYSTEM/include"

        copy_directory \
                "$(builder_install_prefix)/bin" \
                "$INSTALL_DIR/$SYSTEM/bin"

    ) || panic "[$SYSTEM] Could not build qtwebengine dependencies!"

done

log "Done building qtwebengine dependencies."

