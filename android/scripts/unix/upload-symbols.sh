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

OPT_MY_URL=
option_register_var "--symbol-url=<url>" OPT_MY_URL \
       "Use this url for uploading symbols"

PROGRAM_DESCRIPTION=\
"Uploads a symbol directory to either staging or production crash server

Use --crash-prod or --crash-staging to select between servers or provide
your own with --symbol-url=0

Use --symbol-dir to provide symbol directory

Symbol directory contains any structure of *.sym files:"

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

if [ -z "$OPT_CRASH_PROD" ] && [ -z "$OPT_CRASH_STAGING" ] && [ -z "OPT_MY_URL" ]; then
  panic "Must use either --crash-staging, --crash-prod or --symbol-url"
fi

URL=
if [ "$OPT_CRASH_PROD" ]; then
  URL=$PROD_URL
else
  URL=$STAGING_URL
fi

if [ "$OPT_MY_URL" ]; then
    URL=$OPT_MY_URL
fi

process_symbol () {
    set +e
    SYMBOL_FILE="$1"
    LINE=$(sed -n -e 1p "$SYMBOL_FILE")
    read MODULE OS ARCH DEBUG_IDENTIFIER DEBUG_FILE <<<"$LINE"
    if [ "$MODULE" != MODULE ]; then
        panic "Corrupt symbol file $SYMBOL_FILE"
    fi

    CODE_FILE=$DEBUG_FILE
    CODE_IDENTIFIER=$DEBUG_IDENTIFIER

    START=$(date +%s)
    STATUS=$(curl \
        --show-error \
        --silent \
        --dump-header /dev/null \
        --form product="$PRODUCT" \
        --form codeFile="$CODE_FILE" \
        --form codeIdentifier="$CODE_IDENTIFIER" \
        --form debugFile="$DEBUG_FILE" \
        --form debugIdentifier="$DEBUG_IDENTIFIER" \
        --form symbolFile="@$SYMBOL_FILE" \
        --write-out "%{http_code}\n" \
        -o /dev/null \
        "$URL")
    ERROR=$?
    END=$(date +%s)
    DIFF=$(( $END - $START))
    if [ $STATUS -ne 200 ] && [ $STATUS -ne 204 ] || [ $ERROR -ne 0 ]; then
        warn "Unable to upload $SYMBOL_FILE to $URL, status code: $STATUS, after $DIFF sec."
    else
        log "Uploaded $SYMBOL_FILE -> $URL in $DIFF sec."
    fi
    set -e
}

process_dir () {
    find $SYMBOL_DIR -type f -name "*.sym" -print0 | while read -d $'\0' file; do
        process_symbol $file
    done
}

warn "Symbol processing is now done in the cmake build, there is no need to run this as part of the normal build. This is only needed for prebuilts."
log_invocation
process_dir
