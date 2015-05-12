#!/bin/sh
#
# this script is used to rebuild the Android emulator from sources
# in the current directory. It also contains logic to speed up the
# rebuild if it detects that you're using the Android build system
#
# here's the list of environment variables you can define before
# calling this script to control it (besides options):
#
#

# first, let's see which system we're running this on
cd `dirname $0`

# source common functions definitions
. android/build/common.sh

# Parse options
OPTION_DEBUG=no
OPTION_IGNORE_AUDIO=no
OPTION_AOSP_PREBUILTS_DIR=
OPTION_OUT_DIR=
OPTION_HELP=no
OPTION_STRIP=no
OPTION_MINGW=no

GLES_SUPPORT=no

PCBIOS_PROBE=yes

HOST_CC=${CC:-gcc}
OPTION_CC=

AOSP_PREBUILTS_DIR=$(dirname "$0")/../../prebuilts
if [ -d "$AOSP_PREBUILTS_DIR" ]; then
    AOSP_PREBUILTS_DIR=$(cd "$AOSP_PREBUILTS_DIR" && pwd -P 2>/dev/null)
else
    AOSP_PREBUILTS_DIR=
fi

for opt do
  optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
  case "$opt" in
  --help|-h|-\?) OPTION_HELP=yes
  ;;
  --verbose)
    if [ "$VERBOSE" = "yes" ] ; then
        VERBOSE2=yes
    else
        VERBOSE=yes
    fi
  ;;
  --verbosity=*)
    if [ "$optarg" -gt 1 ]; then
        VERBOSE=yes
        if [ "$optarg" -gt 2 ]; then
            VERBOSE2=yes
        fi
    fi
    ;;

  --debug) OPTION_DEBUG=yes
  ;;
  --mingw) OPTION_MINGW=yes
  ;;
  --cc=*) OPTION_CC="$optarg"
  ;;
  --strip) OPTION_STRIP=yes
  ;;
  --no-strip) OPTION_STRIP=no
  ;;
  --out-dir=*) OPTION_OUT_DIR=$optarg
  ;;
  --aosp-prebuilts-dir=*) OPTION_AOSP_PREBUILTS_DIR=$optarg
  ;;
  --build-qemu-android) true # Ignored, used by android-rebuild.sh only.
  ;;
  --no-pcbios) PCBIOS_PROBE=no
  ;;
  --no-tests)
  # Ignore this option, only used by android-rebuild.sh
  ;;
  *)
    echo "unknown option '$opt', use --help"
    exit 1
  esac
done

# Print the help message
#
if [ "$OPTION_HELP" = "yes" ] ; then
    cat << EOF

Usage: rebuild.sh [options]
Options: [defaults in brackets after descriptions]
EOF
    echo "Standard options:"
    echo "  --help                      Print this message"
    echo "  --cc=PATH                   Specify C compiler [$HOST_CC]"
    echo "  --strip                     Strip emulator executables."
    echo "  --no-strip                  Do not strip emulator executables (default)."
    echo "  --debug                     Enable debug (-O0 -g) build"
    echo "  --aosp-prebuilts-dir=<path> Use specific prebuilt toolchain root directory [$AOSP_PREBUILTS_DIR]"
    echo "  --out-dir=<path>            Use specific output directory [objs/]"
    echo "  --mingw                     Build Windows executable on Linux"
    echo "  --verbose                   Verbose configuration"
    echo "  --debug                     Build debug version of the emulator"
    echo "  --no-pcbios                 Disable copying of PC Bios files"
    echo "  --no-tests                  Don't run unit test suite"
    if [ "$IN_ANDROID_REBUILD_SH" ]; then
        echo "  --build-qemu-android        Also build qemu-android binaries"
    fi
    echo ""
    exit 1
fi

if [ "$OPTION_AOSP_PREBUILTS_DIR" ]; then
    if [ ! -d "$OPTION_AOSP_PREBUILTS_DIR"/gcc -a \
         ! -d "$OPTION_AOSP_PREBUILTS_DIR"/clang ]; then
        echo "ERROR: Prebuilts directory does not exist: $OPTION_AOSP_PREBUILTS_DIR/gcc"
        exit 1
    fi
    AOSP_PREBUILTS_DIR=$OPTION_AOSP_PREBUILTS_DIR
fi

if [ "$OPTION_OUT_DIR" ]; then
    OUT_DIR="$OPTION_OUT_DIR"
    mkdir -p "$OUT_DIR" || panic "Could not create output directory: $OUT_DIR"
else
    OUT_DIR=objs
    log "Auto-config: --out-dir=objs"
