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

ARCHIVE_DIR=$PREBUILTS_DIR/archive
if [ ! -d "$ARCHIVE_DIR" ]; then
    dump "Downloading dependencies sources first."
    $(program_directory)/download-sources.sh \
        --verbosity=$(get_verbosity) \
        --prebuilts-dir=$PREBUILTS_DIR
fi
ARCHIVE_DIR=$(cd "$ARCHIVE_DIR" && pwd -P)
log "Using archive directory: $ARCHIVE_DIR"

PACKAGE_LIST=$ARCHIVE_DIR/PACKAGES.TXT
if [ ! -f "$PACKAGE_LIST" ]; then
    panic "Missing package list file from archive: $PACKAGE_LIST"
fi

package_list_parse_file "$PACKAGE_LIST"

export PKG_CONFIG=$(find_program pkg-config)
if [ "$PKG_CONFIG" ]; then
    log "Found pkg-config at: $PKG_CONFIG"
else
    log "pkg-config is not installed on this system."
fi

# Handle zlib, only on Win32 because the zlib configure script
# doesn't know how to generate a static library with -fPIC!
do_windows_zlib_package () {
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
    unpack_archive "$ARCHIVE_DIR/$ZLIB_PACKAGE" "$BUILD_DIR"
    (
        run cd "$BUILD_DIR/zlib-$ZLIB_VERSION" &&
        export CROSS_PREFIX=$(builder_gnu_config_host_prefix) &&
        run ./configure --prefix=$(builder_install_prefix) &&
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
    local PKG_NAME="$1"
    local BUILD_DIR=$(builder_build_dir)
    local PKG_VERSION PKG_PACKAGE PKG_PATCHES_DIR PKG_PATCHES_PACKAGE
    local PKG_DIR PATCH
    PKG_VERSION=$(package_list_get_version $PKG_NAME)
    if [ -z "$PKG_VERSION" ]; then
        panic "Cannot find version for package $PKG_NAME!"
    fi
    log "Extracting $PKG_NAME-$PKG_VERSION"
    PKG_PACKAGE=$(package_list_get_filename $PKG_NAME)
    unpack_archive "$ARCHIVE_DIR/$PKG_PACKAGE" "$BUILD_DIR" ||
    panic "Could not unpack $PKG_NAME-$PKG_VERSION"
    PKG_DIR=$BUILD_DIR/$PKG_NAME-$PKG_VERSION

    PKG_PATCHES_DIR=$PKG_NAME-$PKG_VERSION-patches
    PKG_PATCHES_PACKAGE=$ARCHIVE_DIR/${PKG_PATCHES_DIR}.tar.xz
    if [ -f "$PKG_PATCHES_PACKAGE" ]; then
        log "Patching $PKG_NAME-$PKG_VERSION"
        unpack_archive "$PKG_PATCHES_PACKAGE" "$BUILD_DIR"
        for PATCH in $(cd "$BUILD_DIR" && ls "$PKG_PATCHES_DIR"/*.patch); do
            log "Applying patch: $PATCH"
            (cd "$PKG_DIR" && run patch -p1 < "../$PATCH") ||
                    panic "Could not apply $PATCH"
        done
    fi
}

# Cross-compiling glib for Win32 is broken and requires special care.
# The following was inspired by the glib.mk from MXE (http://mxe.cc/)
# $1: bitness (32 or 64)
do_windows_glib_package () {
    local PREFIX=$(builder_install_prefix)
    local GNU_CONFIG_HOST_PREFIX=$(builder_gnu_config_host_prefix)
    unpack_and_patch glib
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
    local PKG PKG_VERSION PKG_NAME
    local PREFIX=$(builder_install_prefix)
    PKG=$1
    shift
    unpack_and_patch $PKG
    PKG_VERSION=$(package_list_get_version $PKG)
    PKG_NAME=$(package_list_get_filename $PKG)
    dump "$(builder_text) Building $PKG-$PKG_VERSION"
    if [ $PKG == "SDL2" ]; then
        unset SDKROOT
    fi
    (
        run cd "$(builder_build_dir)/$PKG-$PKG_VERSION" &&
        export LDFLAGS="-L$PREFIX/lib" &&
        export CPPFLAGS="-I$PREFIX/include" &&
        export PKG_CONFIG_LIBDIR="$PREFIX/lib/pkgconfig" &&
        run ./configure \
            --prefix=$PREFIX \
            $(builder_gnu_config_host_flag) \
            --disable-shared \
            --with-pic \
            "$@" &&
        run make -j$NUM_JOBS V=1 &&
        run make install V=1
    ) ||
    panic "Could not build and install $PKG_NAME"
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
    builder_prepare_for_host "$1"

    if [ -z "$OPT_FORCE" ]; then
        if timestamp_check \
                "$INSTALL_DIR/$(builder_host)" qemu-android-deps; then
            log "$(builder_text) Already built."
            return 0
        fi
    fi

    local PREFIX=$(builder_install_prefix)

    export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
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
    do_autotools_package libffi

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
        windows-x86)
            do_windows_glib_package 32
            ;;
        windows-x86_64)
            do_windows_glib_package 64
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

    # Handle SDL1
    EXTRA_SDL_FLAGS=
    case $(get_build_os) in
        darwin)
            EXTRA_SDL_FLAGS="--disable-video-x11"
            ;;
    esac
    do_autotools_package SDL \
        --disable-audio \
        --disable-joystick \
        --disable-cdrom \
        --disable-file \
        --disable-threads \
        $EXTRA_SDL_FLAGS

    # The SDL build script install a buggy sdl.pc when cross-compiling for
    # Windows as a static library. I.e. it lacks many of the required
    # libraries, that are part of --static-libs. Patch it directly
    # instead.
    case $1 in
        windows-*)
            sed -i -e 's|^Libs: -L\${libdir}  -lmingw32 -lSDLmain -lSDL  -mwindows|Libs: -lmingw32 -lSDLmain -lSDL  -mwindows -lm -luser32 -lgdi32 -lwinmm -ldxguid|g' $PREFIX/lib/pkgconfig/sdl.pc
            ;;
    esac

    SDL_CONFIG=$PREFIX/bin/sdl-config

    do_autotools_package SDL2
    SDL2_CONFIG=$PREFIX/bin/sdl2-config

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

# Perform a Darwin build through ssh to a remote machine.
# $1: Darwin host name.
# $2: List of darwin target systems to build for.
do_remote_darwin_build () {
    builder_prepare_remote_darwin_build \
            "/tmp/$USER-rebuild-darwin-ssh-$$/qemu-android-deps-build"

    copy_directory "$ARCHIVE_DIR" "$DARWIN_PKG_DIR"/archive

    if [ "$OPT_FORCE" ]; then
        var_append DARWIN_BUILD_FLAGS "--force"
    fi
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

    local BINARY_DIR=$INSTALL_DIR
    run mkdir -p "$BINARY_DIR" ||
            panic "Could not create final directory: $BINARY_DIR"

    for SYSTEM in $DARWIN_SYSTEMS; do
        dump "[$SYSTEM] Retrieving remote darwin binaries"
        run scp -r "$DARWIN_SSH":$REMOTE_DIR/install-prefix/$SYSTEM \
                $BINARY_DIR/
        timestamp_set "$INSTALL_DIR/$SYSTEM" qemu-android-deps
    done

}

# Ignore prebuilt Darwin binaries if --force is not used.
if [ -z "$OPT_FORCE" ]; then
    builder_check_all_timestamps "$INSTALL_DIR" qemu-android-deps
fi

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    # Perform remote Darwin build first.
    dump "Remote qemu-android-deps build for: $DARWIN_SYSTEMS"
    do_remote_darwin_build "$DARWIN_SSH" "$DARWIN_SYSTEMS"
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    build_qemu_android_deps $SYSTEM
done

log "Done building qemu-android dependencies."
