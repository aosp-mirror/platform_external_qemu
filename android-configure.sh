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
OPTION_TARGETS=""
OPTION_DEBUG=no
OPTION_IGNORE_AUDIO=no
OPTION_NO_PREBUILTS=no
OPTION_OUT_DIR=
OPTION_HELP=no
OPTION_STATIC=no
OPTION_MINGW=no

GLES_DIR=
GLES_SUPPORT=no
GLES_PROBE=yes

PCBIOS_PROBE=yes

HOST_CC=${CC:-gcc}
OPTION_CC=

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
  --debug) OPTION_DEBUG=yes
  ;;
  --install=*) OPTION_TARGETS="$OPTION_TARGETS $optarg";
  ;;
  --sdl-config=*) SDL_CONFIG=$optarg
  ;;
  --mingw) OPTION_MINGW=yes
  ;;
  --cc=*) OPTION_CC="$optarg"
  ;;
  --no-strip) OPTION_NO_STRIP=yes
  ;;
  --out-dir=*) OPTION_OUT_DIR=$optarg
  ;;
  --ignore-audio) OPTION_IGNORE_AUDIO=yes
  ;;
  --no-prebuilts) OPTION_NO_PREBUILTS=yes
  ;;
  --static) OPTION_STATIC=yes
  ;;
  --gles-dir=*) GLES_DIR=$optarg
  ;;
  --no-gles) GLES_PROBE=no
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
    echo "  --help                   print this message"
    echo "  --install=FILEPATH       copy emulator executable to FILEPATH [$TARGETS]"
    echo "  --cc=PATH                specify C compiler [$HOST_CC]"
    echo "  --sdl-config=FILE        use specific sdl-config script [$SDL_CONFIG]"
    echo "  --no-strip               do not strip emulator executable"
    echo "  --debug                  enable debug (-O0 -g) build"
    echo "  --ignore-audio           ignore audio messages (may build sound-less emulator)"
    echo "  --no-prebuilts           do not use prebuilt libraries and compiler"
    echo "  --out-dir=<path>         use specific output directory [objs/]"
    echo "  --mingw                  build Windows executable on Linux"
    echo "  --static                 build a completely static executable"
    echo "  --verbose                verbose configuration"
    echo "  --debug                  build debug version of the emulator"
    echo "  --gles-dir=PATH          specify path to GLES host emulation sources [auto-detected]"
    echo "  --no-gles                disable GLES emulation support"
    echo "  --no-pcbios              disable copying of PC Bios files"
    echo "  --no-tests               don't run unit test suite"
    echo ""
    exit 1
fi

# On Linux, try to use our prebuilt toolchain to generate binaries
# that are compatible with Ubuntu 10.4
if [ -z "$CC" -a -z "$OPTION_CC" -a "$HOST_OS" = linux ] ; then
    PREBUILTS_HOST_GCC=$(dirname $0)/../../prebuilts/gcc/linux-x86/host
    # NOTE: GCC 4.8 is currently disabled because this breaks MIPS emulation
    # For some odd reason. Remove the 'DISABLED_' prefix below to re-enable it,
    # e.g. once the MIPS backend has been updated to a more recent version.
    # This only affects Linux emulator binaries.
    PROBE_HOST_CC=$PREBUILTS_HOST_GCC/DISABLED_x86_64-linux-glibc2.11-4.8/bin/x86_64-linux-gcc
    if [ ! -f "$PROBE_HOST_CC" ]; then
        PROBE_HOST_CC=$PREBUILTS_HOST_GCC/x86_64-linux-glibc2.11-4.6/bin/x86_64-linux-gcc
        if [ ! -f "$PROBE_HOST_CC" ] ; then
            PROBE_HOST_CC=$(dirname $0)/../../prebuilts/tools/gcc-sdk/gcc
        fi
    fi
    if [ -f "$PROBE_HOST_CC" ] ; then
        echo "Using prebuilt toolchain: $PROBE_HOST_CC"
        CC="$PROBE_HOST_CC"
    fi
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

if [ "$OPTION_OUT_DIR" ]; then
    OUT_DIR="$OPTION_OUT_DIR"
    mkdir -p "$OUT_DIR" || panic "Could not create output directory: $OUT_DIR"
else
    OUT_DIR=objs
    log "Auto-config: --out-dir=objs"
fi

# Are we running in the Android build system ?
check_android_build


# Adjust a few things when we're building within the Android build
# system:
#    - locate prebuilt directory
#    - locate and use prebuilt libraries
#    - copy the new binary to the correct location
#
if [ "$OPTION_NO_PREBUILTS" = "yes" ] ; then
    IN_ANDROID_BUILD=no
fi