fi

# For OS X, detect the location of the SDK to use.
# NOTE: This must happen before probing the host toolchain, because our
#       prebuilt Clang one depends on runtime object files like crt1.10.6.o
#       provided by the XCode SDK.
if [ "$HOST_OS" = darwin ]; then
    OSX_VERSION=$(sw_vers -productVersion)
    OSX_SDK_SUPPORTED="10.8 10.7 10.6"  # in order of preference
    OSX_SDK_INSTALLED_LIST=$(xcodebuild -showsdks 2>/dev/null | \
            grep macosx | sed -e "s/.*macosx//g" | sort -n | tr '\n' ' ')
    if [ -z "$OSX_SDK_INSTALLED_LIST" ]; then
        echo "ERROR: Please install XCode on this machine!"
        exit 1
    fi
    log "OSX: Installed SDKs: $OSX_SDK_INSTALLED_LIST"

    OSX_SDK_VERSION=

    # compare each SDK version supported with each SDK version installed
    for SUPPORTED_VERSION in $OSX_SDK_SUPPORTED; do
        for INSTALLED_VERSION in $OSX_SDK_INSTALLED_LIST; do
            if [ "$SUPPORTED_VERSION" = "$INSTALLED_VERSION" ]; then
                # use the first match found
                OSX_SDK_VERSION=$INSTALLED_VERSION
                break
            fi
        done
        if [ "$OSX_SDK_VERSION" ]; then
            break
        fi
    done

    if [ "$OSX_SDK_VERSION" ]; then
        log "OSX: Using SDK version $OSX_SDK_VERSION"
    else
        echo "ERROR: Only OSX SDK $OSX_SDK_SUPPORTED are supported and this machine has $OSX_SDK_VERSION."
        echo "Please install Xcode 5 on this machine (If you have Xcode 6 installed, downgrade to Xcode 5)"
        exit 1
    fi

    XCODE_PATH=$(xcode-select -print-path 2>/dev/null)
    log "OSX: XCode path: $XCODE_PATH"

    OSX_SDK_ROOT=$XCODE_PATH/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk
    log "OSX: Looking for $OSX_SDK_ROOT"
    if [ ! -d "$OSX_SDK_ROOT" ]; then
        OSX_SDK_ROOT=/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk
        log "OSX: Looking for $OSX_SDK_ROOT"
        if [ ! -d "$OSX_SDK_ROOT" ]; then
            echo "ERROR: Could not find SDK $OSX_SDK_VERSION at $OSX_SDK_ROOT"
            exit 1
        fi
    fi
    echo "OSX SDK   : Found at $OSX_SDK_ROOT"
fi

CCACHE=
if [ "$USE_CCACHE" != 0 ]; then
    CCACHE=$(which ccache 2>/dev/null || true)
fi

if [ -n "$CCACHE" -a -f "$CCACHE" ]; then
    if [ "$HOST_OS" == "darwin" -a "$OPTION_DEBUG" == "yes" ]; then
        # http://llvm.org/bugs/show_bug.cgi?id=20297
        # ccache works for mingw/gdb, therefore probably works for gcc/gdb
        log "Prebuilt   : CCACHE disabled for OSX debug builds"
        CCACHE=
    else
        log "Prebuilt   : CCACHE=$CCACHE"
    fi
else
    log "Prebuilt   : CCACHE can't be found"
    CCACHE=
fi

# On Linux, try to use our prebuilt toolchain to generate binaries
# that are compatible with Ubuntu 10.4
if [ -z "$CC" -a -z "$OPTION_CC" ] ; then
    GEN_SDK=$PROGDIR/android/scripts/gen-android-sdk-toolchain.sh
    GEN_SDK_FLAGS=
    if [ "$CCACHE" ]; then
        GEN_SDK_FLAGS="$GEN_SDK_FLAGS --ccache=$CCACHE"
    else
        GEN_SDK_FLAGS="$GEN_SDK_FLAGS --no-ccache"
    fi
    GEN_SDK_FLAGS="$GEN_SDK_FLAGS --aosp-dir=$AOSP_PREBUILTS_DIR/.."
    $GEN_SDK $GEN_SDK_FLAGS $OUT_DIR/toolchain || panic "Cannot generate SDK toolchain!"
    CC="$OUT_DIR/toolchain/"$($GEN_SDK $GEN_SDK_FLAGS --print=gcc $OUT_DIR/toolchain)
fi

if [ -n "$OPTION_CC" ]; then
    echo "Using specified C compiler: $OPTION_CC"
    CC="$OPTION_CC"
