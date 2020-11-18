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
shell_import utils/package_builder.shi
shell_import utils/package_list_parser.shi

package_builder_register_options qemu-android-deps

aosp_dir_register_option
prebuilts_dir_register_option
install_dir_register_option

DEFAULT_PREBUILTS_SUBDIR=prebuilts/android-emulator-build
DEFAULT_INSTALL_SUBDIR=qemu-android-deps

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Build qemu-android dependency libraries if needed. This script probes
<prebuilts>/$DEFAULT_INSTALL_SUBDIR/ and will rebuild the libraries from sources
if they are not available.

The default value of <prebuilts> is: \$AOSP/$DEFAULT_PREBUILTS_SUBDIR
Use --prebuilts-dir=<path> or define ANDROID_EMULATOR_PREBUILTS_DIR in your
environment to change it.

Source tarball are probed under <prebuilts>/archive/. If not available,
the 'download-sources.sh' script will be invoked automatically.

Similarly, the script will try to auto-detect the AOSP source tree, to use
prebuilt SDK-compatible toolchains, but you can use --aosp-dir=<path> or
define ANDROID_EMULATOR_AOSP_DIR in your environment to override this.

By default, this script builds binaries for the following host sytems:

    $DEFAULT_HOST_SYSTEMS

You can use --host=<list> to change this.

Final binaries are installed into <prebuilts>/$DEFAULT_INSTALL_SUBDIR/ by
default, but you can change this location with --install-dir=<dir>.

By default, everything is rebuilt in a temporary directory that is
automatically removed after script completion (and after the binaries are
copied into build-prefix/). You can use --build-dir=<path> to specify the
build directory instead. In this case, the directory will not be removed,
allowing one to inspect build failures easily."

option_parse "$@"

##
##  Parameters checks
##
if [ "$PARAMETER_COUNT" != "0" ]; then
    panic "This script doesn't take arguments, see --help for details."
fi

package_builder_process_options

aosp_dir_parse_option
prebuilts_dir_parse_option
install_dir_parse_option

package_builder_parse_package_list

export PKG_CONFIG=$(find_program pkg-config)
if [ "$PKG_CONFIG" ]; then
    log "Found pkg-config at: $PKG_CONFIG"
else
    log "pkg-config is not installed on this system."
fi

display_darwin_warning() {
  # Fancy colors
  RED=`tput setaf 1`
  RESET=`tput sgr0`
  echo "${RED}"
  dump "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-"
  dump "WARNING!! WARNING!! WARNING!! WARNING!! WARNING!! WARNING!!"
  dump "Make sure you are compiling against the lowest supported SDK in"
  dump "Newer SDKs might not be backwards compatible, possibly breaking"
  dump "the build for other users"
  dump "The mac you are building on must have this SDK installed!"
  dump "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-"
  echo "${RESET}"
}

# Handle zlib, only on Win32 because the zlib configure script
# doesn't know how to generate a static library with -fPIC!
do_windows_zlib_package () {
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
        export BINARY_PATH=$PREFIX/bin &&
        export INCLUDE_PATH=$PREFIX/include &&
        export LIBRARY_PATH=$PREFIX/lib &&
        run make -fwin32/Makefile.gcc install \
                PREFIX=$(builder_gnu_config_host_prefix) \
                LOC=$LOC \
                LDFLAGS=$LDFLAGS
    )
}

do_zlib_package () {
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
        export CROSS_PREFIX=$(builder_gnu_config_host_prefix) &&
        (export CFLAGS="-g -O3 -fPIC"; run ./configure --prefix=$(builder_install_prefix)) &&
        run make -j$NUM_JOBS &&
        run make install
    )
}

require_program () {
    local VARNAME PROGNAME CMD
    VARNAME=$1
    PROGNAME=$2
    CMD=$(find_program "$PROGNAME")
    if [ -z "$CMD" ]; then
        panic "Cannot find required build executable: $PROGNAME"
    fi
    eval $VARNAME=\'$CMD\'
}

# Unpack and patch GLib sources
# $1: Unversioned package name (e.g. 'glib')
unpack_and_patch () {
    package_list_unpack_and_patch \
            $1 "$(builder_archive_dir)" "$(builder_build_dir)"
}

