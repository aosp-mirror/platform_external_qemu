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
"Build prebuilt curl for Linux, Windows and Darwin."

package_builder_register_options

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option curl

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options curl
package_builder_parse_package_list

# Handle zlib, only on Win32 because the zlib configure script
# doesn't know how to generate a static library with -fPIC!
build_windows_zlib_package () {
    local ZLIB_VERSION ZLIB_PACKAGE
    local LOC LDFLAGS
    local PREFIX=$(builder_install_prefix)
    local BUILD_DIR=$(builder_build_dir)
    case $(builder_host) in
        windows-x86_64)
            LOC=-m64
            LDFLAGS=-m64
            ;;
    esac
    ZLIB_VERSION=$(package_list_get_version zlib)
    dump "$(builder_text) Building zlib-$ZLIB_VERSION"
    ZLIB_PACKAGE=$(package_list_get_filename zlib)
    unpack_archive "$(builder_archive_dir)/$ZLIB_PACKAGE" "$BUILD_DIR"
    (
        run cd "$BUILD_DIR/zlib-$ZLIB_VERSION" &&
        export PKG_CONFIG_PATH=$(builder_install_prefix)/lib/pkgconfig &&
        export BINARY_PATH=$PREFIX/bin &&
        export INCLUDE_PATH=$PREFIX/include &&
        export LIBRARY_PATH=$PREFIX/lib &&
        run make -fwin32/Makefile.gcc install \
                PREFIX=$(builder_gnu_config_host_prefix) \
                LOC=$LOC \
                LDFLAGS=$LDFLAGS &&
        unset PKG_CONFIG_PATH &&
        unset CROSS_PREFIX
    )
}

# Build a package zlib for Linux and Mac OS X
#
build_zlib_package () {
    local ZLIB_VERSION ZLIB_PACKAGE
    local LOC LDFLAGS
    local BUILD_DIR=$(builder_build_dir)
    case $(builder_host) in
        *-x86)
            LOC=-m32
            LDFLAGS=-m32
            ;;
        *-x86_64)
            LOC=-m64
            LDFLAGS=-m64
            ;;
    esac
    ZLIB_VERSION=$(package_list_get_version zlib)
    dump "$(builder_text) Building zlib-$ZLIB_VERSION"
    ZLIB_PACKAGE=$(package_list_get_filename zlib)
    unpack_archive "$(builder_archive_dir)/$ZLIB_PACKAGE" "$BUILD_DIR"
    (
        run cd "$BUILD_DIR/zlib-$ZLIB_VERSION" &&
        export PKG_CONFIG_PATH=$(builder_install_prefix)/lib/pkgconfig &&
        export CROSS_PREFIX=$(builder_gnu_config_host_prefix) &&
        run ./configure --prefix=$(builder_install_prefix) -fPIC &&
        run make -j$NUM_JOBS &&
        run make install &&
        unset PKG_CONFIG_PATH &&
        unset CROSS_PREFIX
    )
}

# $1+: Configuration options.
build_package_openssl () {
    # Unpack package source into $(builder_src_dir) if needed.
    local PKG_SRCD_NAME=$(package_list_get_unpack_src_dir "openssl")
    local PKG_SRC_TIMESTAMP=$(builder_src_dir)/timestamp-${PKG_SRCD_NAME}
    if [ ! -f "$PKG_SRC_TIMESTAMP" ]; then
      package_list_unpack_and_patch \
              "openssl" "$(builder_archive_dir)" "$(builder_src_dir)"
      touch $PKG_SRC_TIMESTAMP
    fi

    shift
    local PKG_SRC_DIR="$(builder_src_dir)/$PKG_SRCD_NAME"
    local PKG_BUILD_DIR=$(builder_build_dir)/$PKG_SRCD_NAME
    local PKG_BLD_TIMESTAMP=$(builder_build_dir)/$PKG_SRCD_NAME-timestamp
    if [ ! -f "$PKG_BLD_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
      case $SYSTEM in
        darwin*)
          # Required for proper build on Darwin!
          builder_disable_verbose_install
          ;;
      esac
      # build openssl package
      local PKG_FULLNAME="$(basename "${PKG_SRC_DIR}")"
      dump "$(builder_text) Building $PKG_FULLNAME"

      local CONFIG_FLAGS
      local INSTALL_FLAGS

      case $(builder_host) in
        linux-x86_64)
          CONFIG_FLAGS="linux-x86_64 -fPIC"
          ;;
        windows-x86_64)
          CONFIG_FLAGS="mingw64"
          ;;
        darwin-*)
          # NOTE: '-fPIC -fno-common' is required to build PIE-compatible
          # static libraries, i.e. one that can be linked into a .dylib.
          CONFIG_FLAGS="darwin64-x86_64-cc -fPIC -fno-common"
          ;;
        *)
          panic "Host system '$(builder_host)' is not supported by this script!"
          ;;
      esac

      # NOTE: Parallel builds sometimes fail on Darwin and Linux.
      # local NUM_JOBS=1

      (
        run mkdir -p "$PKG_BUILD_DIR" &&
        run cd "$PKG_SRC_DIR" &&
        export LDFLAGS="-L$_SHU_BUILDER_PREFIX/lib" &&
        export CPPFLAGS="-I$_SHU_BUILDER_PREFIX/include" &&
        export PKG_CONFIG_LIBDIR="$_SHU_BUILDER_PREFIX/lib/pkgconfig" &&
        export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR:$_SHU_BUILDER_PKG_CONFIG_PATH" &&
        run "$PKG_SRC_DIR"/Configure \
          $CONFIG_FLAGS \
          -no-shared \
          --prefix=$_SHU_BUILDER_PREFIX \
          --openssldir=$_SHU_BUILDER_PREFIX \
          --cross-compile-prefix=$(builder_gnu_config_host_prefix) \
          "$@" &&
        run make MAKEFLAGS=-j${NUM_JOBS} &&
        run make install_sw $INSTALL_FLAGS &&
        run make clean
        # install_sw skips man, etc, shortenning build time
      ) ||
      panic "Could not build and install $PKG_FULLNAME"
      # success
      touch "$PKG_BLD_TIMESTAMP"
    fi
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote curl build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    (
        builder_prepare_for_host "$SYSTEM" "$AOSP_DIR"

        case $SYSTEM in
            windows-*)
                build_windows_zlib_package
                ;;
            *)
                build_zlib_package
                ;;
        esac


        dump "$(builder_text) Building openssl"

        build_package_openssl

    ) || panic "[$SYSTEM] Could not build openssl!"

done

log "Done building openssl."
