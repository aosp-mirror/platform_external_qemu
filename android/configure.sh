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
PROGNAME=`basename $0`
PROGDIR=`dirname $0`
CURR_SHELL=$(ps -o comm= -p $$)
## Logging support
##
VERBOSE=yes
VERBOSE2=no

panic () {
    echo "ERROR: $@"
    exit 1
}

log () {
    if [ "$VERBOSE" = "yes" ] ; then
        echo "$1"
    fi
}

log2 () {
    if [ "$VERBOSE2" = "yes" ] ; then
        echo "$1"
    fi
}

## Normalize OS and CPU
##

BUILD_ARCH=$(uname -m)
case "$BUILD_ARCH" in
    i?86) BUILD_ARCH=x86
    ;;
    x86_64|amd64) BUILD_ARCH=x86_64
    ;;
    *) panic "$BUILD_ARCH builds are not supported!"
    ;;
esac

log2 "BUILD_ARCH=$BUILD_ARCH"
log2 "CURR_SHELL=$CURR_SHELL"

# at this point, the supported values for CPU are:
#   x86
#   x86_64
#
# other values may be possible but haven't been tested
#

BUILD_EXEEXT=
BUILD_OS=`uname -s`
case "$BUILD_OS" in
    Darwin) BUILD_OS=darwin;;
    Linux) BUILD_OS=linux;;
    FreeBSD) BUILD_OS=freebsd;;
    CYGWIN*|*_NT-*)
        panic "Please build Windows binaries on Linux with --mingw option."
        ;;
    *) panic "Unknown build OS: $BUILD_OS";;
esac

BUILD_TAG=$BUILD_OS-$BUILD_ARCH

log2 "BUILD_TAG=$BUILD_TAG"
log2 "BUILD_EXEEXT=$BUILD_EXEEXT"

HOST_OS=$BUILD_OS
HOST_ARCH=$BUILD_ARCH
HOST_TAG=$HOST_OS-$HOST_ARCH

#### Toolchain support
####

WINDRES=

# Various probes are going to need to run a small C program
TMPC=/tmp/android-$$-test.c
TMPO=/tmp/android-$$-test.o
TMPE=/tmp/android-$$-test$BUILD_EXEEXT
TMPL=/tmp/android-$$-test.log

# cleanup temporary files
clean_temp () {
    rm -f $TMPC $TMPO $TMPL $TMPLE
}

# cleanup temp files then exit with an error
clean_exit () {
    clean_temp
    exit 1
}

# this function will setup the compiler and linker and check that they work as advertized
setup_toolchain () {
    if [ -z "$CC" ] ; then
        CC=gcc
    fi

    # check that we can compile a trivial C program with this compiler
    cat > $TMPC <<EOF
int main(void) {}
EOF
    compile
    if [ $? != 0 ] ; then
        echo "your C compiler doesn't seem to work: $CC"
        cat $TMPL
        clean_exit
    fi
    log "CC         : compiler check ok ($CC)"
    CC_VER=`$CC --version`
    log "CC_VER     : $CC_VER"

    # check that we can link the trivial program into an executable
    if [ -z "$LD" ] ; then
        LD=$CC
    fi
    link
    if [ $? != 0 ] ; then
        echo "your linker doesn't seem to work:"
        cat $TMPL
        clean_exit
    fi
    log "LD         : linker check ok ($LD)"

    if [ -z "$AR" ]; then
        AR=ar
    fi
    log "AR         : archiver ($AR)"
    clean_temp
}

# try to compile the current source file in $TMPC into an object
# stores the error log into $TMPL
#
compile () {
    log2 "Object     : $CC -o $TMPO -c $CFLAGS $TMPC"
    $CC -o $TMPO -c $CFLAGS $TMPC 2> $TMPL
}

# try to link the recently built file into an executable. error log in $TMPL
#
link() {
    log2 "Link      : $LD -o $TMPE $TMPO $LDFLAGS"
    $LD -o $TMPE $TMPO $LDFLAGS 2> $TMPL
}

## Feature test support
##

# check that a given C header file exists on the host system
# $1: variable name which will be set to "yes" or "no" depending on result
# $2: header name
#
# you can define EXTRA_CFLAGS for extra C compiler flags
# for convenience, this variable will be unset by the function.
#
feature_check_header () {
    local result_ch OLD_CFLAGS
    log2 "HeaderCheck: $2"
    echo "#include $2" > $TMPC
    cat >> $TMPC <<EOF
        int main(void) { return 0; }
EOF
    OLD_CFLAGS=$CFLAGS
    CFLAGS="$CFLAGS $EXTRA_CFLAGS"
    compile
    if [ $? != 0 ]; then
        result_ch=no
    else
        result_ch=yes
    fi
    log "HeaderCheck: $2 [$result_ch]"
    EXTRA_CFLAGS=
    CFLAGS=$OLD_CFLAGS
    eval $1=$result_ch
    clean_temp
}

# Find pattern $1 in string $2
# This is to be used in if statements as in:
#
#    if pattern_match <pattern> <string>; then
#       ...
#    fi
#
pattern_match () {
    echo "$2" | grep -q -E -e "$1"
}

# Find if a given shell program is available.
# We need to take care of the fact that the 'which <foo>' command
# may return either an empty string (Linux) or something like
# "no <foo> in ..." (Darwin). Also, we need to redirect stderr
# to /dev/null for Cygwin
#
# $1: variable name
# $2: program name
#
# Result: set $1 to the full path of the corresponding command
#         or to the empty/undefined string if not available
#
find_program () {
    local PROG
    PROG=`which $2 2>/dev/null || true`
    if [ -n "$PROG" ] ; then
        if pattern_match '^no ' "$PROG"; then
            PROG=
        fi
    fi
    eval $1="$PROG"
}

# Default value of --ui option.
UI_DEFAULT=qt

# Default value of --gles option.
GLES_DEFAULT=dgl

# Fancy colors in the terminal
if [ -t 1 ] ; then
  RED=`tput setaf 1`
  GREEN=`tput setaf 2`
  RESET=`tput sgr0`
else
  RED=
  GREEN=
  RESET=
fi

# Parse options
OPTION_QEMU2_SRCDIR=
OPTION_DEBUG=no
OPTION_SANITIZER=no
OPTION_EMUGL_PRINTOUT=no
OPTION_IGNORE_AUDIO=no
OPTION_AOSP_PREBUILTS_DIR=
OPTION_OUT_DIR=
OPTION_HELP=no
OPTION_STRIP=yes
OPTION_CRASHUPLOAD=NONE
OPTION_MINGW=no
OPTION_WINDOWS_MSVC=no
OPTION_GLES=
OPTION_SDK_REV=
OPTION_SYMBOLS=no
OPTION_BENCHMARKS=no
OPTION_LTO=
OPTION_MIPS=
OPTION_SNAPSHOT_PROFILE=no
OPTION_MIN_BUILD=no
OPTION_AEMU64_ONLY=no
OPTION_GOLDFISH_OPENGL_DIR=no
ANDROID_SDK_TOOLS_REVISION=
ANDROID_SDK_TOOLS_BUILD_NUMBER=

GLES_SUPPORT=no

PCBIOS_PROBE=yes

HOST_CC=${CC:-gcc}
OPTION_CC=

HOST_CXX=${CXX:-g++}
OPTION_CXX=

VERBOSITY=0
OPTION_SNAPSHOT_PROFILE=0

AOSP_ROOT_DIR=$(dirname "$0")/../../../
if [ -d "$AOSP_ROOT_DIR" ]; then
    AOSP_ROOT_DIR=$(cd "$AOSP_ROOT_DIR" && pwd -P 2>/dev/null)
else
    AOSP_ROOT_DIR=
fi

