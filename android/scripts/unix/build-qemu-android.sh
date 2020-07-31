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

DEFAULT_INSTALL_SUBDIR=qemu-android

PROGRAM_PARAMETERS="<qemu-android>"

PROGRAM_DESCRIPTION=\
"Build upstream version of qemu-android sources (*without* Android support).
This script will probe <prebuilts>/$DEFAULT_INSTALL_SUBDIR/ to look for
prebuilt dependencies. If not available, the 'build-qemu-android-deps.sh'
script will be invoked automatically.

The default value of <prebuilts> is: $DEFAULT_PREBUILTS_DIR
Use --prebuilts-dir=<path> or define ANDROID_EMULATOR_PREBUILTS_DIR in your
environment to change it.

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

package_builder_register_options

VALID_TARGETS="arm,arm64,x86,x86_64"
DEFAULT_TARGETS="arm,arm64,x86,x86_64"

OPT_TARGET=
option_register_var "--target=<list>" OPT_TARGET \
        "Build binaries for targets [$DEFAULT_TARGETS]"

OPT_SRC_DIR=
option_register_var "--src-dir=<dir>" OPT_SRC_DIR \
        "Set qemu2 source directory [autodetect]"

OPT_DEBUG=
option_register_var "--debug" OPT_DEBUG \
        "Generate debuggable binaries."

OPT_TESTS=
option_register_var "--tests" OPT_TESTS \
        "Build and run QEMU2 tests."

prebuilts_dir_register_option
aosp_dir_register_option
install_dir_register_option qemu-android

option_parse "$@"

if [ "$PARAMETER_COUNT" != 0 ]; then
    panic "This script takes no arguments. See --help for details."
fi

if [ "$OPT_SRC_DIR" ]; then
    QEMU_SRCDIR=$OPT_SRC_DIR
else
    DEFAULT_SRC_DIR=$(cd $(program_directory)/../../.. && pwd -P 2>/dev/null || true)
    if [ "$DEFAULT_SRC_DIR" ]; then
        QEMU_SRCDIR=$DEFAULT_SRC_DIR
        log "Auto-config: --src-dir=$QEMU_SRCDIR"
    else
        panic "Could not find qemu-android sources. Please use --src-dir=<dir>."
    fi
fi
if [ ! -f "$QEMU_SRCDIR/include/qemu-common.h" ]; then
    panic "Not a valid qemu2 source directory: $QEMU_SRCDIR"
fi
QEMU_SRCDIR=$(cd "$QEMU_SRCDIR" && pwd -P)

prebuilts_dir_parse_option
aosp_dir_parse_option
install_dir_parse_option

package_builder_process_options qemu-android

QEMU_ANDROID_DEPS_INSTALL_DIR=$PREBUILTS_DIR/qemu-android-deps

if [ -z "$OPT_INSTALL_DIR" ]; then
    # The default installation directory for --no-android binaries is
    # 'qemu-upstream' instead of 'qemu-android'
    INSTALL_DIR=${INSTALL_DIR%%-android}-upstream
fi

# Qemu needs python2 to build
PYTHON2=$(which python2)

##
## Handle target system list
##
if [ "$OPT_TARGET" ]; then
    TARGETS=$(commas_to_spaces "$OPT_TARGET")
else
    TARGETS=$(commas_to_spaces "$DEFAULT_TARGETS")
    log "Auto-config: --target='$TARGETS'"
fi

BAD_TARGETS=
for TARGET in $TARGETS; do
    if ! list_contains "$VALID_TARGETS" "$TARGET"; then
        BAD_TARGETS="$BAD_TARGETS $TARGET"
    fi
done
if [ "$BAD_TARGETS" ]; then
    panic "Invalid target name(s): [$BAD_TARGETS], use one of: $VALID_TARGETS"
fi

export PKG_CONFIG=$(find_program pkg-config)
if [ "$PKG_CONFIG" ]; then
    log "Found pkg-config at: $PKG_CONFIG"
else
    log "pkg-config is not installed on this system."
fi

# Check that we have the right prebuilts
EXTRA_FLAGS=
if [ "$DARWIN_SSH" ]; then
    var_append EXTRA_FLAGS "--darwin-ssh=$DARWIN_SSH"
