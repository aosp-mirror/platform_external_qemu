#!/bin/sh

# Copyright 2018 The Android Open Source Project
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
PROGDIR=`dirname $0`

shell_import utils/aosp_dir.shi
shell_import utils/temp_dir.shi
shell_import utils/option_parser.shi

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Configure the cmake project and generate the build files"

OPTION_DEBUG=no
OPTION_SANITIZER=none
OPTION_OUT_DIR=objs
OPTION_STRIP=no
OPTION_CRASHUPLOAD=NONE
OPTION_MINGW=
OPTION_WINDOWS_MSVC=
OPTION_SYMBOLS=no
OPTION_SDK_REVISION=
OPTION_SDK_BUILD=
OPTION_GOLDFISH_OPENGL_DIR=no

# Select a default generator, ninja is very fast, so you should pick that
# if available..
OPTION_GENERATOR="auto"

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

aosp_dir_register_option
option_register_var "--sdk-revision=<revision>" OPTION_SDK_REVISION "The emulator sdk revision"
option_register_var "--sdk-build-number=<build_number>" OPTION_SDK_BUILD "The emulator sdk build number"
option_register_var "--debug" OPTION_DEBUG "Build debug version of the emulator"
option_register_var "--strip" OPTION_STRIP "Strip emulator executables"
option_register_var "--symbols" OPTION_SYMBOLS "Generate Breakpad symbol files."
option_register_var "--crash-staging" OPTION_CRASH_STAGING "Send crashes to staging server."
option_register_var "--crash-prod" OPTION_CRASH_PROD "Send crashes to production server."
option_register_var "--out-dir=<path>" OPTION_OUT_DIR "Use specific output directory"
option_register_var "--mingw" OPTION_MINGW "Build Windows executable on Linux using mingw"
option_register_var "--windows-msvc" OPTION_MSVC "Build Windows executable on Linux using Windows SDK + clang (only 64-bit for now)"
option_register_var "--force-fetch-wintoolchain" OPTION_WINTOOLCHAIN "Force refresh the Windows SDK, and replace the current one if any (windows-msvc build)"
option_register_var "--sanitizer=<..>" OPTION_SANITIZER "Build with LLVM sanitizer (sanitizer=[address, thread])"
option_register_var "--generator=<..>" OPTION_GENERATOR "Use the given generator (ninja, make, Xcode, etc.)"
option_register_var "--list-generators" OPTION_LIST_GEN "List available generators"
option_register_var "--no-tests" OPTION_NO_TESTS "Do not run the unit tests"

aosp_dir_parse_option

option_parse "$@"



# Some sanity checks.
if [ "$OPTION_MINGW" -a "$OPTION_MSVC" ]; then
    panic "Choose either --mingw or --windows-msvc, not both."
fi

if [ "$OPTION_CRASH_STAGING" -a "$OPTION_CRASH_PROD" ]; then
    panic "Choose either --crash-staging or --crash-prod, not both."
fi

if [ "$OPTION_WINTOOLCHAIN" -a -z "$OPTION_MSVC" ]; then
    panic "--force-fetch-wintoolchain only for --windows-msvc build."
fi

log_invocation
QEMU_TOP=$AOSP_DIR/external/qemu

# Configure the toolchain we should use
TOOLCHAIN=
case $(get_build_os) in
    linux)
        TOOLCHAIN=$QEMU_TOP/android/build/cmake/toolchain-linux-x86_64.cmake
        CMAKE=$AOSP_DIR/prebuilts/cmake/linux-x86/bin/cmake
        # On linux we can use the shipped ninja version
        if [ "$OPTION_GENERATOR" = "auto" ]; then
           OPTION_GENERATOR=ninja 
        fi
        ;;
    darwin)
        TOOLCHAIN=$QEMU_TOP/android/build/cmake/toolchain-darwin-x86_64.cmake
        CMAKE=$AOSP_DIR/prebuilts/cmake/darwin-x86/bin/cmake
        # We pick ninja if it is on the path, otherwise plain old make.
        OPTION_GENERATOR=$(which ninja >/dev/null && echo "ninja" || echo "make")
        ;;
    *)
        panic "Don't know how to build binaries on this system [$(get_build_os)]"
esac

if [ "$OPTION_LIST_GEN" ]; then
 $CMAKE --help | sed -n -e '/Generators/,$p'
 echo "Note, it might be possible that your ide (CLion for example) has direct support for cmake."
 exit 0
fi

if [ "$OPTION_MSVC" ]; then
    TOOLCHAIN=$QEMU_TOP/android/build/cmake/toolchain-windows_msvc-x86_64.cmake
fi

if [ "$OPTION_MINGW" ]; then
    TOOLCHAIN=$QEMU_TOP/android/build/cmake/toolchain-windows-x86_64.cmake
fi

if [ "$OPTION_CRASH_STAGING" ]; then
   OPTION_CRASHUPLOAD=STAGING
fi

if [ "$OPTION_CRASH_PROD" ]; then
   OPTION_CRASHUPLOAD=PROD
fi


# Setup the parameters we need to pass into cmake generator
CMAKE_PARAMS=

if [ "$OPTION_SDK_REVISION" ]; then
   CMAKE_PARAMS="${CMAKE_PARAMS} -DOPTION_SDK_TOOLS_REVISION=$OPTION_SDK_REVISION"
fi;
if [ "$OPTION_SDK_BUILD" ]; then
   CMAKE_PARAMS="${CMAKE_PARAMS} -DOPTION_SDK_TOOLS_BUILD_NUMBER=$OPTION_SDK_BUILD"
fi;

if [ "$OPTION_SANITIZER" != "none" ] ; then
  CMAKE_PARAMS="${CMAKE_PARAMS}  -DOPTION_ASAN=$OPTION_SANITIZER"
fi

if [ "$OPTION_DEBUG" != "no" ]; then
   CMAKE_PARAMS="${CMAKE_PARAMS}  -DCMAKE_BUILD_TYPE=Debug"
else
   CMAKE_PARAMS="${CMAKE_PARAMS}  -DCMAKE_BUILD_TYPE=Release"
fi

if [ "$OPTION_WINTOOLCHAIN" ]; then
   CMAKE_PARAMS="${CMAKE_PARAMS}  -DOPTION_WINTOOLCHAIN=1"
fi

if [ "$OPTION_GENERATOR" = "ninja" ]; then
   CMAKE_PARAMS="${CMAKE_PARAMS}  -G Ninja"
elif [ "$OPTION_GENERATOR" = "make" ]; then
   CMAKE_PARAMS="${CMAKE_PARAMS} -G \"Unix Makefiles\""
else
   CMAKE_PARAMS="${CMAKE_PARAMS} -G \"${OPTION_GENERATOR}\""
fi

run rm -rf $OPTION_OUT_DIR
run mkdir -p $OPTION_OUT_DIR
run eval $CMAKE \
       -H$QEMU_TOP \
       -B$OPTION_OUT_DIR \
       -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
       -DOPTION_CRASHUPLOAD=$OPTION_CRASHUPLOAD \
       $CMAKE_PARAMS

echo "Ready to go. Type ${GREEN}'${OPTION_GENERATOR} -C ${OPTION_OUT_DIR}'${RESET} to build emulator, and ${GREEN}'${OPTION_GENERATOR} -C ${OPTION_OUT_DIR} tests'${RESET} to run the unit tests."