AOSP_PREBUILTS_DIR=$AOSP_ROOT_DIR/prebuilts

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
    VERBOSITY="$optarg"
    ;;

  --debug) OPTION_DEBUG=yes; OPTION_STRIP=no
  ;;
  --mingw)
    # TODO(joshuaduong): deprecate building with mingw otherwise we'll have to
    # build windows emulator w/o QtWebEngine.
    if [ "$OPTION_WINDOWS_MSVC" = "yes" ] ; then
      panic "Choose either mingw or windows-msvc, not both."
    fi
    OPTION_MINGW=yes
  ;;
  --windows-msvc)
    if [ "$OPTION_MINGW" = "yes" ] ; then
      panic "Choose either mingw or windows-msvc, not both."
    fi
    OPTION_WINDOWS_MSVC=yes
  ;;
  --sanitizer=*) OPTION_SANITIZER=$optarg
  ;;
  --emugl-printout) OPTION_EMUGL_PRINTOUT=yes
  ;;
  --cc=*) OPTION_CC="$optarg"
  ;;
  --cxx=*) OPTION_CXX="$optarg"
  ;;
  --strip) OPTION_STRIP=yes
  ;;
  --no-strip) OPTION_STRIP=no
  ;;
  --crash-staging) OPTION_CRASHUPLOAD=STAGING
  ;;
  --crash-prod) OPTION_CRASHUPLOAD=PROD
  ;;
  --out-dir=*) OPTION_OUT_DIR=$optarg
  ;;
  --aosp-prebuilts-dir=*) OPTION_AOSP_PREBUILTS_DIR=$optarg
  ;;
  --qemu2-src-dir=*) OPTION_QEMU2_SRCDIR=$optarg
  ;;
  --no-pcbios) PCBIOS_PROBE=no
  ;;
  --no-tests)
  # Ignore this option, only used by android/rebuild.sh
  ;;
  --symbols) OPTION_SYMBOLS=yes
  ;;
  --no-symbols) OPTION_SYMBOLS=no
  ;;
  --gles=dgl) OPTION_GLES=dgl
  ;;
  --gles=angle) OPTION_GLES=angle
  ;;
  --gles=*) echo "Unknown --gles value, try one of: dgl angle"
  ;;
  --sdk-revision=*) ANDROID_SDK_TOOLS_REVISION=$optarg
  ;;
  --sdk-build-number=*) ANDROID_SDK_TOOLS_BUILD_NUMBER=$optarg
  ;;
  --benchmarks) OPTION_BENCHMARKS=yes
  ;;
  --lto) OPTION_LTO=true
  ;;
  --with-mips) OPTION_MIPS=true
  ;;
  --snapshot-profile) OPTION_SNAPSHOT_PROFILE=1
  ;;
  --snapshot-profile-level=*) OPTION_SNAPSHOT_PROFILE=$optarg
  ;;
  -min|--min-build) OPTION_MIN_BUILD=yes
  ;;
  -aemu64only|--aemu64-only) OPTION_AEMU64_ONLY=yes
  ;;
  -goldfish-opengl|--goldfish-opengl=*) OPTION_GOLDFISH_OPENGL_DIR=yes
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
    echo "  --cxx=PATH                  Specify C++ compiler [$HOST_CXX]"
    echo "  --no-strip                  Do not strip emulator executables."
    echo "  --strip                     Strip emulator executables (default)."
    echo "  --symbols                   Generating Breakpad symbol files."
    echo "  --no-symbols                Do not generate Breakpad symbol files (default)."
    echo "  --crash-[staging,prod]      Send crashes to specific server (no crash reporting by default)."
    echo "  --gles=dgl                  Build the OpenGLES to Desktop OpenGL Translator (default)"
    echo "  --gles=angle                Build the OpenGLES to ANGLE wrapper"
    echo "  --aosp-prebuilts-dir=<path> Use specific prebuilt toolchain root directory [$AOSP_PREBUILTS_DIR]"
    echo "  --out-dir=<path>            Use specific output directory [objs/]"
    echo "  --mingw                     Build Windows executable on Linux using mingw"
    echo "  --windows-msvc              Build Windows executable on Linux using Windows SDK + clang (only 64-bit for now)"
    echo "  --verbose                   Verbose configuration"
    echo "  --debug                     Build debug version of the emulator"
    echo "  --sanitizer=[...]           Build with LLVM sanitizer (sanitizer=[address, thread])"
    echo "  --no-pcbios                 Disable copying of PC Bios files"
    echo "  --no-tests                  Don't run unit test suite"
    echo "  --benchmarks                Build benchmark programs."
    echo "  --lto                       Force link-time optimization."
    echo "  --snapshot-profile[-level=X] Enable snapshot profiling via debug prints."
    echo "  --min[-build]               Only build the qemu2 x64 host x86 target binaries."
    echo "  --with-mips                 Build the deprecated mips emulator. This option will be removed in future versions."
    echo ""
    exit 1
fi

# Check asan support
[ "$OPTION_SANITIZER" != "no" ] && ([ "$OPTION_MINGW" = "yes" ] || [ "$OPTION_WINDOWS_MSVC" = "yes" ]) && panic "Asan is not supported under windows"

if [ "$OPTION_AOSP_PREBUILTS_DIR" ]; then
    if [ ! -d "$OPTION_AOSP_PREBUILTS_DIR"/gcc -a \
         ! -d "$OPTION_AOSP_PREBUILTS_DIR"/clang ]; then
        echo "ERROR: Prebuilts directory does not exist: $OPTION_AOSP_PREBUILTS_DIR/gcc"
        exit 1
    fi
    AOSP_PREBUILTS_DIR=$OPTION_AOSP_PREBUILTS_DIR
fi

if [ -z "$OPTION_GLES" ]; then
    OPTION_GLES=$GLES_DEFAULT
    log "Auto-config: --gles=$OPTION_GLES"
fi

if [ "$OPTION_OUT_DIR" ]; then
    OUT_DIR="$OPTION_OUT_DIR"
    mkdir -p "$OUT_DIR" || panic "Could not create output directory: $OUT_DIR"
else
    OUT_DIR=objs
    log "Auto-config: --out-dir=objs"
fi

CCACHE=
if [ "$USE_CCACHE" != 0 ]; then
    CCACHE=$(which ccache 2>/dev/null || true)
fi

if [ -n "$CCACHE" -a -f "$CCACHE" ]; then
    if [ "$HOST_OS" = "darwin" -a "$OPTION_DEBUG" = "yes" ]; then
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

# Use gen-android-sdk-toolchain.sh to generate a toolchain that will
# build binaries compatible with the SDK deployement systems.
GEN_SDK=$PROGDIR/scripts/gen-android-sdk-toolchain.sh
GEN_SDK_FLAGS=--cxx11
if [ "$CCACHE" ]; then
    GEN_SDK_FLAGS="$GEN_SDK_FLAGS --ccache=$CCACHE"
else
    GEN_SDK_FLAGS="$GEN_SDK_FLAGS --no-ccache"
fi
SDK_TOOLCHAIN_DIR=$OUT_DIR/build/toolchain
GEN_SDK_FLAGS="$GEN_SDK_FLAGS --aosp-dir=$AOSP_PREBUILTS_DIR/.."
"$GEN_SDK" $GEN_SDK_FLAGS "--verbosity=$VERBOSITY" "$SDK_TOOLCHAIN_DIR" || panic "Cannot generate SDK toolchain!"
BINPREFIX=$("$GEN_SDK" $GEN_SDK_FLAGS --print=binprefix "$SDK_TOOLCHAIN_DIR")
CC="$SDK_TOOLCHAIN_DIR/${BINPREFIX}gcc"
CXX="$SDK_TOOLCHAIN_DIR/${BINPREFIX}g++"
TIDY="$SDK_TOOLCHAIN_DIR/${BINPREFIX}clang-tidy"
AR="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ar"
RANLIB="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ranlib"
LD=$CXX
OBJCOPY="$SDK_TOOLCHAIN_DIR/${BINPREFIX}objcopy"
TOOLCHAIN_SYSROOT="$("${GEN_SDK}" $GEN_SDK_FLAGS --print=sysroot "${SDK_TOOLCHAIN_DIR}")"

if [ -n "$OPTION_CC" ]; then
    echo "Using specified C compiler: $OPTION_CC"
    CC="$OPTION_CC"
fi

if [ -n "$OPTION_CXX" ]; then
    echo "Using specified C++ compiler: $OPTION_CXX"
    CC="$OPTION_CXX"
fi

setup_toolchain

