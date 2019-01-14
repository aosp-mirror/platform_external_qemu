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

shell_import utils/option_parser.shi
shell_import utils/temp_dir.shi

PROGRAM_PARAMETERS="<trace-file>"

PROGRAM_DESCRIPTION=\
"A simple script to convert a tracefile to a human readable output"

OPT_OUTPUT=
option_register_var "--output=<output>" OPT_TARGET \
  "File to write the result to (defaults to stdout)."

OPT_DELETE=
option_register_var "--delete" OPT_DELETE \
        "Delete original trace after conversion."


ROOT=$(realpath $(dirname "$0")/../..)
TRACE_PARSER=$ROOT/scripts/simpletrace.py

option_parse "$@"

if [ "$PARAMETER_COUNT" != 1 ]; then
    panic "This script takes one argument. See --help for details."
fi

OPT_TARGET="$PARAMETER_1"

TMP_DIR=
var_create_temp_dir TMP_DIR traces
TRACE_ALL=$TMP_DIR/trace-events-all

find $ROOT -name 'trace-events' -exec cat {} \; > $TRACE_ALL


if [ "$OPT_OUTPUT" ]; then
  $TRACE_PARSER $TRACE_ALL $OPT_TARGET >OPT_OUTPUT
else
  $TRACE_PARSER $TRACE_ALL $OPT_TARGET
fi

if [ "$OPT_DELETE" ]; then
  rm $OPT_TARGET
fi


