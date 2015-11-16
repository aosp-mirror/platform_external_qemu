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

ARCHIVE_DIR=$PREBUILTS_DIR/archive
if [ ! -d "$ARCHIVE_DIR" ]; then
    dump "Downloading dependencies sources first."
    $(program_directory)/download-sources.sh \
        --verbosity=$(get_verbosity) \
        --prebuilts-dir="$PREBUILTS_DIR" ||
            panic "Could not download source archives!"
fi
if [ ! -d "$ARCHIVE_DIR" ]; then
    panic "Missing archive directory: $ARCHIVE_DIR"
fi
PACKAGE_LIST=$ARCHIVE_DIR/PACKAGES.TXT
if [ ! -f "$PACKAGE_LIST" ]; then
    panic "Missing package list file, run download-sources.sh: $PACKAGE_LIST"
fi

package_builder_process_options curl
package_list_parse_file "$PACKAGE_LIST"

BUILD_SRC_DIR=$TEMP_DIR/src

# Unpack package source into $BUILD_SRC_DIR if needed.
# $1: Package basename.
unpack_package_source () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_unpack_src_dir $1)
    PKG_SRC_TIMESTAMP=$BUILD_SRC_DIR/timestamp-$PKG_NAME
    if [ ! -f "$PKG_SRC_TIMESTAMP" ]; then
        package_list_unpack_and_patch "$1" "$ARCHIVE_DIR" "$BUILD_SRC_DIR"
        touch $PKG_SRC_TIMESTAMP
    fi
}

# $1: Package basename (e.g. 'libpthread-stubs-0.3')
# $2+: Extra configuration options.
build_package () {
    local PKG_NAME PKG_SRC_DIR PKG_BUILD_DIR PKG_SRC_TIMESTAMP PKG_TIMESTAMP
    PKG_NAME=$(package_list_get_src_dir $1)
    unpack_package_source "$1"
    shift
    PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_NAME"
    PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_NAME
    PKG_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/$PKG_NAME-timestamp
    if [ ! -f "$PKG_TIMESTAMP" -o -n "$OPT_FORCE" ]; then
        case $SYSTEM in
            darwin*)
                # Required for proper build on Darwin!
                builder_disable_verbose_install
                ;;
        esac
        builder_build_autotools_package \
            "$PKG_SRC_DIR" \
            "$PKG_BUILD_DIR" \
            "$@"

        touch "$PKG_TIMESTAMP"
    fi
}

# Handle zlib, only on Win32 because the zlib configure script
# doesn't know how to generate a static library with -fPIC!
build_windows_zlib_package () {
    local ZLIB_VERSION ZLIB_PACKAGE
    local LOC LDFLAGS
    local PREFIX=$(builder_install_prefix)
    local BUILD_DIR=$(builder_build_dir)
    case $(builder_host) in
        windows-x86)
            LOC=-m32
            LDFLAGS=-m32
            ;;
        windows-x86_64)
            LOC=-m64
            LDFLAGS=-m64
            ;;
    esac
    ZLIB_VERSION=$(package_list_get_version zlib)
    dump "$(builder_text) Building zlib-$ZLIB_VERSION"
    ZLIB_PACKAGE=$(package_list_get_filename zlib)
    unpack_archive "$ARCHIVE_DIR/$ZLIB_PACKAGE" "$BUILD_DIR"
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
    unpack_archive "$ARCHIVE_DIR/$ZLIB_PACKAGE" "$BUILD_DIR"
    (
        run cd "$BUILD_DIR/zlib-$ZLIB_VERSION" &&
        export PKG_CONFIG_PATH=$(builder_install_prefix)/lib/pkgconfig &&
        export CROSS_PREFIX=$(builder_gnu_config_host_prefix) &&
        run ./configure --prefix=$(builder_install_prefix) &&
        run make -j$NUM_JOBS &&
        run make install &&
        unset PKG_CONFIG_PATH &&
        unset CROSS_PREFIX
    )
}

# $1+: Configuration options.
build_package_openssl () {
    # Unpack package source into $BUILD_SRC_DIR if needed.
    local BUILD_SRC_DIR=${TEMP_DIR}/src
    local PKG_SRCD_NAME=$(package_list_get_unpack_src_dir "openssl")
    local PKG_SRC_TIMESTAMP=$BUILD_SRC_DIR/timestamp-${PKG_SRCD_NAME}
    if [ ! -f "$PKG_SRC_TIMESTAMP" ]; then
      package_list_unpack_and_patch "openssl" "$ARCHIVE_DIR" "$BUILD_SRC_DIR"
      touch $PKG_SRC_TIMESTAMP
    fi

    shift
    local PKG_SRC_DIR="$BUILD_SRC_DIR/$PKG_SRCD_NAME"
    local PKG_BUILD_DIR=$TEMP_DIR/build-$SYSTEM/$PKG_SRCD_NAME
    local PKG_BLD_TIMESTAMP=$TEMP_DIR/build-$SYSTEM/$PKG_SRCD_NAME-timestamp
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
          CONFIG_FLAGS="linux-x86_64"
          ;;
        linux-x86)
          CONFIG_FLAGS="linux-generic32 386 -m32"
          ;;
        windows-x86)
          CONFIG_FLAGS="mingw 386 -m32"
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
      NUM_JOBS=1

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