# Cross-compiling glib for Win32 is broken and requires special care.
# The following was inspired by the glib.mk from MXE (http://mxe.cc/)
# $1: bitness (32 or 64)
do_windows_glib_package () {
    local PREFIX=$(builder_install_prefix)
    local GNU_CONFIG_HOST_PREFIX=$(builder_gnu_config_host_prefix)
    package_list_unpack_and_patch \
            glib "$(builder_archive_dir)" "$(builder_build_dir)"
    local GLIB_VERSION GLIB_PACKAGE GLIB_DIR
    GLIB_VERSION=$(package_list_get_version glib)
    GLIB_DIR=$(builder_build_dir)/glib-$GLIB_VERSION
    dump "$(builder_text) Building glib-$GLIB_VERSION"
    require_program GLIB_GENMARSHAL glib-genmarshal
    require_program GLIB_COMPILE_SCHEMAS glib-compile-schemas
    require_program GLIB_COMPILE_RESOURCES glib-compile-resources
    (
        run cd "$GLIB_DIR" &&
        export LDFLAGS="-L$PREFIX/lib -L$PREFIX/lib$1" &&
        export CPPFLAGS="-I$PREFIX/include -I$GLIB_DIR -I$GLIB_DIR/glib" &&
        export CC=${GNU_CONFIG_HOST_PREFIX}gcc &&
        export CXX=${GNU_CONFIG_HOST_PREFIX}c++ &&
        export PKG_CONFIG=$(find_program pkg-config) &&
        export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig &&
        run ./configure \
            --prefix=$PREFIX \
            $(builder_gnu_config_host_flag) \
            --disable-shared \
            --with-threads=win32 \
            --with-pcre=internal \
            --disable-debug \
            --disable-man \
            --with-libiconv=no \
            GLIB_GENMARSHAL=$GLIB_GENMARSHAL \
            GLIB_COMPILE_SCHEMAS=$GLIB_COMPILE_SCHEMAS \
            GLIB_COMPILE_RESOURCES=$GLIB_COMPILE_RESOURCES &&

        # Necessary to build gio stuff properly.
        run ln -s "$GLIB_COMPILE_RESOURCES" gio/ &&

        run make -j$NUM_JOBS -C glib install sbin_PROGRAMS= noinst_PROGRAMS= &&
        run make -j$NUM_JOBS -C gmodule install bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS= &&
        run make -j$NUM_JOBS -C gthread install bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS= &&
        run make -j$NUM_JOBS -C gobject install bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS= &&
        run make -j$NUM_JOBS -C gio install bin_PROGRAMS= sbin_PROGRAMS= noinst_PROGRAMS= MISC_STUFF= &&
        run make -j$NUM_JOBS install-pkgconfigDATA &&
        run make -j$NUM_JOBS -C m4macros install &&

        # Missing -lole32 results in link failure later!
        sed -i -e 's|\-lglib-2.0|-lglib-2.0 -lole32|g' \
            $PREFIX/lib/pkgconfig/glib-2.0.pc
    )
}

# Generic routine used to unpack and rebuild a generic auto-tools package
# $1: package name, unversioned and unsuffixed (e.g. 'libpng')
# $2+: extra configuration flags
do_autotools_package () {
    # build position-independent code for non-Windows (Windows is always PIC)
    local PIC_FLAG
    case $(builder_host) in
        windows-*)
            PIC_FLAG=
            ;;
        *)
            PIC_FLAG=--with-pic
            ;;
    esac

    builder_unpack_package_source $1
    builder_build_autotools_package "$@" --disable-shared $PIC_FLAG
}