fi

if [ -z "$CC" ]; then
  CC=$HOST_CC
fi

# By default, generate 32-bit binaries, the Makefile have targets that
# generate 64-bit ones by using -m64 on the command-line.
force_32bit_binaries

case $OS in
    linux-*)
        TARGET_DLL_SUFFIX=.so
        ;;
    darwin-*)
        TARGET_DLL_SUFFIX=.dylib
        ;;
    windows*)
        TARGET_DLL_SUFFIX=.dll
esac

TARGET_OS=$OS

setup_toolchain

BUILD_AR=$AR
BUILD_CC=$CC
BUILD_CXX=$CC
BUILD_LD=$LD
BUILD_AR=$AR
BUILD_CFLAGS=$CFLAGS
BUILD_CXXFLAGS=$CXXFLAGS
BUILD_LDFLAGS=$LDFLAGS

if [ "$OPTION_MINGW" = "yes" ] ; then
    enable_linux_mingw
    TARGET_OS=windows
    TARGET_DLL_SUFFIX=.dll
else
    enable_cygwin
fi

# Are we running in the Android build system ?
check_android_build


# Adjust a few things when we're building within the Android build
# system:
#    - locate prebuilt directory
#    - locate and use prebuilt libraries
#    - copy the new binary to the correct location
#
# Try to find the GLES emulation headers and libraries automatically
GLES_DIR=distrib/android-emugl
if [ ! -d "$GLES_DIR" ]; then
    panic "GLES       : Could not find GPU emulation sources!: $GLES_DIR"
else
    echo "GLES       : Found GPU emulation sources: $GLES_DIR"
fi

if [ "$PCBIOS_PROBE" = "yes" ]; then
    PCBIOS_DIR=$AOSP_PREBUILTS_DIR/qemu-kernel/x86/pc-bios
    if [ ! -d "$PCBIOS_DIR" ]; then
        log2 "PC Bios    : Probing $PCBIOS_DIR (missing)"
        PCBIOS_DIR=../pc-bios
    fi
    log2 "PC Bios    : Probing $PCBIOS_DIR"
    if [ ! -d "$PCBIOS_DIR" ]; then
        log "PC Bios    : Could not find prebuilts directory."
    else
        mkdir -p $OUT_DIR/lib/pc-bios
        for BIOS_FILE in bios.bin vgabios-cirrus.bin; do
            log "PC Bios    : Copying $BIOS_FILE"
            cp -f $PCBIOS_DIR/$BIOS_FILE $OUT_DIR/lib/pc-bios/$BIOS_FILE
        done
    fi
fi

# we can build the emulator with Cygwin, so enable it
enable_cygwin

setup_toolchain

###
###  Audio subsystems probes
###
PROBE_COREAUDIO=no
PROBE_ALSA=no
PROBE_OSS=no
PROBE_ESD=no
PROBE_PULSEAUDIO=no
PROBE_WINAUDIO=no

case "$TARGET_OS" in
    darwin*) PROBE_COREAUDIO=yes;
    ;;
    linux-*) PROBE_ALSA=yes; PROBE_OSS=yes; PROBE_ESD=yes; PROBE_PULSEAUDIO=yes;
    ;;
    freebsd-*) PROBE_OSS=yes;
    ;;
    windows) PROBE_WINAUDIO=yes
    ;;
esac

# create the objs directory that is going to contain all generated files
# including the configuration ones
#
mkdir -p $OUT_DIR

###
###  Compiler probe
###

####
####  Host system probe
####

# because the previous version could be read-only
clean_temp

# check host endianess
#
HOST_BIGENDIAN=no
if [ "$TARGET_OS" = "$OS" ] ; then
cat > $TMPC << EOF
#include <inttypes.h>
int main(int argc, char ** argv){
        volatile uint32_t i=0x01234567;
        return (*((uint8_t*)(&i))) == 0x01;
}
EOF
feature_run_exec HOST_BIGENDIAN
fi

# check whether we have <byteswap.h>
#
feature_check_header HAVE_BYTESWAP_H      "<byteswap.h>"
feature_check_header HAVE_MACHINE_BSWAP_H "<machine/bswap.h>"
feature_check_header HAVE_FNMATCH_H       "<fnmatch.h>"

