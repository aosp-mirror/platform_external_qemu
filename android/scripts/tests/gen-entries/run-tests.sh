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
#

PROGDIR=$(dirname "$0")

TOPDIR=$(cd $PROGDIR/../.. && pwd -P)

# Remove leading ./ if applicable.
PROGDIR=${PROGDIR#./}

# Assume we are under .../android/scripts/tests/gen-entries/
# and look for .../android/scripts/unix/utils/common.shi

_SHU_PROGDIR=$TOPDIR/unix/utils
. $_SHU_PROGDIR/common.shi

shell_import option_parser.shi

PROGRAM_PARAMETERS=

PROGRAM_DESCRIPTION=\
"Run the test suite for the gen-entries.py script. This looks for
files named t.<number>/<name>.entries, and uses them as input to
'gen-entries.py'. It then compares the output to t.<number>/<name>.<mode> ."

OPT_SCRIPT=$TOPDIR/gen-entries.py
option_register_var "--script=<script>" OPT_SCRIPT "Specify alternate gen-entries.py script." "$SCRIPT"

OPT_OUT_DIR=
option_register_var "--out-dir=<dir>" OPT_OUT_DIR "Generate outputs into <dir>"

OPT_TOOL=
option_register_var "--tool=<tool>" OPT_TOOL "Launch visual diff tool in case of differences."

option_parse "$@"

# Check gen-entries.py script.
if [ ! -f "$OPT_SCRIPT" ]; then
    panic "Cannot find script: $OPT_SCRIPT"
fi

# Create output directory.
OUT_DIR=
if [ "$OPT_OUT_DIR" ]; then
    OUT_DIR=$OPT_OUT_DIR
else
    OUT_DIR=/tmp/emulator-gen-entries-testing
    echo "Auto-config: --out-dir=$OUT_DIR"
fi
mkdir -p "$OUT_DIR" && rm -rf "$OUT_DIR/gen-entries"

OUT_DIR=$OUT_DIR/gen-entries

MODES="def sym wrapper symbols _symbols functions funcargs"

# Find test directories
TEST_DIRS=$(cd "$PROGDIR" && find . -name "t.*" | sed -e 's|^\./||')
for TEST_DIR in $TEST_DIRS; do
    IN=$PROGDIR/$TEST_DIR
    NAMES=$(cd $IN && find . -name "*.entries" | sed -e 's|^\./||g' -e 's|\.entries$||g')
    OUT=$OUT_DIR/$TEST_DIR
    mkdir -p "$OUT"
    for NAME in $NAMES; do
        echo "Processing $TEST_DIR/$NAME.entries"
        run cp -f "$IN/$NAME.entries" "$OUT/$NAME.entries"
        for MODE in $MODES; do
            python3 $OPT_SCRIPT --mode=$MODE "$IN/$NAME.entries" --output=$OUT/$NAME.$MODE
        done
    done
    if ! diff -qr "$IN" "$OUT"; then
        if [ "$OPT_TOOL" ]; then
            $OPT_TOOL "$IN" "$OUT"
        else
            echo "ERROR: Invalid differences between actual and expected output!"
            diff -burN "$IN" "$OUT"
            exit 1
        fi
    fi
done

echo "All good!"
exit 0