do_dtc_package () {
    local PKG=dtc
    unpack_and_patch $PKG
    PKG_VERSION=$(package_list_get_version $PKG)
    PKG_NAME=$(package_list_get_filename $PKG)
    dump "$(builder_text) Building libfdt from $PKG-$PKG_VERSION"
    # NOTE: Cannot use the dtc Makefiles here, but since the library is
    #       really simple, just compile the source files straight into a
    #       library, then 'install' it manually !!
    (
        run cd "$(builder_build_dir)/$PKG-$PKG_VERSION" &&
        CC=$(builder_gnu_config_host_prefix)gcc
        AR=$(builder_gnu_config_host_prefix)ar
        CCFLAGS=
        LIBNAME=libfdt.a
        var_append CCFLAGS "-Ilibfdt"
        case $(builder_host) in
            linux-*|darwin-*)
                var_append CCFLAGS "-fPIC"
                ;;
        esac
        run $CC -c $CCFLAGS libfdt/*.c
        run $AR crs $LIBNAME *.o
        run mkdir -p "$PREFIX/include"
        run cp libfdt/*.h "$PREFIX"/include/
        run mkdir -p "$PREFIX/lib"
        run cp $LIBNAME "$PREFIX/lib"
    ) || panic "Cannot rebuild libfdt"
}

# $1: host os name.
# $2: environment shell script.
build_qemu_android_deps () {
    builder_prepare_for_host "$1" "$AOSP_DIR"

    # When building aarch64, we may be in a cross build. If so,
    # export CONFIG_SITE to point to cross-config.arm64
    if [ "$1" == "linux-aarch64" ]; then
        log "Building linux-aarch64; using CONFIG_SITE for arm64"
        export CONFIG_SITE=$(dirname "$0")/cross-config.arm64
    fi

    local is_darwin_aarch64="0"
    if [ "$1" == "darwin-aarch64" ]; then
        log "Building darwin-aarch64, some stuff changes"
        is_darwin_aarch64="1"
    fi

    if [ -z "$OPT_FORCE" ]; then
        if timestamp_check \
                "$INSTALL_DIR/$(builder_host)" qemu-android-deps; then
            log "$(builder_text) Already built."
            return 0
        fi
    fi

    local PREFIX=$(builder_install_prefix)

    export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
    case $1 in
        darwin-*)
          display_darwin_warning
          ;;
    esac

    # Handle zlib for Windows
    case $1 in
        windows-*)
            do_windows_zlib_package
            ;;
        *)
            do_zlib_package
            ;;
    esac

    # libfdt, from the dtc package, is needed.
    do_dtc_package dtc

    # libffi is required by glib.
    if [ "1" == "$is_darwin_aarch64" ]; then
        do_autotools_package libffi --build=aarch64-apple-darwin20.0.0
    else
        do_autotools_package libffi
    fi

    # Must define LIBFFI_CFLAGS and LIBFFI_LIBS to ensure
    # that GLib picks it up properly. Note that libffi places
    # its headers and libraries in uncommon places.
    LIBFFI_VERSION=$(package_list_get_version libffi)
    LIBFFI_CFLAGS="-I$PREFIX/lib/libffi-$LIBFFI_VERSION/include"
    LIBFFI_LIBS="$PREFIX/lib/libffi.la"
    if [ ! -f "$LIBFFI_LIBS" ]; then
        LIBFFI_LIBS="$PREFIX/lib64/libffi.la"
    fi
    if [ ! -f "$LIBFFI_LIBS" ]; then
        LIBFFI_LIBS="$PREFIX/lib32/libffi.la"
    fi
    if [ ! -f "$LIBFFI_LIBS" ]; then
        panic "Cannot locate libffi libraries!"
    fi

    log "Using LIBFFI_CFLAGS=[$LIBFFI_CFLAGS]"
    log "Using LIBFFI_LIBS=[$LIBFFI_LIBS]"
    export LIBFFI_CFLAGS LIBFFI_LIBS

    # libiconv and gettext are needed on Darwin only
    case $1 in
        darwin-*)
            do_autotools_package libiconv
            do_autotools_package gettext \
                --disable-rpath \
                --disable-acl \
                --disable-curses \
                --disable-openmp \
                --disable-java \
                --disable-native-java \
                --without-emacs \
                --disable-c++ \
                --without-libexpat-prefix
            ;;
    esac

    # glib is required by pkg-config and qemu-android
    case $1 in
        windows-x86_64)
            do_windows_glib_package 64
            ;;
        darwin-aarch64)
            do_autotools_package glib \
                --disable-always-build-tests \
                --disable-debug \
                --disable-fam \
                --disable-installed-tests \
                --disable-libelf \
                --disable-man \
                --disable-selinux \
                --disable-xattr \
                --disable-compile-warnings
            ;;
        *)
            do_autotools_package glib \
                --disable-always-build-tests \
                --disable-debug \
                --disable-fam \
                --disable-included-printf \
                --disable-installed-tests \
                --disable-libelf \
                --disable-man \
                --disable-selinux \
                --disable-xattr \
                --disable-compile-warnings
            ;;
    esac

    # Export these to ensure that pkg-config picks them up properly.
    export GLIB_CFLAGS="-I$PREFIX/include/glib-2.0 -I$PREFIX/lib/glib-2.0/include"
    export GLIB_LIBS="$PREFIX/lib/libglib-2.0.la"
    case $(get_build_os) in
        darwin)
            GLIB_LIBS="$GLIB_LIBS -Wl,-framework,Carbon -Wl,-framework,Foundation"
            ;;
    esac

    # pkg-config is required by qemu-android, and not available on
    # Windows and OS X
    do_autotools_package pkg-config \
        --without-pc-path \
        --disable-host-tool

    # Handle libpng
    do_autotools_package libpng

    do_autotools_package pixman \
        --disable-gtk \
        --disable-libpng

    # Handle SDL2
    local build_sdl=1

    case $1 in
        darwin-aarch64)
            build_sdl=0
            ;;
    esac

    if [ "1" == "$build_sdl" ]; then
        local SDL_FLAGS=
        case $1 in
            windows*)
                # DirectX sources assume __FUNCTION__ is a macro, but on GCC it isn't.
                SDL_FLAGS=--disable-directx
                ;;
        esac

        do_autotools_package SDL2 $SDL_FLAGS
        SDL_CONFIG=$PREFIX/bin/sdl2-config

        # default sdl2.pc libs doesn't work with our cross-compiler,
        # append missing libs
        case $1 in
            windows-*)
                sed -i -e '/^Libs: -L\${libdir} /s/$/ -Wl,--no-undefined -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc/' $PREFIX/lib/pkgconfig/sdl2.pc
                ;;
            darwin-*)
                sed -i "" -e '/^Libs: -L\${libdir} /s/$/ -lm -Wl,-framework,OpenGL  -Wl,-framework,ForceFeedback -lobjc -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit/' $PREFIX/lib/pkgconfig/sdl2.pc
                ;;
            linux-*)
                sed -i -e '/^Libs: -L\${libdir} /s/$/ -Wl,--no-undefined -lm -ldl -lrt/' $PREFIX/lib/pkgconfig/sdl2.pc
                ;;
        esac
    else
        SDL_CONFIG=none
    fi

    PKG_CONFIG_LIBDIR=$PREFIX/lib/pkgconfig
    case $1 in
        windows-*)
            # Use the host version, or the build will freeze.
            PKG_CONFIG=pkg-config
            ;;
        *)
            PKG_CONFIG=$PREFIX/bin/pkg-config
            ;;
    esac
    export SDL_CONFIG PKG_CONFIG PKG_CONFIG_LIBDIR

    # Create script to setup the environment correctly for the
    # later qemu-android build.
    ENV_SH=$(builder_build_dir)/env-qemu-prebuilts-$(builder_host).sh
    cat > $ENV_SH <<EOF
# Auto-generated automatically - DO NOT EDIT
export PREFIX=$PREFIX
export LIBFFI_CFLAGS="$LIBFFI_CFLAGS"
export LIBFFI_LIBS="$LIBFFI_LIBS"
export GLIB_CFLAGS="$GLIB_CFLAGS"
export GLIB_LIBS="$GLIB_LIBS"
export SDL_CONFIG=$SDL_CONFIG
export PKG_CONFIG=$PKG_CONFIG
export PKG_CONFIG_LIBDIR=$PKG_CONFIG_LIBDIR
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH
export PATH=$PATH
EOF

    dump "$(builder_text) Copying prebuilts to $INSTALL_DIR/$(builder_host)"

    BINARY_DIR=$INSTALL_DIR/$(builder_host)
    run mkdir -p "$BINARY_DIR" ||
    panic "Could not create final directory: $BINARY_DIR"

    local SUBDIR
    for SUBDIR in include lib lib32 lib64 bin; do
        if [ ! -d "$PREFIX/$SUBDIR" ]; then
            continue
        fi
        run copy_directory "$PREFIX/$SUBDIR" "$BINARY_DIR/$SUBDIR"
    done

    unset PKG_CONFIG PKG_CONFIG_PATH PKG_CONFIG_LIBDIR SDL_CONFIG
    unset LIBFFI_CFLAGS LIBFFI_LIBS GLIB_CFLAGS GLIB_LIBS

    timestamp_set "$INSTALL_DIR/$(builder_host)" qemu-android-deps
}

# Ignore prebuilt Darwin binaries if --force is not used.
if [ -z "$OPT_FORCE" ]; then
    builder_check_all_timestamps "$INSTALL_DIR" qemu-android-deps
fi

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Let the people know that this is risky..
    display_darwin_warning

    # Perform remote Darwin build first.
    dump "Remote qemu-android-deps build for: $DARWIN_SYSTEMS"
    builder_prepare_remote_darwin_build

    if [ "$OPT_FORCE" ]; then
        var_append DARWIN_BUILD_FLAGS "--force"
    fi

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        builder_remote_darwin_retrieve_install_dir $SYSTEM $INSTALL_DIR
        timestamp_set "$INSTALL_DIR/$SYSTEM" qemu-android-deps
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    build_qemu_android_deps $SYSTEM
done

log "Done building qemu-android dependencies."
