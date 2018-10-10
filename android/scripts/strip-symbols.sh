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
# The posix way of getting the full path..
PROGDIR="$( cd "$(dirname "$0")" ; pwd -P )"

shell_import utils/aosp_dir.shi
shell_import utils/temp_dir.shi
shell_import utils/option_parser.shi

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


PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Strips all the binaries"

OPT_OUT=objs
option_register_var "--out-dir=<dir>" OPT_OUT "Use specific output directory"

OPT_MINGW=
option_register_var "--mingw" OPT_MINGW "Use mingw symbols stripper"

aosp_dir_register_option
option_parse "$@"
aosp_dir_parse_option

# Import the packagee builder
shell_import utils/package_builder.shi


OLD_DIR=$PWD
cd $OPT_OUT 
case $(get_build_os) in
  linux)
      EXES=$(find . -type f -executable)
      OPT_HOST=linux-x86_64
      _SHU_BUILDER_GNU_CONFIG_HOST=x86_64-linux
      ;;
  darwin)
      EXES=$(find .  -path ./toolchain -prune -o -type f -perm +111)
      OPT_HOST=darwin-x86_64
      _SHU_BUILDER_GNU_CONFIG_HOST=
      ;;
  *)
      panic "Don't know how to build binaries on this system [$(get_build_os)]"
esac

if [ "$OPT_MINGW" ]; then
  OPT_HOST=windows-x86_64
  _SHU_BUILDER_GNU_CONFIG_HOST=x86_64-w64-mingw32
   # TODO MSVC:
   # _SHU_BUILDER_GNU_CONFIG_HOST=x86_64-pc-windows-msvc
fi


_SHU_BUILDER_GNU_CONFIG_HOST_PREFIX=${_SHU_BUILDER_GNU_CONFIG_HOST}-
cd $OLD_DIR
TOOLCHAIN_WRAPPER_DIR=$OPT_OUT/toolchain
# Build the symbols, and strip them.
build_symbols $OPT_OUT $OPT_OUT/build $OPT_HOST $EXES
strip_libs $OPT_OUT $EXES