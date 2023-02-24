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

PROGRAM_PARAMETERS="<regex-files-of-interest>"

PROGRAM_DESCRIPTION=\
"Run include what you use on the android emulator code base.

It assumes that include-what-you-use is on the path, and that you have
generated a compilation database as follows:

$ android/rebuild.sh --cmake_option CMAKE_EXPORT_COMPILE_COMMANDS=ON

To analyze:

$ ./android/scripts/unix/iwyu.sh \".*qemu/android/.*\" | tee sample.yaml

To fix:

$ ./android/scripts/unix/iwyu.sh --fix < sample.yaml > patch.p1

See the documenation in android/scripts/iwyu/README.md for details.
"
OPT_FIX=
OPT_SAFE=
OPT_BUILD=objs
option_register_var "--out=<dir-with-compiler-database>" OPT_BUILD "Build directory with compile database."
option_register_var "-j<count>" OPT_NUM_JOBS "Run <count> parallel build jobs [$(get_build_num_cores)]"
option_register_var "--jobs=<count>" OPT_NUM_JOBS "Same as -j<count>."
option_register_var "--fix" OPT_FIX "Run in fix mode"

aosp_dir_register_option
aosp_dir_parse_option
option_parse "$@"
IWYU=$AOSP_DIR/external/qemu/android/scripts/iwyu/


function analyze() {
  # TODO cross compile so we can do windows cleanup?
  local BUILD_HOST=$(get_build_os)

  log2 "Using $CXXFLAGS"
  log2 "C++ ${LIBCINCDIR}"

  if [ "$OPT_NUM_JOBS" ]; then
    NUM_JOBS=$OPT_NUM_JOBS
    log2 "Parallel jobs count: $NUM_JOBS"
  else
    NUM_JOBS=$(get_build_num_cores)
    log2 "Auto-config: --jobs=$NUM_JOBS"
  fi

  GEN=$(dirname "$0")/gen-android-sdk-toolchain.sh

  # Extract our compiler configuration
  CFLAGS=$(${GEN} --print=cflags unused)
  CXXFLAGS=$(${GEN} --print=cxxflags unused)
  QEMU=${AOSP}/external/qemu

  # We first configure a mapping file, which we need to translate messed up
  # includes.
  COMPILER_FLAGS="-Xiwyu --mapping_file=$AOSP_DIR/external/qemu/android/scripts/iwyu/emulator.imp"
  case "$BUILD_HOST" in
    linux*)
      # Make sure libc++ is on the path in the right place. Needs to be the
      # first, lets you get total weirdness!!
      LIBCINCDIR=$(${GEN} --print=inccplusplus unused)
      var_append COMPILER_FLAGS "-isystem ${LIBCINCDIR}"
      ;;
    *)
      ;;
  esac


  var_append COMPILER_FLAGS "${CFLAGS} ${CXXFLAGS}"
  log $IWYU/iwyu_tool.py  -cflags "${COMPILER_FLAGS}" -j ${NUM_JOBS} -p ${OPT_BUILD} ${PARAMETER_1} -- -Xiwyu --update_comments -Xiwyu --no_fwd_decls --cxx17ns
  $IWYU/iwyu_tool.py  -cflags "${COMPILER_FLAGS}" -j ${NUM_JOBS} -p ${OPT_BUILD} ${PARAMETER_1} -- -Xiwyu --update_comments  -Xiwyu --no_fwd_decls -Xiwyu --cxx17ns
}


function fix() {
   BUILD_DIR=$(realpath ${OPT_BUILD})
   log $IWYU/fix_includes.py  --ignore_re "${BUILD_DIR}/.*" --nocomments --nosafe_headers -n -p ${OPT_BUILD}
   $IWYU/fix_includes.py  --ignore_re "${BUILD_DIR}/.*" --nocomments --nosafe_headers --reorder  -p ${OPT_BUILD}
}


if [ "$OPT_FIX" ]; then
  fix
else
  analyze
fi