BUILD_AR=$AR
BUILD_RANLIB=$RANLIB
BUILD_CC=$CC
BUILD_CXX=$CXX
BUILD_LD=$LD
BUILD_OBJCOPY=$OBJCOPY
BUILD_CFLAGS=$CFLAGS
BUILD_CXXFLAGS=$CXXFLAGS
BUILD_LDFLAGS=$LDFLAGS
TEST_SHELL=
# Teach make to build static executables where this makes sense. Darwin doesn't
# have a sane way of linking statically against system libraries at all.
if [ "$HOST_OS" = "darwin" ]; then
  BUILD_STATIC_FLAGS=
else
  BUILD_STATIC_FLAGS="-static"
fi

if [ "$OPTION_MINGW" = "yes" ] ; then
    # Are we on Linux ?
    log "Mingw      : Checking for Linux host"
    if [ "$HOST_OS" != "linux" ] ; then
        echo "Sorry, but mingw compilation is only supported on Linux !"
        exit 1
    fi
    TEST_SHELL=wine
    WINE_CMD=$(which wine 2>/dev/null || true)
    if [ -z "$WINE_CMD" ]; then
        echo "${RED}WARNING: Wine is not installed on this machine!! Unit tests will be ignored and will not run!!${RESET}"
        TEST_SHELL="true ||"
    fi
    WIN_SDK_FLAGS="$GEN_SDK_FLAGS --host=windows-x86_64"
    "$GEN_SDK" $WIN_SDK_FLAGS "$SDK_TOOLCHAIN_DIR" || panic "Cannot generate SDK toolchain!"
    BINPREFIX=$("$GEN_SDK" $WIN_SDK_FLAGS --print=binprefix "$SDK_TOOLCHAIN_DIR")
    CC="$SDK_TOOLCHAIN_DIR/${BINPREFIX}gcc"
    CXX="$SDK_TOOLCHAIN_DIR/${BINPREFIX}g++"
    LD=$CC
    WINDRES=$SDK_TOOLCHAIN_DIR/${BINPREFIX}windres
    AR="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ar"
    OBJCOPY="$SDK_TOOLCHAIN_DIR/${BINPREFIX}objcopy"
    HOST_OS=windows
    HOST_TAG=$HOST_OS-$HOST_ARCH
fi

# Our BUILD_TARGET_OS now has two different targets for windows:
# windows and windows_msvc. BUILD_TARGET_OS_FLAVOR will be "windows"
# for both of those targets.
HOST_OS_FLAVOR=$HOST_OS

if [ "$OPTION_WINDOWS_MSVC" = "yes" ] ; then
    # Are we on Linux ?
    # TODO(joshuaduong): maybe mac too, or windows + cygwin
    log "Windows-msvc      : Checking for Linux host"
    if [ "$HOST_OS" != "linux" ] ; then
        echo "Sorry, but windows-msvc compilation is only supported on Linux !"
        exit 1
    fi
    TEST_SHELL=wine
    WINE_CMD=$(which wine 2>/dev/null || true)
    if [ -z "$WINE_CMD" ]; then
        echo "${RED}WARNING: Wine is not installed on this machine!! Unit tests will be ignored and will not run!!${RESET}"
        TEST_SHELL="true ||"
    fi
    WIN_SDK_FLAGS="$GEN_SDK_FLAGS --host=windows_msvc-x86_64"
    "$GEN_SDK" $WIN_SDK_FLAGS "$SDK_TOOLCHAIN_DIR" || panic "Cannot generate SDK toolchain!"
    BINPREFIX=$("$GEN_SDK" $WIN_SDK_FLAGS --print=binprefix "$SDK_TOOLCHAIN_DIR")
    CC="$SDK_TOOLCHAIN_DIR/${BINPREFIX}clang"
    CXX="$SDK_TOOLCHAIN_DIR/${BINPREFIX}clang++"
    LD=$CC
    WINDRES=$SDK_TOOLCHAIN_DIR/${BINPREFIX}windres
    AR="$SDK_TOOLCHAIN_DIR/${BINPREFIX}ar"
    OBJCOPY="$SDK_TOOLCHAIN_DIR/${BINPREFIX}objcopy"
    HOST_OS=windows_msvc
    HOST_OS_FLAVOR=windows
    HOST_TAG=$HOST_OS-$HOST_ARCH
fi

# Try to find the GLES emulation headers and libraries automatically
GLES_DIR=android/android-emugl
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
        mkdir -p $OUT_DIR/lib/pc-bios || panic "Could not create dir $OUT_DIR/lib/pc-bios"
        for BIOS_FILE in bios.bin vgabios-cirrus.bin vgabios-virtio.bin bios-256k.bin efi-virtio.rom kvmvapic.bin linuxboot.bin; do
            log "PC Bios    : Copying $BIOS_FILE"
            cp -f $PCBIOS_DIR/$BIOS_FILE $OUT_DIR/lib/pc-bios/$BIOS_FILE ||
                panic "Missing BIOS file: $PCBIOS_DIR/$BIOS_FILE"
        done
        # The following BIOS files are only required by QEMU 2.7.0, move them to the non-optional
        # list above once the big rebasing has been completed.
        for BIOS_FILE in linuxboot_dma.bin; do
            if [ ! -f "$PCBIOS_DIR/$BIOS_FILE" ]; then
                log "PC Bios   : Skipping optional missing $BIOS_FILE"
                continue
            fi
            log "PC Bios    : Copying $BIOS_FILE"
            cp -f $PCBIOS_DIR/$BIOS_FILE $OUT_DIR/lib/pc-bios/$BIOS_FILE ||
                panic "Could not copy BIOS file: $PCBIOS_DIR/$BIOS_FILE"
        done
        # copy key files needed by vnc
        VNC_KEYMAPS=$(dirname "$0")/../pc-bios/keymaps
        cp -f -r $VNC_KEYMAPS $OUT_DIR/lib/pc-bios ||
            panic "Missing vnc keymap file: $VNC_KEYMAPS"
        log "VNC        : Copying $VNC_KEYMAPS"
    fi
fi

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

case "$HOST_OS" in
    darwin) PROBE_COREAUDIO=yes;
    ;;
    linux) PROBE_ALSA=yes; PROBE_OSS=yes; PROBE_ESD=yes; PROBE_PULSEAUDIO=yes;
    ;;
    freebsd) PROBE_OSS=yes;
    ;;
    windows*) PROBE_WINAUDIO=yes
    ;;
esac

CMAKE_DIR=$AOSP_PREBUILTS_DIR/cmake
if [ ! -d "$CMAKE_DIR" ]; then
  panic "Missing cmake directory: $CMAKE_DIR"
fi

probe_prebuilts_dir () {
    local PREBUILTS_DIR
    PREBUILTS_DIR=$AOSP_PREBUILTS_DIR/android-emulator-build/$3
    if [ ! -d "$PREBUILTS_DIR" ]; then
        panic "Missing prebuilts directory: $PREBUILTS_DIR"
    fi
    log "$1 prebuilts dir: $PREBUILTS_DIR"
    eval $2=\$PREBUILTS_DIR
}

###
###  Common probe
###
probe_prebuilts_dir "Common" COMMON_PREBUILTS_DIR common

###
###  Zlib probe
###
probe_prebuilts_dir "Zlib" ZLIB_PREBUILTS_DIR qemu-android-deps

###
###  Libpng probe
###
probe_prebuilts_dir "Libpng" LIBPNG_PREBUILTS_DIR qemu-android-deps

###
###  Libsdl2 probe
###
probe_prebuilts_dir "Libsdl2" LIBSDL2_PREBUILTS_DIR qemu-android-deps

###
###  Libxml2 probe
###
probe_prebuilts_dir "Libxml2" LIBXML2_PREBUILTS_DIR common/libxml2

###
###  Libcurl probe
###
probe_prebuilts_dir "LibCURL" LIBCURL_PREBUILTS_DIR curl

###
###  LibANGLEtranslation probe
###
probe_prebuilts_dir "LibANGLEtranslation" ANGLE_TRANSLATION_PREBUILTS_DIR common/ANGLE

###
###  Protobuf library probe
###
probe_prebuilts_dir "Protobuf" PROTOBUF_PREBUILTS_DIR protobuf

###
###  lz4 probe
###
probe_prebuilts_dir "lz4" LZ4_PREBUILTS_DIR common/lz4