# check for Mingw version.
MINGW_VERSION=
if [ "$TARGET_OS" = "windows" ]; then
log "Mingw      : Probing for GCC version."
GCC_VERSION=$($CC -v 2>&1 | awk '$1 == "gcc" && $2 == "version" { print $3; }')
GCC_MAJOR=$(echo "$GCC_VERSION" | cut -f1 -d.)
GCC_MINOR=$(echo "$GCC_VERSION" | cut -f2 -d.)
log "Mingw      : Found GCC version $GCC_MAJOR.$GCC_MINOR [$GCC_VERSION]"
MINGW_GCC_VERSION=$(( $GCC_MAJOR * 100 + $GCC_MINOR ))
fi
# Build the config.make file
#

case $OS in
    windows)
        HOST_EXEEXT=.exe
        HOST_DLLEXT=.dll
        ;;
    darwin)
        HOST_EXEEXT=
        HOST_DLLEXT=.dylib
        ;;
    *)
        HOST_EXEEXT=
        HOST_DLLEXT=
        ;;
esac

case $TARGET_OS in
    windows)
        TARGET_EXEEXT=.exe
        TARGET_DLLEXT=.dll
        ;;
    darwin)
        TARGET_EXEXT=
        TARGET_DLLEXT=.dylib
        ;;
    *)
        TARGET_EXEEXT=
        TARGET_DLLEXT=.so
        ;;
esac

# Strip executables and shared libraries when needed.
if [ "$OPTION_DEBUG" != "yes" -a "$OPTION_STRIP" = "yes" ]; then
    case $HOST_OS in
        darwin)
            LDFLAGS="$LDFLAGS -Wl,-S"
            ;;
        *)
            LDFLAGS="$LDFLAGS -Wl,--strip-all"
            ;;
    esac
fi

create_config_mk "$OUT_DIR"
echo "" >> $config_mk
echo "HOST_PREBUILT_TAG := $TARGET_OS" >> $config_mk
echo "HOST_EXEEXT       := $TARGET_EXEEXT" >> $config_mk
echo "HOST_DLLEXT       := $TARGET_DLLEXT" >> $config_mk
echo "PREBUILT          := $ANDROID_PREBUILT" >> $config_mk
echo "PREBUILTS         := $ANDROID_PREBUILTS" >> $config_mk

echo "" >> $config_mk
echo "BUILD_OS          := $HOST_OS" >> $config_mk
echo "BUILD_ARCH        := $HOST_ARCH" >> $config_mk
echo "BUILD_EXEEXT      := $HOST_EXEEXT" >> $config_mk
echo "BUILD_DLLEXT      := $HOST_DLLEXT" >> $config_mk
echo "BUILD_AR          := $BUILD_AR" >> $config_mk
echo "BUILD_CC          := $BUILD_CC" >> $config_mk
echo "BUILD_CXX         := $BUILD_CXX" >> $config_mk
echo "BUILD_LD          := $BUILD_LD" >> $config_mk
echo "BUILD_CFLAGS      := $BUILD_CFLAGS" >> $config_mk
echo "BUILD_LDFLAGS     := $BUILD_LDFLAGS" >> $config_mk

PWD=`pwd`
echo "SRC_PATH          := $PWD" >> $config_mk
echo "CONFIG_COREAUDIO  := $PROBE_COREAUDIO" >> $config_mk
echo "CONFIG_WINAUDIO   := $PROBE_WINAUDIO" >> $config_mk
echo "CONFIG_ESD        := $PROBE_ESD" >> $config_mk
echo "CONFIG_ALSA       := $PROBE_ALSA" >> $config_mk
echo "CONFIG_OSS        := $PROBE_OSS" >> $config_mk
echo "CONFIG_PULSEAUDIO := $PROBE_PULSEAUDIO" >> $config_mk
echo "BUILD_STANDALONE_EMULATOR := true" >> $config_mk
if [ $OPTION_DEBUG = yes ] ; then
    echo "BUILD_DEBUG_EMULATOR := true" >> $config_mk
fi
echo "EMULATOR_BUILD_EMUGL       := true" >> $config_mk
echo "EMULATOR_EMUGL_SOURCES_DIR := $GLES_DIR" >> $config_mk

if [ -n "$ANDROID_SDK_TOOLS_REVISION" ] ; then
    echo "ANDROID_SDK_TOOLS_REVISION := $ANDROID_SDK_TOOLS_REVISION" >> $config_mk
fi

if [ "$OPTION_MINGW" = "yes" ] ; then
    echo "" >> $config_mk
    echo "USE_MINGW := 1" >> $config_mk
    echo "HOST_OS   := windows" >> $config_mk
    echo "HOST_MINGW_VERSION := $MINGW_GCC_VERSION" >> $config_mk
fi

if [ "$HOST_OS" = "darwin" ]; then
    echo "mac_sdk_root := $OSX_SDK_ROOT" >> $config_mk
    echo "mac_sdk_version := $OSX_SDK_VERSION" >> $config_mk
