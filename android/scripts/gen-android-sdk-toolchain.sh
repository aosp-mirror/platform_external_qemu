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
shell_import utils/option_parser.shi

PROGRAM_PARAMETERS="<install-dir>"

PROGRAM_DESCRIPTION=\
"Generate a special toolchain wrapper that can be used to build binaries to be
included with the Android SDK. These binaries are guaranteed to run on systems
older than the build machine.

This is achieved by using the prebuilt toolchains found under:

  \$AOSP/prebuilts/gcc/<system>/host/

Where \$AOSP is the path to an AOSP checkout, and <system> corresponds to the
host system this script runs on (i.e. linux-x86 or darwin-x86).

If you have the 'ccache' program installed, the wrapper will use it
automatically unless you use the --no-ccache option.

The script will put under <install-dir>/ various tools, e.g.
'x86_64-linux-c++' for the 64-bit Linux compiler.

You can use the --print=<tool> option to print the corresponding name
or path, *instead* of creating a toolchain. Valid values for <tool> are:

   binprefix       -> Print the binprefix (e.g. 'x86_64-linux-')
   cc              -> Print the compiler name (e.g. 'x86_64_linux-cc')
   c++             -> Print the c++ compiler name.
   ld, ar, as, ... -> Same for other tools."

aosp_dir_register_option

OPT_HOST=
option_register_var "--host=<system>" OPT_HOST "Host system to support."

OPT_BINPREFIX=
option_register_var "--binprefix=<prefix>" OPT_BINPREFIX "Specify toolchain binprefix [autodetected]"

OPT_PREFIX=
option_register_var "--prefix=<prefix>" OPT_PREFIX "Specify extra include/lib prefix."

OPT_PRINT=
option_register_var "--print=<tool>" OPT_PRINT "Print the exact name of a specific tool."

OPT_CCACHE=
option_register_var "--ccache=<program>" OPT_CCACHE "Use specific ccache program."

OPT_NO_CCACHE=
option_register_var "--no-ccache" OPT_NO_CCACHE "Don't try to probe and use ccache."

option_parse "$@"

if [ "$PARAMETER_COUNT" != 1 ]; then
    panic "This script requires a single parameter! See --help."
fi

INSTALL_DIR=$PARAMETER_1

# Save original PATH for later.
ORIGINAL_PATH=$PATH

# Determine host system type.
BUILD_HOST=$(get_build_os)
BUILD_ARCH=$(get_build_arch)
BUILD_HOST_TAG=${BUILD_HOST}-${BUILD_ARCH}
log "Found current build machine: $BUILD_HOST_TAG"

# Handle CCACHE related arguments.
CCACHE=
if [ "$OPT_CCACHE" ]; then
    if [ "$OPT_NO_CCACHE" ]; then
        panic "You cannot use both --ccache=<program> and --no-ccache at the same time."
    fi
    CCACHE=$(find_program "$OPT_CCACHE")
    if [ -z "$CCACHE" ]; then
        panic "Missing ccache program: $OPT_CCACHE"
    fi
elif [ -z "$OPT_NO_CCACHE" ]; then
    CCACHE=$(find_program ccache)
    if [ "$CCACHE" ]; then
        log "Auto-config: --ccache=$CCACHE"
    fi
fi

aosp_dir_parse_option

# Handle --host option.
if [ "$OPT_HOST" ]; then
    case $OPT_HOST in
        linux-*|darwin-*|windows-*)
            ;;
        *)
            panic "Invalid --host value: $OPT_HOST"
            ;;
    esac
    case $OPT_HOST in
        *-x86|*-x86_64)
            ;;
        *)
            panic "Invalid --host value: $OPT_HOST"
            ;;
    esac
    HOST=$OPT_HOST
else
    HOST=$BUILD_HOST_TAG
    log "Auto-config: --host=$HOST"
fi

# Handle --binprefix option.
BINPREFIX=
if [ "$OPT_BINPREFIX" ]; then
    # Allow a final - after the binprefix
    BINPREFIX=${OPT_BINPREFIX%%-}
    if [ "$BINPREFIX" ]; then
        BINPREFIX=${BINPREFIX}-
    fi
fi