if [ "$IN_ANDROID_BUILD" = "yes" ] ; then
    locate_android_prebuilt

    # use ccache if USE_CCACHE is defined and the corresponding
    # binary is available.
    #
    if [ -n "$USE_CCACHE" ] ; then
        CCACHE="$ANDROID_PREBUILT/ccache/ccache$EXE"
        if [ ! -f $CCACHE ] ; then
            CCACHE="$ANDROID_PREBUILTS/ccache/ccache$EXE"
        fi
    fi

    # finally ensure that our new binary is copied to the 'out'
    # subdirectory as 'emulator'
    HOST_BIN=$(get_android_abs_build_var HOST_OUT_EXECUTABLES)
    if [ "$TARGET_OS" = "windows" ]; then
        HOST_BIN=$(echo $HOST_BIN | sed "s%$OS/bin%windows/bin%")
    fi
    if [ -n "$HOST_BIN" ] ; then
        OPTION_TARGETS="$OPTION_TARGETS $HOST_BIN/emulator$EXE"
        log "Targets    : TARGETS=$OPTION_TARGETS"
    fi

    # find the Android SDK Tools revision number
    TOOLS_PROPS=$ANDROID_TOP/sdk/files/tools_source.properties
    if [ -f $TOOLS_PROPS ] ; then
        ANDROID_SDK_TOOLS_REVISION=`awk -F= '/Pkg.Revision/ { print $2; }' $TOOLS_PROPS 2> /dev/null`
        log "Tools      : Found tools revision number $ANDROID_SDK_TOOLS_REVISION"
    else
        log "Tools      : Could not locate $TOOLS_PROPS !?"
    fi
else
    if [ "$USE_CCACHE" != 0 ]; then
        CCACHE=$(which ccache 2>/dev/null)
    fi
fi  # IN_ANDROID_BUILD = no

if [ -n "$CCACHE" -a -f "$CCACHE" ] ; then
    CC="$CCACHE $CC"
    log "Prebuilt   : CCACHE=$CCACHE"
else
    log "Prebuilt   : CCACHE can't be found"
    CCACHE=
fi

# Try to find the GLES emulation headers and libraries automatically
if [ "$GLES_PROBE" = "yes" ]; then
    GLES_SUPPORT=yes
    if [ -z "$GLES_DIR" ]; then
        GLES_DIR=../../sdk/emulator/opengl
        log2 "GLES       : Probing source dir: $GLES_DIR"
        if [ ! -d "$GLES_DIR" ]; then
            GLES_DIR=../opengl
            log2 "GLES       : Probing source dir: $GLES_DIR"
            if [ ! -d "$GLES_DIR" ]; then
                GLES_DIR=
            fi
        fi
        if [ -z "$GLES_DIR" ]; then
            echo "GLES       : Could not find GPU emulation sources!"
            GLES_SUPPORT=no
        else
            echo "GLES       : Found GPU emulation sources: $GLES_DIR"
            GLES_SUPPORT=yes
        fi
    fi
fi

if [ "$PCBIOS_PROBE" = "yes" ]; then
    PCBIOS_DIR=$(dirname "$0")/../../prebuilts/qemu-kernel/x86/pc-bios
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

# For OS X, detect the location of the SDK to use.
if [ "$HOST_OS" = darwin ]; then
    OSX_VERSION=$(sw_vers -productVersion)
    OSX_SDK_SUPPORTED="10.6 10.7 10.8"
    OSX_SDK_INSTALLED_LIST=$(xcodebuild -showsdks 2>/dev/null | grep macosx | sed -e "s/.*macosx//g" | sort -n)
    if [ -z "$OSX_SDK_INSTALLED_LIST" ]; then
        echo "ERROR: Please install XCode on this machine!"
        exit 1
    fi
    log "OSX: Installed SDKs: $OSX_SDK_INSTALLED_LIST"

    OSX_SDK_VERSION=$(echo "$OSX_SDK_INSTALLED_LIST" | tr ' ' '\n' | head -1)
    log "OSX: Using SDK version $OSX_SDK_VERSION"

    XCODE_PATH=$(xcode-select -print-path 2>/dev/null)
    log "OSX: XCode path: $XCODE_PATH"

    OSX_SDK_ROOT=$XCODE_PATH/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk
    log "OSX: Looking for $OSX_SDK_ROOT"
    if [ ! -d "$OSX_SDK_ROOT" ]; then
        OSX_SDK_ROOT=/Developer/SDKs/MaxOSX${OSX_SDK_VERSION}.sdk
        log "OSX: Looking for $OSX_SDK_ROOT"
        if [ ! -d "$OSX_SDK_ROOT" ]; then
            echo "ERROR: Could not find SDK $OSX_SDK_VERSION at $OSX_SDK_ROOT"
            exit 1
        fi
    fi
    echo "OSX SDK   : Found at $OSX_SDK_ROOT"
fi

# we can build the emulator with Cygwin, so enable it
enable_cygwin

setup_toolchain

###
###  SDL Probe
###