###
###  Protobuf library probe
###
probe_prebuilts_dir "Protobuf" PROTOBUF_PREBUILTS_DIR protobuf

###
###  virglrenderer probe
###
probe_prebuilts_dir "virglrenderer" VIRGLRENDERER_PREBUILTS_DIR common/virglrenderer

###
### MSVC probe
###
if [ "$OPTION_WINDOWS_MSVC" = "yes" ] ; then
    probe_prebuilts_dir "msvc" MSVC_DIR msvc
fi

CACERTS_FILE="$PROGDIR/data/ca-bundle.pem"
if [ ! -f "$CACERTS_FILE" ]; then
    panic "Missing cacerts file: $CACERTS_FILE"
fi
mkdir -p $OUT_DIR/lib
cp -f "$CACERTS_FILE" "$OUT_DIR/lib/"

ADVANCED_FEATURE_FILE="$PROGDIR/data/advancedFeatures.ini"
if [ ! -f "$ADVANCED_FEATURE_FILE" ]; then
    panic "Missing advanced feature file: $ADVANCED_FEATURE_FILE"
fi
mkdir -p $OUT_DIR/lib
cp -f "$ADVANCED_FEATURE_FILE" "$OUT_DIR/lib/"

ADVANCED_FEATURE_FILE_CANARY="$PROGDIR/data/advancedFeaturesCanary.ini"
if [ ! -f "$ADVANCED_FEATURE_FILE_CANARY" ]; then
    panic "Missing Canary advanced feature file: $ADVANCED_FEATURE_FILE_CANARY"
fi
mkdir -p $OUT_DIR/lib
cp -f "$ADVANCED_FEATURE_FILE_CANARY" "$OUT_DIR/lib/"

SERVER_FLAGS_BACKUP="$PROGDIR/data/emu-original-feature-flags.protobuf"
if [ ! -f "$SERVER_FLAGS_BACKUP" ]; then
    panic "Missing server feature flags backup file: $SERVER_FLAGS_BACKUP"
fi
mkdir -p $OUT_DIR/lib
cp -f "$SERVER_FLAGS_BACKUP" "$OUT_DIR/lib/"

###
###  Breakpad probe
###
probe_prebuilts_dir "Breakpad" BREAKPAD_PREBUILTS_DIR common/breakpad
if [ "$OPTION_MINGW" = "yes" ] ; then
    DUMPSYMS=$BREAKPAD_PREBUILTS_DIR/$BUILD_TAG/bin/dump_syms_dwarf
else
    ##Mac and Linux builds
    DUMPSYMS=$BREAKPAD_PREBUILTS_DIR/$BUILD_TAG/bin/dump_syms
fi

###
###  Qt probe
###
probe_prebuilts_dir "Qt" QT_PREBUILTS_DIR qt

###
###  e2fsprogs probe
###
probe_prebuilts_dir "e2fsprogs" E2FSPROGS_PREBUILTS_DIR common/e2fsprogs

###
###  ffmpeg probe
###
probe_prebuilts_dir "ffmpeg" FFMPEG_PREBUILTS_DIR common/ffmpeg

###
###  bluez probe
###
if [ "$HOST_OS" = "linux" ] ; then
  probe_prebuilts_dir "bluez" LIBBLUEZ_PREBUILTS_DIR common/bluez
fi

###
###  libusb probe
###
if [ "$HOST_OS" = "linux" -o "$HOST_OS" = "darwin" ] ; then
  probe_prebuilts_dir "usb" LIBUSB_PREBUILTS_DIR common/libusb
fi

###
###  x264 probe
###
probe_prebuilts_dir "x264" X264_PREBUILTS_DIR common/x264

###
###  libvpx probe
###
probe_prebuilts_dir "libxpx" LIBVPX_PREBUILTS_DIR common/libvpx

probe_prebuilts_dir "QEMU2 Dependencies" \
    QEMU2_DEPS_PREBUILTS_DIR \
    qemu-android-deps

probe_prebuilts_dir "TCMALLOC Dependencies" \
    TCMALLOC_PREBUILTS_DIR \
    common/tcmalloc


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

# check whether we have <byteswap.h>
#
feature_check_header HAVE_BYTESWAP_H      "<byteswap.h>"
feature_check_header HAVE_MACHINE_BSWAP_H "<machine/bswap.h>"
feature_check_header HAVE_FNMATCH_H       "<fnmatch.h>"

# check for Mingw version.
if [ "$HOST_OS" = "windows" ] && [ "$OPTION_MINGW" = "yes" ]; then
log "Mingw      : Probing for GCC version."
GCC_VERSION=$($CC -v 2>&1 | awk '$1 == "gcc" && $2 == "version" { print $3; }')
GCC_MAJOR=$(echo "$GCC_VERSION" | cut -f1 -d.)
GCC_MINOR=$(echo "$GCC_VERSION" | cut -f2 -d.)
log "Mingw      : Found GCC version $GCC_MAJOR.$GCC_MINOR [$GCC_VERSION]"
MINGW_GCC_VERSION=$(( $GCC_MAJOR * 100 + $GCC_MINOR ))
fi

# Build the config.make file
#

PREBUILT_PATH_PAIRS=
PREBUILT_SYMPATH_PAIRS=

# Copy a single file
# $1: Source file path.
# $2: Destination file path (not a directory!)
copy_file () {
    local SRC_FILE="$1"
    local DST_FILE="$2"

    log2 "Copying $SRC_FILE -> $DST_FILE"

    case $DST_FILE in
        */)
            if [ ! -d "$DST_FILE" ]; then
                mkdir -p "${DST_FILE%%/}" ||
                        panic "Cannot create destination directory: $DST_FILE"
            fi
            DST_FILE=${DST_FILE%%/}
            ;;
        *)
            if [ -d "$DST_FILE" ]; then
                DST_FILE=${DST_FILE}/
            else
                local DST_DIR="$(dirname "$DST_FILE")"
                if [ ! -d "$DST_DIR" ]; then
                    mkdir -p "$(dirname "$DST_FILE")" ||
                            panic "Could not create destination directory: $DST_DIR"
                fi
            fi
            ;;
    esac
    cp -a $SRC_FILE $DST_FILE ||
            panic "Could not copy $SRC_FILE into $DST_FILE !?"
}


# Set a prebuilt file to be copied
# $1: PKG root directory
# $2: Source file path relative to PKG SRC directory
# $3: Destination file path (not a directory!) relative to OUT_DIR
install_prebuilt_dll () {
    local SRC_FILE="$1"
    local DST_FILE="$2"

    if [ -L $SRC_FILE ]; then
        PREBUILT_SYMPATH_PAIRS="$PREBUILT_SYMPATH_PAIRS $SRC_FILE:$DST_FILE"
    else
        PREBUILT_PATH_PAIRS="$PREBUILT_PATH_PAIRS $SRC_FILE:$DST_FILE"
    fi
}

# Returns the compiler (GCC)/clang
cc_type () {
   local TYPE=GCC
   if  $1 --version | grep -q clang; then
     TYPE=clang
   fi
   printf $TYPE
}

PREBUILT_ARCHS=
case $HOST_OS in
    windows_msvc)
        PREBUILT_ARCHS="x86_64"
        ;;
    windows)
        PREBUILT_ARCHS="x86 x86_64"
        ;;
    *)
        PREBUILT_ARCHS="x86_64"
        ;;
esac

if [ "$OPTION_AEMU64_ONLY" == "yes" ]; then
    PREBUILT_ARCHS="x86_64"
fi

GOLDFISH_OPENGL_DIR=$AOSP_ROOT_DIR/device/generic/goldfish-opengl

if [ "$OPTION_GOLDFISH_OPENGL_DIR" = "yes" ]; then
    GOLDFISH_OPENGL_DIR=$OPTION_GOLDFISH_OPENGL_DIR
fi

log "Guest OpenGL driver location: $GOLDFISH_OPENGL_DIR"

###
### Copy tcmalloc if available
###