# Generate a small toolchain wrapper program
#
# $1: program name, without any prefix (e.g. gcc, g++, ar, etc..)
# $2: source prefix (e.g. 'i586-mingw32msvc-')
# $3: destination prefix (e.g. 'i586-px-mingw32msvc-')
# $4: destination directory for the generated program
#
# You may also define the following variables to pass extra tool flags:
#
#    EXTRA_CFLAGS
#    EXTRA_CXXFLAGS
#    EXTRA_LDFLAGS
#    EXTRA_ARFLAGS
#    EXTRA_ASFLAGS
#    EXTRA_WINDRESFLAGS
#
# As well as a special variable containing commands to setup the
# environment before tool invokation:
#
#    EXTRA_ENV_SETUP
#
gen_wrapper_program ()
{
    local PROG="$1"
    local SRC_PREFIX="$2"
    local DST_PREFIX="$3"
    local DST_FILE="$4/${SRC_PREFIX}$PROG"
    local FLAGS=""
    local LDFLAGS=""

    case $PROG in
      cc|gcc|cpp)
          FLAGS=$FLAGS" $EXTRA_CFLAGS"
          ;;
      c++|g++)
          FLAGS=$FLAGS" $EXTRA_CXXFLAGS"
          ;;
      ar) FLAGS=$FLAGS" $EXTRA_ARFLAGS";;
      as) FLAGS=$FLAGS" $EXTRA_ASFLAGS";;
      ld|ld.bfd|ld.gold) FLAGS=$FLAGS" $EXTRA_LDFLAGS";;
      windres) FLAGS=$FLAGS" $EXTRA_WINDRESFLAGS";;
    esac

    if [ "$CCACHE" ]; then
        DST_PREFIX="$CCACHE $DST_PREFIX"
    fi

    cat > "$DST_FILE" << EOF
#!/bin/sh
# Auto-generated by $(program_name), DO NOT EDIT!!

# Environment setup
$EXTRA_ENV_SETUP

# Tool invokation.
${DST_PREFIX}$PROG $FLAGS "\$@" $LDFLAGS
EOF
    chmod +x "$DST_FILE"
    log "  Generating: ${SRC_PREFIX}$PROG"
}

# $1: source prefix
# $2: destination prefix
# $3: destination directory.
gen_wrapper_toolchain () {
    local SRC_PREFIX="$1"
    local DST_PREFIX="$2"
    local DST_DIR="$3"
    local PROG
    local PROGRAMS="cc gcc c++ g++ cpp as ld ar ranlib strip strings nm objdump objcopy dlltool"

    log "Generating toolchain wrappers in: $DST_DIR"
    run mkdir -p "$DST_DIR"

    if [ -n "$SRC_PREFIX" ]; then
        SRC_PREFIX=${SRC_PREFIX%%-}-
    fi

    case $SRC_PREFIX in
        *mingw*)
            PROGRAMS="$PROGRAMS windres"
            case $CURRENT_HOST in
                windows-x86)
                    EXTRA_WINDRESFLAGS="--target=pe-i386"
                    ;;
            esac
            ;;
    esac

    if [ "$CCACHE" ]; then
        # If this is clang, disable ccache-induced warnings and
        # restore colored diagnostics.
        # http://petereisentraut.blogspot.fr/2011/05/ccache-and-clang.html
        if (${DST_PREFIX}gcc --version 2>/dev/null | grep -q clang); then
            EXTRA_CLANG_FLAGS="-Qunused-arguments -fcolor-diagnostics"
            EXTRA_CFLAGS="$EXTRA_CFLAGS $EXTRA_CLANG_FLAGS"
            EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS $EXTRA_CLANG_FLAGS"
        fi
    fi

    for PROG in $PROGRAMS; do
        gen_wrapper_program $PROG "$SRC_PREFIX" "$DST_PREFIX" "$DST_DIR"
    done

    EXTRA_CFLAGS=
    EXTRA_CXXFLAGS=
    EXTRA_LDFLAGS=
    EXTRA_ARFLAGS=
    EXTRA_ASFLAGS=
    EXTRA_WINDRESFLAGS=
    EXTRA_ENV_SETUP=
}