fi

# Build the config-host.h file
#
config_h=$OUT_DIR/config-host.h
cat > $config_h <<EOF
/* This file was autogenerated by '$PROGNAME' */

#define CONFIG_QEMU_SHAREDIR   "/usr/local/share/qemu"

EOF

if [ "$HAVE_BYTESWAP_H" = "yes" ] ; then
  echo "#define CONFIG_BYTESWAP_H 1" >> $config_h
fi
if [ "$HAVE_MACHINE_BYTESWAP_H" = "yes" ] ; then
  echo "#define CONFIG_MACHINE_BSWAP_H 1" >> $config_h
fi
if [ "$HAVE_FNMATCH_H" = "yes" ] ; then
  echo "#define CONFIG_FNMATCH  1" >> $config_h
fi
echo "#define CONFIG_GDBSTUB  1" >> $config_h
echo "#define CONFIG_SLIRP    1" >> $config_h
echo "#define CONFIG_SKINS    1" >> $config_h
echo "#define CONFIG_TRACE    1" >> $config_h

case "$TARGET_OS" in
    windows)
        echo "#define CONFIG_WIN32  1" >> $config_h
        ;;
    *)
        echo "#define CONFIG_POSIX  1" >> $config_h
        ;;
esac

case "$TARGET_OS" in
    linux-*)
        echo "#define CONFIG_KVM_GS_RESTORE 1" >> $config_h
        ;;
esac

# only Linux has fdatasync()
case "$TARGET_OS" in
    linux-*)
        echo "#define CONFIG_FDATASYNC    1" >> $config_h
        ;;
esac

case "$TARGET_OS" in
    linux-*|darwin-*)
        echo "#define CONFIG_MADVISE  1" >> $config_h
        ;;
esac

# the -nand-limits options can only work on non-windows systems
if [ "$TARGET_OS" != "windows" ] ; then
    echo "#define CONFIG_NAND_LIMITS  1" >> $config_h
fi
echo "#define QEMU_VERSION    \"0.10.50\"" >> $config_h
echo "#define QEMU_PKGVERSION \"Android\"" >> $config_h
case "$CPU" in
    x86) CONFIG_CPU=I386
    ;;
    ppc) CONFIG_CPU=PPC
    ;;
    x86_64) CONFIG_CPU=X86_64
    ;;
    *) CONFIG_CPU=$CPU
    ;;
esac
if [ "$HOST_BIGENDIAN" = "1" ] ; then
  echo "#define HOST_WORDS_BIGENDIAN 1" >> $config_h
fi
BSD=0
case "$TARGET_OS" in
    linux-*) CONFIG_OS=LINUX
    ;;
    darwin-*) CONFIG_OS=DARWIN
              BSD=1
    ;;
    freebsd-*) CONFIG_OS=FREEBSD
              BSD=1
    ;;
    windows*) CONFIG_OS=WIN32
    ;;
    *) CONFIG_OS=$OS
esac

case $TARGET_OS in
    linux-*|darwin-*)
        echo "#define CONFIG_IOVEC 1" >> $config_h
        ;;
esac

echo "#define CONFIG_$CONFIG_OS   1" >> $config_h
if [ $BSD = 1 ] ; then
    echo "#define CONFIG_BSD       1" >> $config_h
    echo "#define O_LARGEFILE      0" >> $config_h
    echo "#define MAP_ANONYMOUS    MAP_ANON" >> $config_h
fi

case "$TARGET_OS" in
    linux-*)
        echo "#define CONFIG_SIGNALFD       1" >> $config_h
        ;;
esac

echo "#define CONFIG_ANDROID       1" >> $config_h

log "Generate   : $config_h"

# Generate the QAPI headers and sources from qapi-schema.json
# Ideally, this would be done in our Makefiles, but as far as I
# understand, the platform build doesn't support a single tool
# that generates several sources files, nor the standalone one.
export PYTHONDONTWRITEBYTECODE=1
AUTOGENERATED_DIR=qapi-auto-generated
python scripts/qapi-types.py qapi.types --output-dir=$AUTOGENERATED_DIR -b < qapi-schema.json
python scripts/qapi-visit.py --output-dir=$AUTOGENERATED_DIR -b < qapi-schema.json
python scripts/qapi-commands.py --output-dir=$AUTOGENERATED_DIR -m < qapi-schema.json
log "Generate   : $AUTOGENERATED_DIR"

clean_temp

echo "Ready to go. Type 'make' to build emulator"