# Perform a Darwin build through ssh to a remote machine.
# $1: Darwin host name.
# $2: List of darwin target systems to build for.
do_remote_darwin_build () {
    builder_prepare_remote_darwin_build \
            "/tmp/$USER-rebuild-darwin-ssh-$$/curl-build"

    copy_directory "$ARCHIVE_DIR" "$DARWIN_PKG_DIR"/archive

    local PKG_DIR="$DARWIN_PKG_DIR"
    local REMOTE_DIR=/tmp/$DARWIN_PKG_NAME
    # Generate a script to rebuild all binaries from sources.
    # Note that the use of the '-l' flag is important to ensure
    # that this is run under a login shell. This ensures that
    # ~/.bash_profile is sourced before running the script, which
    # puts MacPorts' /opt/local/bin in the PATH properly.
    #
    # If not, the build is likely to fail with a cryptic error message
    # like "readlink: illegal option -- f"
    cat > $PKG_DIR/build.sh <<EOF
#!/bin/bash -l
PROGDIR=\$(dirname \$0)
\$PROGDIR/scripts/$(program_name) \\
    --build-dir=$REMOTE_DIR/build \\
    --host=$(spaces_to_commas "$DARWIN_SYSTEMS") \\
    --install-dir=$REMOTE_DIR/install-prefix \\
    --prebuilts-dir=$REMOTE_DIR \\
    --aosp-dir=$REMOTE_DIR/aosp \\
    $DARWIN_BUILD_FLAGS
EOF
    builder_run_remote_darwin_build

    run rm -rf "$PKG_DIR"

    run mkdir -p "$INSTALL_DIR" ||
            panic "Could not create final directory: $INSTALL_DIR"

    for SYSTEM in $DARWIN_SYSTEMS; do
        dump "[$SYSTEM] Retrieving remote darwin binaries"
        run rm -rf "$INSTALL_DIR"/* &&
        run $ANDROID_EMULATOR_SSH_WRAPPER rsync -haz --delete \
                --exclude=intermediates --exclude=libs \
                $DARWIN_SSH:$REMOTE_DIR/install-prefix/$SYSTEM \
                $INSTALL_DIR
    done
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote curl build for: $DARWIN_SYSTEMS"
    do_remote_darwin_build "$DARWIN_SSH" "$DARWIN_SYSTEMS"
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

        build_package_openssl

       dump "$(builder_text) Building libcurl"

        build_package curl \
            --disable-debug \
            --enable-optimize \
            --disable-warnings \
            --disable-werror \
            --disable-curldebug \
            --enable-symbol-hiding \
            --disable-ares \
            --enable-dependency-tracking \
            --disable-largefile \
            --disable-shared \
            --disable-libtool-lock \
            --enable-http \
            --disable-ftp \
            --disable-file \
            --disable-ldap \
            --disable-ldaps \
            --disable-rtsp \
            --disable-proxy \
            --disable-dict \
            --disable-telnet \
            --disable-tftp \
            --disable-pop3 \
            --disable-imap \
            --disable-smtp \
            --disable-gopher \
            --disable-manual \
            --disable-libcurl-option \
            --disable-ipv6 \
            --disable-versioned-symbols \
            --enable-threaded-resolver \
            --disable-verbose \
            --disable-sspi \
            --disable-crypto-auth \
            --disable-ntlm-wb \
            --disable-tls-srp \
            --enable-cookies \
            --disable-soname-bump \
            --with-zlib="$builder_install_prefix" \
            --without-winssl \
            --without-darwinssl \
            --with-ssl="$builder_install_prefix" \
            --without-gnutls \
            --without-polarssl \
            --without-cyassl \
            --without-nss \
            --without-axtls \
            --without-libmetalink \
            --without-libssh2 \
            --without-librtmp \
            --without-libidn \
            --without-winidn

        # Copy libraries and header files
        copy_directory_files \
                "$(builder_install_prefix)" \
                "$INSTALL_DIR/$SYSTEM" \
                lib/libcrypto.a \
                lib/libssl.a \
                lib/libz.a \
                lib/libcurl.a \
                bin/openssl$(builder_host_ext)

       copy_directory \
               "$(builder_install_prefix)/include/curl" \
               "$INSTALL_DIR/$SYSTEM/include/curl"

        # Copy the curl executable; this is not necessary
        # but serves as a validation point
        copy_directory_files \
            "$(builder_install_prefix)" \
            "$INSTALL_DIR/$SYSTEM" \
            bin/curl$(builder_host_ext)

    ) || panic "[$SYSTEM] Could not build libcurl!"

done

log "Done building curl."