# Prepare the build for a given host system.
# $1: Host system tag (e.g. linux-x86_64)
prepare_build_for_host () {
    local CURRENT_HOST=$1

    CURRENT_TEXT="[$CURRENT_HOST]"

    PREBUILT_TOOLCHAIN_DIR=
    TOOLCHAIN_PREFIX=
    EXTRA_ENV_SETUP=
    PREBUILT_TOOLCHAIN_DIR=$AOSP_DIR/$(aosp_prebuilt_toolchain_subdir_for $CURRENT_HOST)
    TOOLCHAIN_PREFIX=$(aosp_prebuilt_toolchain_prefix_for $CURRENT_HOST)
    case $CURRENT_HOST in
        darwin-*)
            # Ensure we use the 10.8 SDK or else.
            OSX_VERSION=$(sw_vers -productVersion)
            OSX_SDK_SUPPORTED="10.6 10.7 10.8"
            OSX_SDK_INSTALLED_LIST=$(xcodebuild -showsdks 2>/dev/null | grep macosx | sed -e "s/.*macosx//g" | sort -n | tr '\n' ' ')
            if [ -z "$OSX_SDK_INSTALLED_LIST" ]; then
                panic "Please install XCode on this machine!"
            fi
            log "OSX: Installed SDKs: $OSX_SDK_INSTALLED_LIST"

            OSX_SDK_VERSION=$(echo "$OSX_SDK_INSTALLED_LIST" | tr ' ' '\n' | head -1)
            log "OSX: Using SDK version $OSX_SDK_VERSION"

            XCODE_PATH=$(xcode-select -print-path 2>/dev/null)
            log "OSX: XCode path: $XCODE_PATH"

            OSX_SDK_ROOT=$XCODE_PATH/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk
            log "OSX: Looking for $OSX_SDK_ROOT"
            if [ ! -d "$OSX_SDK_ROOT" ]; then
                OSX_SDK_ROOT=/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk
                log "OSX: Looking for $OSX_SDK_ROOT"
                if [ ! -d "$OSX_SDK_ROOT" ]; then
                    panic "Could not find SDK $OSX_SDK_VERSION at $OSX_SDK_ROOT"
                fi
            fi
            log "OSX: Using SDK at $OSX_SDK_ROOT"
            EXTRA_ENV_SETUP="export SDKROOT=$OSX_SDK_ROOT"
            ;;
    esac

    if [ -z "$OPT_BINPREFIX" ]; then
        BINPREFIX=$TOOLCHAIN_PREFIX
    fi

    case $CURRENT_HOST in
        linux-x86_64)
            GNU_CONFIG_HOST=x86_64-linux
            ;;
        linux-x86)
            GNU_CONFIG_HOST=i686-linux
            ;;
        windows-x86)
            GNU_CONFIG_HOST=i686-w64-mingw32
            ;;
        windows-x86_64)
            GNU_CONFIG_HOST=x86_64-w64-mingw32
            ;;
        darwin-*)
            # Use host compiler.
            GNU_CONFIG_HOST=
            ;;
        *)
            panic "Host system '$CURRENT_HOST' is not supported by this script!"
            ;;
    esac

    if [ "$OPT_BINPREFIX" ]; then
        BINPREFIX=${OPT_BINPREFIX%%-}
    else
        BINPREFIX=$GNU_CONFIG_HOST
    fi

    case $CURRENT_HOST in
        *-x86)
            EXTRA_CFLAGS="-m32"
            EXTRA_CXXFLAGS="-m32"
            EXTRA_LDFLAGS="-m32"
            ;;
        *-x86_64)
            EXTRA_CFLAGS="-m64"
            EXTRA_CXXFLAGS="-m64"
            EXTRA_LDFLAGS="-m64"
            ;;
        *)
            panic "Host system '$CURRENT_HOST' is not supported by this script!"
            ;;
    esac

    CROSS_PREFIX=${GNU_CONFIG_HOST}-

    PATH=$ORIGINAL_PATH

    if [ "$OPT_PREFIX" ]; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS -I$OPT_PREFIX/include"
        EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -I$OPT_PREFIX/include"
        EXTRA_LDFLAGS="$EXTRA_LDFLAGS -L$OPT_PREFIX/lib"
    fi

    if [ "$OPT_PRINT" ]; then
        # If $BINPREFIX is not empty, ensure it has a trailing -.
        if [ "$BINPREFIX" ]; then
            BINPREFIX=${BINPREFIX%%-}-
        fi
        case $OPT_PRINT in
            binprefix)
                printf "%s\n" "$BINPREFIX"
                ;;
            *)
                printf "%s\n" "${BINPREFIX}$OPT_PRINT"
                ;;
        esac
    else
        if [ "$BINPREFIX" ]; then
            log "$CURRENT_TEXT Generating ${BINPREFIX%%-} wrapper toolchain in $INSTALL_DIR"
        else
            log "$CURRENT_TEXT Generating host wrapper toolchain in $INSTALL_DIR"
        fi
        gen_wrapper_toolchain "$BINPREFIX" "$PREBUILT_TOOLCHAIN_DIR/bin/$TOOLCHAIN_PREFIX" "$INSTALL_DIR"

        # Create pkgconfig link for other scripts.
        case $CURRENT_HOST in
            linux-*)
                ln -sf "$PREBUILT_TOOLCHAIN_DIR"/sysroot/usr/lib/pkgconfig "$INSTALL_DIR"/pkgconfig
                ;;
        esac
    fi
}

prepare_build_for_host $HOST