TCMALLOC_PREBUILTS_DIR=$AOSP_PREBUILTS_DIR/android-emulator-build/common/tcmalloc
if [ -d $TCMALLOC_PREBUILTS_DIR ]; then
    # Linux only
    if [ $HOST_OS = "linux" ]; then
        TCMALLOC_DSTDIR="$OUT_DIR/lib64"
        TCMALLOC_SRCDIR="$TCMALLOC_PREBUILTS_DIR/linux-x86_64/lib64"
        TCMALLOC_LIBNAME="libtcmalloc_minimal.so.4"
        if [ -f "$TCMALLOC_SRCDIR/$TCMALLOC_LIBNAME" ]; then
            install_prebuilt_dll "$TCMALLOC_SRCDIR/$TCMALLOC_LIBNAME" \
                              "$TCMALLOC_DSTDIR/$TCMALLOC_LIBNAME"
        fi
    fi
fi

###
### Copy ANGLE if available
###
ANGLE_PREBUILTS_DIR=$AOSP_PREBUILTS_DIR/android-emulator-build/common/ANGLE
if [ -d $ANGLE_PREBUILTS_DIR ]; then
    ANGLE_HOST=$HOST_OS
    case $ANGLE_HOST in
        windows*)
            ANGLE_SUFFIX=.dll
            ;;
        linux)
            ANGLE_SUFFIX=.so
            ;;
        *)
    esac
    # Windows only (for now)
    case $ANGLE_HOST in
        windows*)
            log "Copying ANGLE prebuilt libraries from $ANGLE_PREBUILTS_DIR"
            for LIBNAME in libEGL libGLESv2 d3dcompiler_47; do # GLESv2 only for now
                for ANGLE_ARCH in $PREBUILT_ARCHS; do
                    if [ "$ANGLE_ARCH" = "x86" ]; then
                        ANGLE_LIBDIR=lib
                    else
                        ANGLE_LIBDIR=lib64
                    fi
                    ANGLE_LIBNAME=$LIBNAME$ANGLE_SUFFIX
                    ANGLE_SRCDIR=$ANGLE_PREBUILTS_DIR/$ANGLE_HOST-$ANGLE_ARCH
                    for ANGLE_DX in 9 ""; do
                        ANGLE_DSTDIR="$OUT_DIR/$ANGLE_LIBDIR/gles_angle$ANGLE_DX"
                        ANGLE_DSTLIB="$ANGLE_LIBNAME"
                        if [ -f "$ANGLE_SRCDIR/lib/dx$ANGLE_DX/$ANGLE_LIBNAME" ]; then
                            install_prebuilt_dll "$ANGLE_SRCDIR/lib/dx$ANGLE_DX/$ANGLE_LIBNAME" \
                                             "$ANGLE_DSTDIR/$ANGLE_DSTLIB"
                        fi
                    done
                done
            done
            ;;
    esac
fi

###
### Copy Swiftshader if available
###
SWIFTSHADER_PREBUILTS_DIR=$AOSP_PREBUILTS_DIR/android-emulator-build/common/swiftshader
if [ -d $SWIFTSHADER_PREBUILTS_DIR ]; then
    log "Copying Swiftshader prebuilt libraries from $SWIFTSHADER_PREBUILTS_DIR"
    SWIFTSHADER_PREFIX=lib
    SWIFTSHADER_HOST=$HOST_OS
    case $SWIFTSHADER_HOST in
        windows*)
            SWIFTSHADER_SUFFIX=.dll
            ;;
        darwin)
            SWIFTSHADER_SUFFIX=.dylib
            ;;
        linux)
            SWIFTSHADER_SUFFIX=.so
            ;;
        *)
    esac
    for LIBNAME in EGL GLES_CM GLESv2; do
        for SWIFTSHADER_ARCH in $PREBUILT_ARCHS; do
            if [ "$SWIFTSHADER_ARCH" = "x86" ]; then
                SWIFTSHADER_LIBDIR=lib
            else
                SWIFTSHADER_LIBDIR=lib64
            fi
            SWIFTSHADER_LIBNAME=$SWIFTSHADER_PREFIX$LIBNAME$SWIFTSHADER_SUFFIX
            SWIFTSHADER_SRCDIR=$SWIFTSHADER_PREBUILTS_DIR/$SWIFTSHADER_HOST-$SWIFTSHADER_ARCH

            SWIFTSHADER_DSTDIR="$OUT_DIR/$SWIFTSHADER_LIBDIR/gles_swiftshader"
            SWIFTSHADER_DSTLIB="$SWIFTSHADER_LIBNAME"
            if [ -f "$SWIFTSHADER_SRCDIR/lib/$SWIFTSHADER_LIBNAME" ]; then
                install_prebuilt_dll "$SWIFTSHADER_SRCDIR/lib/$SWIFTSHADER_LIBNAME" \
                                 "$SWIFTSHADER_DSTDIR/$SWIFTSHADER_DSTLIB"
            fi
        done
    done
fi

###
### Copy Vulkan libraries (for now, for test only, so send them to testlibs)
###
VULKAN_PREBUILTS_DIR=$AOSP_PREBUILTS_DIR/android-emulator-build/common/vulkan
if [ -d $VULKAN_PREBUILTS_DIR ]; then
    log "Copying Vulkan prebuilt libraries from $VULKAN_PREBUILTS_DIR"

    VULKAN_TESTLIB_DSTDIR="$OUT_DIR/testlib64"
    VULKAN_LIB_DSTDIR="$OUT_DIR/lib64/vulkan"
    VULKAN_HOST=$HOST_OS

    for PREBUILT_ARCH in $PREBUILT_ARCHS; do
        VULKAN_SRCDIR="$VULKAN_PREBUILTS_DIR/$HOST_OS-$PREBUILT_ARCH"

        VULKAN_MAC_ICD_LIB=libMoltenVK.dylib
        VULKAN_MAC_ICD_FILE=MoltenVK_icd.json

        VULKAN_MOCK_ICD_FILE=VkICD_mock_icd.json

        case $VULKAN_HOST in
            windows*)
                VULKAN_LOADER_LIB=vulkan-1.dll
                VULKAN_MOCK_ICD_LIB=VkICD_mock_icd.dll
                ;;
            darwin)
                VULKAN_LOADER_LIB=libvulkan.dylib
                VULKAN_MOCK_ICD_LIB=libVkICD_mock_icd.dylib
                ;;
            linux)
                VULKAN_LOADER_LIB=libvulkan.so
                VULKAN_MOCK_ICD_LIB=libVkICD_mock_icd.so
                ;;
            *)
        esac

        # Install the loader only to the test dir, unless on mac
        if [ "$PREBUILT_ARCH" = "x86_64" ]; then
            copy_file "$VULKAN_SRCDIR/$VULKAN_LOADER_LIB" \
                             "$VULKAN_TESTLIB_DSTDIR/$VULKAN_LOADER_LIB"
            copy_file "$VULKAN_SRCDIR/$VULKAN_MOCK_ICD_LIB" \
                             "$VULKAN_TESTLIB_DSTDIR/$VULKAN_MOCK_ICD_LIB"
            copy_file "$VULKAN_SRCDIR/$VULKAN_MOCK_ICD_FILE" \
                             "$VULKAN_TESTLIB_DSTDIR/$VULKAN_MOCK_ICD_FILE"

            # For mac, we copy a mac vulkan implementation
            if [ "$VULKAN_HOST" = "darwin" ]; then
                copy_file "$VULKAN_SRCDIR/$VULKAN_LOADER_LIB" \
                                 "$VULKAN_LIB_DSTDIR/$VULKAN_LOADER_LIB"
                copy_file "$VULKAN_SRCDIR/$VULKAN_MAC_ICD_LIB" \
                                 "$VULKAN_LIB_DSTDIR/$VULKAN_MAC_ICD_LIB"
                copy_file "$VULKAN_SRCDIR/$VULKAN_MAC_ICD_FILE" \
                          "$VULKAN_LIB_DSTDIR/$VULKAN_MAC_ICD_FILE"
            fi
        fi
    done
fi