fi
$(program_directory)/build-qemu-android-deps.sh \
    --verbosity=$(get_verbosity) \
    --prebuilts-dir=$PREBUILTS_DIR \
    --aosp-dir=$AOSP_DIR \
    --host=$(spaces_to_commas "$HOST_SYSTEMS") \
    $EXTRA_FLAGS \
        || panic "could not check or rebuild qemu-android dependencies."

display_darwin_warning() {
  # Fancy colors
  RED=`tput setaf 1`
  RESET=`tput sgr0`
  echo "${RED}"
  dump "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-"
  dump "WARNING!! WARNING!! WARNING!! WARNING!! WARNING!! WARNING!!"
  dump "Make sure the mac you are building on DOES not have any"
  dump "libraries installed that are not found on a clean factory mac."
  dump "For example homebrew with gcrypt, or a private openssl library"
  dump "or anything else for that matter."
  dump "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-"
  echo "${RESET}"
}
# Local build of qemu-android binaries.
# $1: host os name.
# $2: AOSP source directory
build_qemu_android () {
    builder_prepare_for_host $1 "$2"

    QEMU_TARGETS=
    QEMU_TARGET_LIST=
    QEMU_TARGET_BUILDS=
    for TARGET in $TARGETS; do
        case $TARGET in
            arm64)
                QEMU_TARGET=aarch64
                ;;
            x86)
                QEMU_TARGET=i386
                ;;
            mips)
                QEMU_TARGET=mipsel
                ;;
            mips64)
                QEMU_TARGET=mips64el
                ;;
            *)
                QEMU_TARGET=$TARGET
                ;;
        esac
        var_append QEMU_TARGETS "$QEMU_TARGET"
        var_append QEMU_TARGET_LIST "${QEMU_TARGET}-softmmu"
        var_append QEMU_TARGET_BUILDS "$QEMU_TARGET-softmmu/qemu-system-$QEMU_TARGET$(builder_host_exe_extension)"
    done

    dump "$(builder_text) Building qemu-android"
    log "Qemu targets: $QEMU_TARGETS"

    BUILD_DIR=$TEMP_DIR/build-$1/qemu-android
    (
        PREFIX=$QEMU_ANDROID_DEPS_INSTALL_DIR/$1

        if [ "$(get_verbosity)" -gt 3 ]; then
            var_append BUILD_FLAGS "V=1"
        fi

        # Generate LINK-* files that will be used to generate build files
        # for the emulator build system.
        LINKPROG_FLAGS="LINKPROG=$BUILD_DIR/link-prog AR=$BUILD_DIR/ar-prog"

        run mkdir -p "$BUILD_DIR"
        run rm -rf "$BUILD_DIR"/*
        run cd "$BUILD_DIR"
        EXTRA_LDFLAGS="-L$PREFIX/lib"
        case $1 in
           darwin-*)
               EXTRA_LDFLAGS="$EXTRA_LDFLAGS -Wl,-framework,Carbon"
               ;;
           windows_msvc*)
               EXTRA_LDFLAGS="$EXTRA_LDFLAGS -ladvapi32"
               ;;
           *)
               EXTRA_LDFLAGS="$EXTRA_LDFLAGS -static-libgcc -static-libstdc++"
               ;;
        esac
        case $1 in
            windows*)
                ;;
            *)
                EXTRA_LDFLAGS="$EXTRA_LDFLAGS -ldl -lm"
                ;;
        esac
        CROSS_PREFIX_FLAG=
        if [ "$(builder_gnu_config_host_prefix)" ]; then
            CROSS_PREFIX_FLAG="--cross-prefix=$(builder_gnu_config_host_prefix)"
        fi
        EXTRA_CFLAGS="-I$PREFIX/include"
        case $1 in
            windows*)
                # Necessary to build version.rc properly.
                # VS_VERSION_INFO is a relatively new features that is
                # not supported by the cross-toolchain's windres tool.
                EXTRA_CFLAGS="$EXTRA_CFLAGS -DVS_VERSION_INFO=1 -Wno-unused-command-line-argument"
                EXTRA_CFLAGS="$EXTRA_CFLAGS -Wno-incompatible-ms-struct -Wno-c++11-narrowing"
                ;;
            darwin-*)
                EXTRA_CFLAGS="$EXTRA_CFLAGS -mmacosx-version-min=10.8"
                ;;
        esac

        AUDIO_BACKENDS_FLAG=
        case $1 in
            linux-x86*)
                # Use PulseAudio on Linux because the default backend,
                # OSS, does not work
                AUDIO_BACKENDS_FLAG="--audio-drv-list=pa"
                ;;
            linux-aarch64)
                # TODO(bohu) cross build pa for aarch64
                AUDIO_BACKENDS_FLAG=""
                ;;
            windows*)
                # Prefer winaudio on Windows over dsound.
                AUDIO_BACKENDS_FLAG="--audio-drv-list=winaudio,dsound"
                ;;
        esac

        WHPX_FLAG=
        case $1 in
            windows*)
                # Windows Hypervisor Platform accelerator will only be needed
                # on Windows.
                WHPX_FLAG="--enable-whpx"
                ;;
            *)
                WHPX_FLAG="--disable-whpx"
                ;;
        esac

        LIBUSB_FLAGS=
        case $1 in
            windows*)
                # Libusb support on windows is needed for bluetooth passthru
                LIBUSB_FLAGS="--enable-libusb --enable-usb-redir"
                ;;
            linux-aarch64)
                # TODO(bohu): cross build libusb for aarch64
                LIBUSB_FLAGS="--disable-libusb --disable-usb-redir"
                ;;
            *)
                LIBUSB_FLAGS="--enable-libusb --enable-usb-redir"
                ;;
        esac


        PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
        PKG_CONFIG_LIBDIR=$PREFIX/lib/pkgconfig
        case $1 in
            windows*)
                # Use the host version, or the build will freeze.
                PKG_CONFIG=`which pkg-config`
                ;;
            *)
                PKG_CONFIG=$PREFIX/bin/pkg-config
                ;;
        esac

        # Create a pkg-config wrapper that redefines the |prefix|
        # variable to the right location, then force its usage.
        cat > pkg-config <<EOF
#!/bin/sh
$PKG_CONFIG --define-variable=prefix=$PREFIX "\$@"
EOF
        chmod a+x pkg-config
        PKG_CONFIG=$(pwd -P)/pkg-config

        export PKG_CONFIG PKG_CONFIG_PATH PKG_CONFIG_LIBDIR

        # Export these to ensure that pkg-config picks them up properly.
        export GLIB_CFLAGS="-I$PREFIX/include/glib-2.0 -I$PREFIX/lib/glib-2.0/include"
        if [ "$BUILD_OS" = "windows_msvc" ]; then
          export GLIB_LIBS="$PREFIX/lib/libglib-2.0.lib"
        else
          export GLIB_LIBS="$PREFIX/lib/libglib-2.0.la"
        fi
        case $BUILD_OS in
            darwin)
                GLIB_LIBS="$GLIB_LIBS -Wl,-framework,Carbon -Wl,-framework,Foundation"
        esac

        case $1 in
          darwin*)
            GCC=g++
            ;;
          linux-x86*)
            GCC=gcc
            ;;
          linux-aarch64)
            GCC=g++
            ;;
          windows*)
            GCC=g++
            ;;
        esac

        # Create a linking program that will capture its command-line to a file.
        cat > link-prog <<EOF
#!/bin/sh

# Extract target file.
seen_o=
target=
for opt; do
    if [ "\$opt" = "-o" ]; then
      seen_o=true
    elif [ "\$seen_o" ]; then
      target=\$opt
      seen_o=
    fi
done

if [ "\$target" ]; then
  dest=$BUILD_DIR/LINK-\$target
  mkdir -p \$(dirname \$dest)
  printf "%s\n" "\$@" > $BUILD_DIR/LINK-\$target
fi
$(builder_gnu_config_host_prefix)$GCC "\$@"
EOF
        chmod a+x link-prog

        # Create an archiver program that will capture its command-line to a file.
        cat > ar-prog <<EOF
#!/bin/sh

# Extract target file.
seen_rcs=
target=
for opt; do
    if [ "\$opt" = "rcs" ]; then
      seen_rcs=true
    elif [ "\$seen_rcs" ]; then
      target=\$opt
      seen_rcs=
    fi
done

if [ "\$target" ]; then
  dest=$BUILD_DIR/LINK-\$target
  mkdir -p \$(dirname \$dest)
  printf "%s\n" "\$@" > $BUILD_DIR/LINK-\$target
fi
$(builder_gnu_config_host_prefix)ar "\$@"
EOF
        chmod a+x ar-prog

        SDL_CONFIG=$PREFIX/bin/sdl2-config
        export SDL_CONFIG

        DEBUG_FLAGS=
        if [ "$OPT_DEBUG" ]; then
            DEBUG_FLAGS="--enable-debug"
        fi

        run $QEMU_SRCDIR/configure \
            $CROSS_PREFIX_FLAG \
            --target-list="$QEMU_TARGET_LIST" \
            --prefix=$PREFIX \
            --extra-cflags="$EXTRA_CFLAGS" \
            --extra-ldflags="$EXTRA_LDFLAGS" \
            --python=$PYTHON2 \
            $DEBUG_FLAGS \
            $LIBUSB_FLAGS \
            $AUDIO_BACKENDS_FLAG \
            $WHPX_FLAG \
            --disable-attr \
            --disable-bluez \
            --disable-brlapi \
            --disable-blobs \
            --disable-bzip2 \
            --disable-cap-ng \
            --disable-capstone \
            --disable-cocoa \
            --disable-curl \
            --disable-curses \
            --disable-docs \
            --disable-glusterfs \
            --disable-gtk \
            --disable-guest-agent \
            --disable-guest-agent-msi \
            --disable-libiscsi \
            --disable-jemalloc \
            --disable-libnfs \
            --disable-libssh2 \
            --disable-seccomp \
            --disable-smartcard \
            --disable-spice \
            --disable-user \
            --disable-vde \
            --disable-vte \
            --disable-vxhs \
            --disable-vhost-net \
            --disable-vnc-sasl \
            --disable-werror \
            --disable-xen \
            --disable-xen-pci-passthrough \
            --disable-xfsctl \
            --enable-sdl \
            --enable-trace-backends=nop \
            --enable-vnc \
            --with-sdlabi=2.0 \
            &&

            case $1 in
                windows*)
                    # Cannot build optionrom when cross-compiling to Windows,
                    # so disable that by removing the ROMS= line from
                    # the generated config-host.mak file.
                    sed -i -e '/^ROMS=/d' config-host.mak

                    # For Windows, config-host.h must be created after
                    # before the rest, or the parallel build will
                    # fail miserably.
                    run make config-host.h $BUILD_FLAGS
                    run make version.o  $BUILD_FLAGS
                    ;;
            esac

            # Now build everything else in parallel.
            run make -j$NUM_JOBS $BUILD_FLAGS $LINKPROG_FLAGS V=1

            for QEMU_EXE in $QEMU_TARGET_BUILDS; do
                if [ ! -f "$QEMU_EXE" ]; then
                    panic "$(builder_text) Could not build $QEMU_EXE!!"
                fi
            done

            if [ -n "$OPT_TESTS" ]; then
                if ! run make -j$NUM_JOBS $BUILD_FLAGS check GTESTER_FLAGS="-m quick -k"
                then
                    panic "$(builder_text) Failure when running qemu2 unittests!!"
                fi
            fi

    ) || panic "Build failed!!"

    BINARY_DIR=$INSTALL_DIR/$1
    run mkdir -p "$BINARY_DIR" ||
    panic "Could not create final directory: $BINARY_DIR"

    run cp -p \
        "$BUILD_DIR"/config-host.h \
        "$BINARY_DIR"/config-host.h

    for QEMU_TARGET in $QEMU_TARGETS; do
        QEMU_EXE=qemu-system-${QEMU_TARGET}$(builder_host_exe_extension)
        dump "$(builder_text) Copying $QEMU_EXE to $BINARY_DIR/"

        run cp -p \
            "$BUILD_DIR"/$QEMU_TARGET-softmmu/$QEMU_EXE \
            "$BINARY_DIR"/$QEMU_EXE

        mkdir -p "$BINARY_DIR"/$QEMU_TARGET-softmmu

        run cp -p \
            "$BUILD_DIR"/$QEMU_TARGET-softmmu/config-target.h \
            "$BINARY_DIR"/$QEMU_TARGET-softmmu/config-target.h

        if [ -z "$OPT_DEBUG" ]; then
            case $1 in
                linux-aarch64*)
                ;;
                *)
                run ${GNU_CONFIG_HOST_PREFIX}strip "$BINARY_DIR"/$QEMU_EXE
                ;;
            esac
        fi
    done

    # Copy LINK-* files, adjusting hard-coded paths in them.
    for LINK_FILE in "$BUILD_DIR"/LINK-*; do
        [ -f $LINK_FILE ] && \
        sed \
            -e 's|'${PREBUILTS_DIR}'|@PREBUILTS_DIR@|g' \
            -e 's|'${QEMU_SRCDIR}'|@SRC_DIR@|g' \
            -e 's|'${BUILD_DIR}'||g' \
            "$LINK_FILE" > $INSTALL_DIR/$1/$(basename "$LINK_FILE")
    done

    mkdir -p $INSTALL_DIR/$1/tests
    # Copy all the test files
    for LINK_FILE in "$BUILD_DIR"/LINK-tests/*; do
        [ -f $LINK_FILE ] && \
        sed \
            -e 's|'${PREBUILTS_DIR}'|@PREBUILTS_DIR@|g' \
            -e 's|'${QEMU_SRCDIR}'|@SRC_DIR@|g' \
            -e 's|'${BUILD_DIR}'||g' \
            "$LINK_FILE" > $INSTALL_DIR/$1/tests/$(basename "$LINK_FILE")
    done

    unset PKG_CONFIG PKG_CONFIG_PATH PKG_CONFIG_LIBDIR SDL_CONFIG
    unset LIBFFI_CFLAGS LIBFFI_LIBS GLIB_CFLAGS GLIB_LIBS

    timestamp_set "$INSTALL_DIR/$1" qemu-android
}

if [ "$DARWIN_SSH" -a "$DARWIN_SYSTEMS" ]; then
    dump "Remote build for: $DARWIN_SYSTEMS"

    display_darwin_warning
    builder_prepare_remote_darwin_build

    copy_directory "$QEMU_SRCDIR" "$DARWIN_PKG_DIR/qemu-src"

    run mkdir -p "$DARWIN_PKG_DIR/prebuilts"
    for SYSTEM in $DARWIN_SYSTEMS; do
        copy_directory "$QEMU_ANDROID_DEPS_INSTALL_DIR/$SYSTEM" \
                "$DARWIN_PKG_DIR/prebuilts/qemu-android-deps/$SYSTEM"
    done

    var_append DARWIN_BUILD_FLAGS \
        --target=$(spaces_to_commas "$TARGETS") \
        --src-dir=$DARWIN_REMOTE_DIR/qemu-src \
        --prebuilts-dir=$DARWIN_REMOTE_DIR/prebuilts \
        --install-dir=$DARWIN_REMOTE_DIR/prebuilts/qemu-android \

    builder_run_remote_darwin_build

    for SYSTEM in $DARWIN_SYSTEMS; do
        BINARY_DIR="$INSTALL_DIR/$SYSTEM"
        dump "[$SYSTEM] Retrieving remote darwin binaries."
        run mkdir -p "$BINARY_DIR" ||
                panic "Could not create installation directory: $BINARY_DIR"

        REMOTE_SRCDIR="$DARWIN_SSH:$DARWIN_REMOTE_DIR/prebuilts/qemu-android/$SYSTEM"
        builder_remote_darwin_scp -r \
            "$REMOTE_SRCDIR"/qemu-system-* \
            "$REMOTE_SRCDIR"/LINK-qemu-system-* \
            "$REMOTE_SRCDIR"/LINK-*.a \
            "$REMOTE_SRCDIR"/config-host.h \
            "$REMOTE_SRCDIR"/*/config-target.h \
            $BINARY_DIR/

        timestamp_set "$BINARY_DIR" qemu-android
    done
fi

for SYSTEM in $LOCAL_HOST_SYSTEMS; do
    build_qemu_android $SYSTEM "$AOSP_DIR"
done

log "Done building qemu-android."
