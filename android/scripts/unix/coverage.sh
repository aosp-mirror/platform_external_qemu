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
"Generate a code coverage report."


OPT_NUM_JOBS=
option_register_var "-j<count>" OPT_NUM_JOBS "Run <count> parallel build jobs [$(get_build_num_cores)]"
option_register_var "--jobs=<count>" OPT_NUM_JOBS "Same as -j<count>."

OPT_OUT=objs
option_register_var "--out-dir=<dir>" OPT_OUT "Use specific output directory"

OPT_CSV=
option_register_var  "--csv" OPT_CSV "Produce a CSV instead of html"

OPT_BUILDID=-1
option_register_var "--buildid=<buildid>" OPT_BUILDID "Use the given build id for csv generation."

OPT_COVERAGE=code-coverage.zip
option_register_var "--coverage=<zipfile>" OPT_COVERAGE "Zip file with all the .gcno, .gcda coverage files."

aosp_dir_register_option

aosp_dir_parse_option
option_parse "$@"

if [ "$OPT_NUM_JOBS" ]; then
    NUM_JOBS=$OPT_NUM_JOBS
    log "Parallel jobs count: $NUM_JOBS"
else
    NUM_JOBS=$(get_build_num_cores)
    log "Auto-config: --jobs=$NUM_JOBS"
fi

OS=$(get_build_os)
CLANG_BINDIR=$AOSP_DIR/$(aosp_prebuilt_clang_dir_for $OS)
QEMU2_TOP_DIR=$(realpath $PROGDIR/../..)/.
ANDROID_SDK_TOOLS_CL_SHA1=$( git log -n 1 --pretty=format:"%H" )

var_create_temp_dir CODE_COVERAGE_DIR codecoverage

function prep_env() {
  # Prepares the python virtual environment
  VENV=$OPT_OUT/build/venv

  if [ ! -f $VENV/bin/activate ]; then
    run python -m virtualenv $VENV
  fi

  source $VENV/bin/activate
  run pip install gcovr
}

if [ "$OPT_BUILDID" ]; then
  run unzip $OPT_COVERAGE -d $CODE_COVERAGE_DIR
fi

prep_env

run mkdir -p "$OPT_OUT/coverage"
if [ "$OPT_CSV" ]; then
  COVXML=$OPT_OUT/coverage/emu-coverage.xml
  run gcovr \
    -j $NUM_JOBS \
    --root $QEMU2_TOP_DIR \
    --print-summary \
    --output $COVXML \
    --gcov-executable "${CLANG_BINDIR}/llvm-cov gcov" \
    --xml \
    --xml-pretty \
    --verbose \
    $CODE_COVERAGE_DIR
  xsltproc --param buildid $OPT_BUILDID \
    $QEMU2_TOP_DIR/android/scripts/coverage.xsl \
    $COVXML > "$OPT_OUT/coverage/emu-coverage.csv"
else
  TITLE=$(git log --pretty=oneline -n 1 $ANDROID_SDK_TOOLS_CL_SHA1)
  run gcovr \
    -j $NUM_JOBS \
    --root $QEMU2_TOP_DIR \
    --print-summary \
    --output $OPT_OUT/coverage/emu-coverage.html \
    --gcov-executable "${CLANG_BINDIR}/llvm-cov gcov" \
    --html-title "$TITLE" \
    --html-details \
    --html \
    --verbose \
    $CODE_COVERAGE_DIR
fi