###
###  Copy Mesa if available
###
MESA_PREBUILTS_DIR=$AOSP_PREBUILTS_DIR/android-emulator-build/mesa
if [ [ -d $MESA_PREBUILTS_DIR ] && [ "$OPTION_AEMU64_ONLY" == "no" ] ]; then
    log "Copying Mesa prebuilt libraries from $MESA_PREBUILTS_DIR"
    case $HOST_OS in
        windows*)
            MESA_LIBNAME=opengl32.dll
            ;;
        linux)
            MESA_LIBNAME="libGL.so libGL.so.1"
            ;;
        *)
            MESA_LIBNAME=
    esac
    for LIBNAME in $MESA_LIBNAME; do
        for MESA_ARCH in $PREBUILT_ARCHS; do
            if [ "$MESA_ARCH" = "x86" ]; then
                MESA_LIBDIR=lib
            else
                MESA_LIBDIR=lib64
            fi
            MESA_SRCDIR=$MESA_PREBUILTS_DIR/$HOST_OS-$MESA_ARCH
            MESA_DSTDIR="$OUT_DIR/$MESA_LIBDIR/gles_mesa"
            MESA_DSTLIB="$LIBNAME"
            if [ "$LIBNAME" = "opengl32.dll" ]; then
                MESA_DSTLIB="mesa_opengl32.dll"
            fi
            install_prebuilt_dll "$MESA_SRCDIR/lib/$LIBNAME" "$MESA_DSTDIR/$MESA_DSTLIB"
        done
    done
fi

# Copy Qt shared libraries, if needed.
log "Copying Qt prebuilt libraries from $QT_PREBUILTS_DIR"
for QT_ARCH in $PREBUILT_ARCHS; do
    QT_SRCDIR=$QT_PREBUILTS_DIR/$HOST_OS-$QT_ARCH
    case $QT_ARCH in
        x86) QT_DSTDIR=$OUT_DIR/lib/qt;;
        x86_64) QT_DSTDIR=$OUT_DIR/lib64/qt;;
        *) panic "Invalid Qt host architecture: $QT_ARCH";;
    esac
    mkdir -p "$QT_DSTDIR" || panic "Could not create Qt library sub-directory!"
    if [ ! -d "$QT_SRCDIR" ]; then
        continue
    fi
    case $HOST_OS in
        windows*) QT_DLL_FILTER="*.dll";;
        darwin) QT_DLL_FILTER="*.dylib";;
        *) QT_DLL_FILTER="*.so*";;
    esac
    QT_LIBS=$(cd "$QT_SRCDIR" && find . -name "$QT_DLL_FILTER" -not -path "*.dSYM/*" 2>/dev/null)
    if [ -z "$QT_LIBS" ]; then
        panic "Cannot find Qt prebuilt libraries!?"
    fi
    for QT_LIB in $QT_LIBS; do
        QT_LIB=${QT_LIB#./}
        QT_DST_LIB=$QT_LIB
        case $HOST_OS in
            windows*)
                # NOTE: On Windows, the prebuilt libraries are placed under
                # $PREBUILTS/qt/bin, not $PREBUILTS/qt/lib, ensure that they
                # are always copied to $OUT_DIR/lib[64]/qt/lib/.
                QT_DST_LIB=$(printf %s "$QT_DST_LIB" | sed -e 's|^bin/|lib/|g')
                ;;
        esac
        install_prebuilt_dll "$QT_SRCDIR/$QT_LIB" "$QT_DSTDIR/$QT_DST_LIB"
    done
done

# Copy e2fsprogs binaries.
log "Copying e2fsprogs binaries."
for E2FS_ARCH in $PREBUILT_ARCHS; do
    # NOTE: in windows only 32-bit binaries are available, so we'll copy the
    # 32-bit executables to the bin64/ directory to cover all our bases
    case $HOST_OS in
        windows*) E2FS_SRCDIR=$E2FSPROGS_PREBUILTS_DIR/$HOST_OS-x86;;
        *) E2FS_SRCDIR=$E2FSPROGS_PREBUILTS_DIR/$HOST_OS-$E2FS_ARCH;;
    esac

    if [ ! -d "$E2FS_SRCDIR" ]; then
        continue
    fi

    case $E2FS_ARCH in
        x86) E2FS_DSTDIR=$OUT_DIR/bin;;
        x86_64) E2FS_DSTDIR=$OUT_DIR/bin64;;
        *) panic "Invalid e2fsprogs host architecture: $E2FS_ARCH"
    esac
    copy_file "$E2FS_SRCDIR/sbin/*" "$E2FS_DSTDIR/"
done

# Copy the requested library from the toolchain sysroot.
# This function
#   - resolves symlinks to find the actual library
#   - copies the actual library with the resolved name.
#   - copies all the symlinks found in $SRCDIR to that library as well.
# This way, linking against generic/specific version works on systems that
# do/don't understand symlinks.
#   $1: Destination directory.
#   $2: Source directory.
#   $3: Name of the library (without the .so suffix).
copy_toolchain_lib () {
    local DESTDIR SRCDIR NAME
    local FROM REALNAME
    local SYMLINK SYMNAME
    DESTDIR="$1"
    SRCDIR="$2"
    NAME="$3.so"

    mkdir -p "${DESTDIR}" || panic "Failed to created ${DESTDIR}"

    FROM="$(readlink -qnm "${SRCDIR}/${NAME}")" ||
            panic "Coulnd't resolve ${SRCDIR}/${NAME}"
    REALNAME="$(basename "${FROM}")"

    cp -f "${FROM}" "${DESTDIR}/${REALNAME}" ||
            panic "Cound't copy ${FROM} --> ${DESTDIR}/${REALNAME}"

    # Now re-create all the symlinks as relative path symlinks.
    for SYMLINK in "${SRCDIR}/${NAME}".*; do
        if [ "${SYMLINK}" = "${FROM}" -o "$(readlink -qnm "${SYMLINK}")" != "${FROM}" ]; then
            continue
        fi
        SYMNAME="$(basename "${SYMLINK}")"
        if [ ! -e "${DESTDIR}/${SYMNAME}" -o "$(readlink -qnm "${DESTDIR}/${SYMNAME}")" != "${DESTDIR}/${REALNAME}" ]; then
            rm -f "${DESTDIR}/${SYMNAME}"
            ln -s "${REALNAME}" "${DESTDIR}/${SYMNAME}" || \
                    panic "Failed to create symlink ${DESTDIR}/${SYMNAME} --> ${REALNAME}"
        fi
    done
}

if [ -n "${TOOLCHAIN_SYSROOT}" ]; then
    # We currently only bundle libraries for linux
    # - mingw: Statically links all libraries (b.android.com/191369)
    # - darwin: We use the development machine's installed SDK. No point
    #       bundling, since we don't have a uniform libraries to begin with.
    case $BUILD_OS in
      linux*)
        LIBCPLUSPLUS=$("$GEN_SDK" $GEN_SDK_FLAGS --print=libcplusplus unused_parameter)
        copy_toolchain_lib "${OUT_DIR}/intermediates64" "$(dirname ${LIBCPLUSPLUS})" libc++
        if [ "$OPTION_MINGW" = "no" ] ; then
          log "Bundling ${LIBCPLUSPLUS}"
          copy_toolchain_lib "${OUT_DIR}/lib64" "$(dirname ${LIBCPLUSPLUS})" libc++
        fi
        ;;
        *) log "Not copying toolchain libraries for ${BUILD_TARGET_OS}";;
    esac
fi

###
###  Libwinpthreads-1.dll probe
###

