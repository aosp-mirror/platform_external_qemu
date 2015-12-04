#!/bin/bash

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

shell_import utils/option_parser.shi

PRODUCT=AndroidEmulator
STAGING_URL=https://clients2.google.com/cr/staging_symbol # staging
PROD_URL=https://clients2.google.com/cr/symbol # production


###
###  COMMAND LINE PARSING
###

OPT_SYMBOL_DIR=
option_register_var "--symbol-dir=<path>" OPT_SYMBOL_DIR \
       "Path to Breakpad symbol dir."

OPT_CRASH_STAGING=
option_register_var "--crash-staging" OPT_CRASH_STAGING \
       "Upload crashes to staging."

OPT_CRASH_PROD=
option_register_var "--crash-prod" OPT_CRASH_PROD \
       "Upload crashes to production."


PROGRAM_DESCRIPTION=\
"Uploads a symbol directory to either staging or production crash server

Use --crash-prod or --crash-staging to select between servers

Use --symbol-dir to provide symbol directory

Symbol directory structure matches objs/build/symbols:
> symbol_dir
  > binary_name
    > binary build id
      > binary_name.sym"

option_parse "$@"

SYMBOL_DIR=
if [ -z "$OPT_SYMBOL_DIR" ]; then
  panic "--symbol-dir=<path> is required"
else
  SYMBOL_DIR="$OPT_SYMBOL_DIR"
fi

if [ "$OPT_CRASH_PROD" ] && [ "$OPT_CRASH_STAGING" ]; then
  panic "Can not use both --crash-staging and --crash-prod at the same time"
fi

if [ -z "$OPT_CRASH_PROD" ] && [ -z "$OPT_CRASH_STAGING" ]; then
  panic "Must use either --crash-staging or --crash-prod"
fi

URL=
if [ "$OPT_CRASH_PROD" ]; then
  URL=$PROD_URL
else
  URL=$STAGING_URL
fi

process_symbol () {
    SYMBOL_FILE="$1"
    LINE=$(sed -n -e 1p "$SYMBOL_FILE")
    read MODULE OS ARCH DEBUG_IDENTIFIER DEBUG_FILE <<<"$LINE"
    if [ "$MODULE" != MODULE ]; then
        panic "Corrupt symbol file $SYMBOL_FILE"
    fi

    CODE_FILE=$DEBUG_FILE
    CODE_IDENTIFIER=$DEBUG_IDENTIFIER

    curl \
        --verbose \
        --show-error \
        --dump-header /dev/null \
        --form product="$PRODUCT" \
        --form codeFile="$CODE_FILE" \
        --form codeIdentifier="$CODE_IDENTIFIER" \
        --form debugFile="$DEBUG_FILE" \
        --form debugIdentifier="$DEBUG_IDENTIFIER" \
        --form symbolFile="@$SYMBOL_FILE" \
        "$URL" ||
            panic "Curl failed with return code $?"
}


find $SYMBOL_DIR -type f -print0 -name "*.sym" | while read -d $'\0' file; do
    echo "Processing $file"
    process_symbol $file
done