if [ -n "$SDL_CONFIG" ] ; then

	# check that we can link statically with the library.
	#
	SDL_CFLAGS=`$SDL_CONFIG --cflags`
	SDL_LIBS=`$SDL_CONFIG --static-libs`

	# quick hack, remove the -D_GNU_SOURCE=1 of some SDL Cflags
	# since they break recent Mingw releases
	SDL_CFLAGS=`echo $SDL_CFLAGS | sed -e s/-D_GNU_SOURCE=1//g`

	log "SDL-probe  : SDL_CFLAGS = $SDL_CFLAGS"
	log "SDL-probe  : SDL_LIBS   = $SDL_LIBS"


	EXTRA_CFLAGS="$SDL_CFLAGS"
	EXTRA_LDFLAGS="$SDL_LIBS"

	case "$OS" in
		freebsd-*)
		EXTRA_LDFLAGS="$EXTRA_LDFLAGS -lm -lpthread"
		;;
	esac

	cat > $TMPC << EOF
#include <SDL.h>
#undef main
int main( int argc, char** argv ) {
   return SDL_Init (SDL_INIT_VIDEO);
}
EOF
	feature_check_link  SDL_LINKING

	if [ $SDL_LINKING != "yes" ] ; then
		echo "You provided an explicit sdl-config script, but the corresponding library"
		echo "cannot be statically linked with the Android emulator directly."
		echo "Error message:"
		cat $TMPL
		clean_exit
	fi
	log "SDL-probe  : static linking ok"

	# now, let's check that the SDL library has the special functions
	# we added to our own sources
	#
	cat > $TMPC << EOF
#include <SDL.h>
#undef main
int main( int argc, char** argv ) {
	int  x, y;
	SDL_Rect  r;
	SDL_WM_GetPos(&x, &y);
	SDL_WM_SetPos(x, y);
	SDL_WM_GetMonitorDPI(&x, &y);
	SDL_WM_GetMonitorRect(&r);
	return SDL_Init (SDL_INIT_VIDEO);
}
EOF
	feature_check_link  SDL_LINKING

	if [ $SDL_LINKING != "yes" ] ; then
		echo "You provided an explicit sdl-config script in SDL_CONFIG, but the"
		echo "corresponding library doesn't have the patches required to link"
		echo "with the Android emulator. Unsetting SDL_CONFIG will use the"
		echo "sources bundled with the emulator instead"
		echo "Error:"
		cat $TMPL
		clean_exit
	fi

	log "SDL-probe  : extra features ok"
	clean_temp

	EXTRA_CFLAGS=
	EXTRA_LDFLAGS=
fi

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

ORG_CFLAGS=$CFLAGS
ORG_LDFLAGS=$LDFLAGS

if [ "$OPTION_IGNORE_AUDIO" = "yes" ] ; then
PROBE_ESD_ESD=no
PROBE_ALSA=no
PROBE_PULSEAUDIO=no
fi

# Probe a system library
#
# $1: Variable name (e.g. PROBE_ESD)
# $2: Library name (e.g. "Alsa")
# $3: Path to source file for probe program (e.g. android/config/check-alsa.c)
# $4: Package name (e.g. libasound-dev)
#
probe_system_library ()
{
    if [ `var_value $1` = yes ] ; then
        CFLAGS="$ORG_CFLAGS"
        LDFLAGS="$ORG_LDFLAGS -ldl"
        cp -f android/config/check-esd.c $TMPC
        compile
        if [ $? = 0 ] ; then
            log "AudioProbe : $2 seems to be usable on this system"
        else
            if [ "$OPTION_IGNORE_AUDIO" = no ] ; then
                echo "The $2 development files do not seem to be installed on this system"
                echo "Are you missing the $4 package ?"
                echo "You can ignore this error with --ignore-audio, otherwise correct"
                echo "the issue(s) below and try again:"
                cat $TMPL
                clean_exit
            fi
            eval $1=no
            log "AudioProbe : $2 seems to be UNUSABLE on this system !!"
        fi
        clean_temp
    fi
}

probe_system_library PROBE_ESD        ESounD     android/config/check-esd.c libesd-dev
probe_system_library PROBE_ALSA       Alsa       android/config/check-alsa.c libasound-dev
probe_system_library PROBE_PULSEAUDIO PulseAudio android/config/check-pulseaudio.c libpulse-dev

CFLAGS=$ORG_CFLAGS
LDFLAGS=$ORG_LDFLAGS

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

# Strip executables and shared libraries unless --debug is used.
if [ "$OPTION_DEBUG" != "yes" -a "$OPTION_NO_STRIP" != "yes" ]; then
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
if [ -n "$SDL_CONFIG" ] ; then
echo "QEMU_SDL_CONFIG   := $SDL_CONFIG" >> $config_mk
fi
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
if [ $OPTION_STATIC = yes ] ; then
    echo "CONFIG_STATIC_EXECUTABLE := true" >> $config_mk
fi
if [ "$GLES_SUPPORT" = "yes" ]; then
    echo "EMULATOR_BUILD_EMUGL       := true" >> $config_mk
    echo "EMULATOR_EMUGL_SOURCES_DIR := $GLES_DIR" >> $config_mk
fi

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

if [ "$OPTION_STATIC" = "yes" ] ; then
    echo "CONFIG_STATIC_EXECUTABLE := true" >> $config_mk
    echo "#define CONFIG_STATIC_EXECUTABLE  1" >> $config_h
fi

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