# $1: DLL name (e.g. libwinpthread-1.dll)
probe_mingw_dlls () {
    local DLLS DLL SRC_DLL DST_DLL MINGW_SYSROOT
    MINGW_SYSROOT=$("$GEN_SDK" $WIN_SDK_FLAGS --print=sysroot unused_parameter)
    DLLS=$(cd "$MINGW_SYSROOT" && find . -name "$1" 2>/dev/null)
    if [ -z "$DLLS" ]; then
        log "Mingw      : No $1 available under $MINGW_SYSROOT"
    else
        log "Mingw      : Copying prebuilt $1 libraries."
        for DLL in $DLLS; do
            SRC_DLL=${DLL#./}  # Remove initial ./ in DLL path.
            case $SRC_DLL in
                lib/*)
                    DST_DLL=lib64${SRC_DLL##lib}
                    ;;
                bin/*)
                    DST_DLL=lib64${SRC_DLL##bin}
                    ;;
                lib32/*)
                    DST_DLL=lib${SRC_DLL##lib32}
                    ;;
                *)
                    DST_DLL=${SRC_DLL}
                    ;;
            esac
            log2 "Mingw      :    $SRC_DLL -> $DST_DLL"
            install_prebuilt_dll "$MINGW_SYSROOT"/$SRC_DLL "$OUT_DIR"/$DST_DLL
        done
    fi
}

if [ "$OPTION_MINGW" = "yes" ] ; then
    probe_mingw_dlls libwinpthread-1.dll
    probe_mingw_dlls libgcc_s_seh-1.dll
    probe_mingw_dlls libgcc_s_sjlj-1.dll
fi

# Re-create the configuration file
config_mk=$OUT_DIR/build/config.make
config_dir=$(dirname $config_mk)
mkdir -p "$config_dir" 2> $TMPL
if [ $? != 0 ] ; then
    echo "Can't create directory for build config file: $config_dir"
    cat $TMPL
    clean_exit
fi

log "Generate   : $config_mk"

echo "# This file was autogenerated by $PROGNAME. Do not edit !" > $config_mk
echo "TEST_SHELL             := $TEST_SHELL" >> $config_mk
echo "BUILD_TARGET_OS        := $HOST_OS" >> $config_mk
echo "BUILD_TARGET_OS_FLAVOR := $HOST_OS_FLAVOR" >> $config_mk
echo "BUILD_TARGET_ARCH      := $HOST_ARCH" >> $config_mk
echo "BUILD_TARGET_CC        := $CC" >> $config_mk
echo "BUILD_TARGET_CC_TYPE   := $(cc_type $CC)" >> $config_mk
echo "BUILD_TARGET_CXX       := $CXX" >> $config_mk
echo "BUILD_TARGET_LD        := $LD" >> $config_mk
echo "BUILD_TARGET_AR        := $AR" >> $config_mk
echo "BUILD_TARGET_OBJCOPY   := $OBJCOPY" >> $config_mk
echo "BUILD_TARGET_WINDRES   := $WINDRES" >> $config_mk
echo "BUILD_TARGET_DUMPSYMS  := $DUMPSYMS" >> $config_mk
echo "BUILD_OBJS_DIR         := $OUT_DIR" >> $config_mk
echo "BUILD_TIDY             := $TIDY" >> $config_mk

echo "" >> $config_mk
echo "BUILD_HOST_AR          := $BUILD_AR" >> $config_mk
echo "BUILD_HOST_RANLIB      := $BUILD_RANLIB" >> $config_mk
echo "BUILD_HOST_CC          := $BUILD_CC" >> $config_mk
echo "BUILD_HOST_CC_TYPE     := $(cc_type $BUILD_CC)" >> $config_mk
echo "BUILD_HOST_CXX         := $BUILD_CXX" >> $config_mk
echo "BUILD_HOST_LD          := $BUILD_LD" >> $config_mk
echo "BUILD_HOST_OBJCOPY     := $BUILD_OBJCOPY" >> $config_mk
echo "BUILD_HOST_CFLAGS      := $BUILD_CFLAGS" >> $config_mk
echo "BUILD_HOST_LDFLAGS     := $BUILD_LDFLAGS" >> $config_mk
echo "BUILD_HOST_DUMPSYMS    := $DUMPSYMS" >> $config_mk

if [ "${TOOLCHAIN_SYSROOT}" ]; then
echo "TOOLCHAIN_SYSROOT    := $TOOLCHAIN_SYSROOT" >> $config_mk
fi
if [ "$OPTION_LTO" = "true" ]; then
    echo "BUILD_ENABLE_LTO      := true" >> $config_mk
fi

if [ "$OPTION_MIPS" = "true" ]; then
    echo "BUILD_ENABLE_MIPS      := true" >> $config_mk
fi

if [ "$OPTION_MIN_BUILD" = "yes" ]; then
    echo "CONFIG_MIN_BUILD  := true" >> $config_mk
fi
if [ "$OPTION_AEMU64_ONLY" = "yes" ]; then
    echo "CONFIG_AEMU64_ONLY  := true" >> $config_mk
fi
echo "CONFIG_COREAUDIO  := $PROBE_COREAUDIO" >> $config_mk
echo "CONFIG_WINAUDIO   := $PROBE_WINAUDIO" >> $config_mk
echo "CONFIG_ESD        := $PROBE_ESD" >> $config_mk
echo "CONFIG_ALSA       := $PROBE_ALSA" >> $config_mk
echo "CONFIG_OSS        := $PROBE_OSS" >> $config_mk
echo "CONFIG_PULSEAUDIO := $PROBE_PULSEAUDIO" >> $config_mk
echo "QT_PREBUILTS_DIR  := $QT_PREBUILTS_DIR" >> $config_mk

if [ "$OPTION_BENCHMARKS" = "yes" ]; then
  echo "BUILD_BENCHMARKS := true" >> $config_mk
fi

if [ "$OPTION_GLES" = "angle" ] ; then
    echo "EMULATOR_USE_ANGLE := true" >> $config_mk
else
    echo "EMULATOR_USE_ANGLE := false" >> $config_mk
fi

echo "CMAKE_DIR := $CMAKE_DIR" >> $config_mk
echo "COMMON_PREBUILTS_DIR := $COMMON_PREBUILTS_DIR" >> $config_mk
echo "ZLIB_PREBUILTS_DIR := $ZLIB_PREBUILTS_DIR" >> $config_mk
echo "LIBPNG_PREBUILTS_DIR := $LIBPNG_PREBUILTS_DIR" >> $config_mk
echo "LIBSDL2_PREBUILTS_DIR := $LIBSDL2_PREBUILTS_DIR" >> $config_mk
echo "LIBXML2_PREBUILTS_DIR := $LIBXML2_PREBUILTS_DIR" >> $config_mk
echo "LIBCURL_PREBUILTS_DIR := $LIBCURL_PREBUILTS_DIR" >> $config_mk
echo "ANGLE_TRANSLATION_PREBUILTS_DIR := $ANGLE_PREBUILTS_DIR" >> $config_mk
echo "BREAKPAD_PREBUILTS_DIR := $BREAKPAD_PREBUILTS_DIR" >> $config_mk
# libuuid is a part of e2fsprogs package
echo "LIBUUID_PREBUILTS_DIR := $E2FSPROGS_PREBUILTS_DIR" >> $config_mk
echo "PROTOBUF_PREBUILTS_DIR := $PROTOBUF_PREBUILTS_DIR" >> $config_mk
echo "LZ4_PREBUILTS_DIR := $LZ4_PREBUILTS_DIR" >> $config_mk
echo "FFMPEG_PREBUILTS_DIR := $FFMPEG_PREBUILTS_DIR" >> $config_mk
echo "X264_PREBUILTS_DIR := $X264_PREBUILTS_DIR" >> $config_mk
echo "LIBVPX_PREBUILTS_DIR := $LIBVPX_PREBUILTS_DIR" >> $config_mk
echo "VIRGLRENDERER_PREBUILTS_DIR := $VIRGLRENDERER_PREBUILTS_DIR" >> $config_mk
echo "GOLDFISH_OPENGL_DIR := $GOLDFISH_OPENGL_DIR" >> $config_mk

if [ "$HOST_OS" = "windows_msvc" ]; then
  echo "MSVC_DIR := $MSVC_DIR" >> $config_mk
fi

if [ "$HOST_OS" = "linux" ]; then
  echo "LIBBLUEZ_PREBUILTS_DIR := $LIBBLUEZ_PREBUILTS_DIR" >> $config_mk
fi

if [ "$HOST_OS" = "linux" -o "$HOST_OS" = "darwin" ]; then
  echo "LIBUSB_PREBUILTS_DIR := $LIBUSB_PREBUILTS_DIR" >> $config_mk
fi

if [ $OPTION_DEBUG = "yes" ] ; then
    echo "BUILD_DEBUG := true" >> $config_mk
fi
if [ $OPTION_SANITIZER != "no" ] ; then
    echo "BUILD_SANITIZER := $OPTION_SANITIZER" >> $config_mk
fi
if [ $OPTION_EMUGL_PRINTOUT = "yes" ] ; then
    echo "BUILD_EMUGL_PRINTOUT := true" >> $config_mk
fi
if [ $OPTION_SNAPSHOT_PROFILE -gt 0 ] ; then
    echo "BUILD_SNAPSHOT_PROFILE := $OPTION_SNAPSHOT_PROFILE" >> $config_mk
fi
if [ "$OPTION_STRIP" = "yes" ]; then
    echo "BUILD_STRIP_BINARIES := true" >> $config_mk
fi
if [ "$OPTION_SYMBOLS" = "yes" ]; then
    echo "BUILD_GENERATE_SYMBOLS := true" >> $config_mk
    echo "" >> $config_mk
fi

echo "EMULATOR_CRASHUPLOAD := $OPTION_CRASHUPLOAD" >> $config_mk


if [ "$ANDROID_SDK_TOOLS_REVISION" ] ; then
  echo "ANDROID_SDK_TOOLS_REVISION := $ANDROID_SDK_TOOLS_REVISION" >> $config_mk
fi

if [ "$ANDROID_SDK_TOOLS_BUILD_NUMBER" ] ; then
  echo "ANDROID_SDK_TOOLS_BUILD_NUMBER := $ANDROID_SDK_TOOLS_BUILD_NUMBER" >> $config_mk
fi

# Dump the ToT CL SHA1 number (located in .git/FETCH_HEAD)
ANDROID_SDK_TOOLS_CL_SHA1=$( git log -n 1 --pretty=format:"%H" )
if [ "$ANDROID_SDK_TOOLS_CL_SHA1" ] ; then
  echo "ANDROID_SDK_TOOLS_CL_SHA1 := $ANDROID_SDK_TOOLS_CL_SHA1" >> $config_mk
fi

echo "QEMU2_DEPS_PREBUILTS_DIR := $QEMU2_DEPS_PREBUILTS_DIR" >> $config_mk
echo "TCMALLOC_PREBUILTS_DIR := $TCMALLOC_PREBUILTS_DIR" >> $config_mk

if [ -z "$OPTION_QEMU2_SRCDIR" ]; then
    QEMU2_TOP_DIR=$ANDROID_EMULATOR_QEMU2_SRCDIR
    if [ -n "$QEMU2_TOP_DIR" ]; then
        log "QEMU2      : $QEMU2_TOP_DIR  [environment]"
    else
        QEMU2_TOP_DIR=$PROGDIR/..
        log "QEMU2      : $QEMU2_TOP_DIR  [auto-detect]"
    fi
    if [ ! -d "$QEMU2_TOP_DIR" ]; then
        panic "Missing QEMU2 source directory: $QEMU2_TOP_DIR"
    fi
    QEMU2_TOP_DIR=$(cd "$QEMU2_TOP_DIR" && pwd -P)
else
    QEMU2_TOP_DIR=$OPTION_QEMU2_SRCDIR
    log "QEMU2      : $QEMU2_TOP_DIR  [--qemu2-src-dir]"
fi

if [ ! -d "$QEMU2_TOP_DIR"/android-qemu2-glue ]; then
    panic "Missing directory: $QEMU2_TOP_DIR/android-qemu2-glue"
else
    echo "QEMU2_TOP_DIR := $QEMU2_TOP_DIR" >> $config_mk
fi

echo "PREBUILT_PATH_PAIRS := \\" >> $config_mk
for PREBUILT_PATH_PAIR in $PREBUILT_PATH_PAIRS;
do
    echo "    $PREBUILT_PATH_PAIR \\" >> $config_mk
done
echo "" >> $config_mk

echo "PREBUILT_SYMPATH_PAIRS := \\" >> $config_mk
for PREBUILT_SYMPATH_PAIR in $PREBUILT_SYMPATH_PAIRS;
do
    echo "    $PREBUILT_SYMPATH_PAIR \\" >> $config_mk
done

cat $QEMU2_TOP_DIR/qemu2-qapi-auto-generated/trace-config >> $config_mk

echo "" >> $config_mk

# Build the config-host.h file
#
config_h=$OUT_DIR/build/config-host.h
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
echo "#define CONFIG_QT       1" >> $config_h
echo "#undef CONFIG_SDL"         >> $config_h

echo "#define CONFIG_BLOCK_DELAYED_FLUSH  1" >> $config_h
echo "#define CONFIG_LIVE_BLOCK_MIGRATION 1" >> $config_h

case "$HOST_OS" in
    windows*)
        echo "#define CONFIG_WIN32  1" >> $config_h
        ;;
    *)
        echo "#define CONFIG_POSIX  1" >> $config_h
        ;;
esac

case "$HOST_OS" in
    linux)
        echo "#define CONFIG_KVM_GS_RESTORE 1" >> $config_h
        ;;
esac

# only Linux has fdatasync()
case "$HOST_OS" in
    linux)
        echo "#define CONFIG_FDATASYNC    1" >> $config_h
        ;;
esac

case "$HOST_OS" in
    linux|darwin)
        echo "#define CONFIG_MADVISE  1" >> $config_h
        ;;
esac

# the -nand-limits options can only work on non-windows systems
case "$HOST_OS" in
    windows*)
        ;;
    *)
        echo "#define CONFIG_NAND_LIMITS  1" >> $config_h
        ;;
esac

echo "#define QEMU_VERSION    \"0.10.50\"" >> $config_h
echo "#define QEMU_PKGVERSION \"Android\"" >> $config_h
BSD=
case "$HOST_OS" in
    linux) CONFIG_OS=LINUX
    ;;
    darwin) CONFIG_OS=DARWIN
            BSD=1
    ;;
    freebsd) CONFIG_OS=FREEBSD
             BSD=1
    ;;
    windows*) CONFIG_OS=WIN32
    ;;
    *) CONFIG_OS=$HOST_OS
esac

case $HOST_OS in
    linux|darwin)
        echo "#define CONFIG_IOVEC 1" >> $config_h
        ;;
esac

echo "#define CONFIG_$CONFIG_OS   1" >> $config_h
if [ "$BSD" ]; then
    echo "#define CONFIG_BSD       1" >> $config_h
    echo "#define O_LARGEFILE      0" >> $config_h
    echo "#define MAP_ANONYMOUS    MAP_ANON" >> $config_h
fi

case "$HOST_OS" in
    linux)
        echo "#define CONFIG_SIGNALFD       1" >> $config_h
        ;;
esac

echo "#define CONFIG_ANDROID       1" >> $config_h

log "Generate   : $config_h"

QEMU2_CONFIG_DIR=$OUT_DIR/build/qemu2-config

[ -d $QEMU2_CONFIG_DIR ] || mkdir -p $QEMU2_CONFIG_DIR

if [ "$OPT_MINGW" ]; then
    $OUT_DIR/objs/build/toolchain/x86_64-mingw32-windres \
        -o $QEMU2_CONFIG_DIR/version.o \
        $QEMU2_TOP_DIR/version.rc
fi

replace_with_if_different () {
    cmp -s "$1" "$2" || mv "$2" "$1"
}

# Generate qemu-version.h from Git history.
QEMU_VERSION_H=$QEMU2_CONFIG_DIR/qemu-version.h
QEMU_VERSION_H_TMP=$QEMU_VERSION_H.tmp
rm -f "$QEMU_VERSION_H"

if [ -d "$QEMU2_TOP_DIR/.git" ]; then
    QEMU_VERSION=$(cd "$QEMU2_TOP_DIR" && git describe --match 'v*' 2>/dev/null | tr -d '\n')
else
    QEMU_VERSION=$(date "+%Y-%m-%d")
fi

echo "QEMU2    : Version [$QEMU_VERSION]"

printf "#define QEMU_PKGVERSION \"(android-%s)\"\n" "$QEMU_VERSION" > $QEMU_VERSION_H_TMP
printf '#define QEMU_FULL_VERSION QEMU_VERSION " (" QEMU_PKGVERSION ")"\n' >> $QEMU_VERSION_H_TMP
replace_with_if_different "$QEMU_VERSION_H" "$QEMU_VERSION_H_TMP"
rm -f "$QEMU_VERSION_TMP_H"

clean_temp

echo "Ready to go. Type ${GREEN}'make'${RESET} to build emulator, and ${GREEN}'make tests'${RESET} to run the unit tests."
