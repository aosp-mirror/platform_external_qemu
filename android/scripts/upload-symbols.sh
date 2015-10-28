#!/bin/bash

PRODUCT=AndroidEmulator
STAGING_URL=https://clients2.google.com/cr/staging_symbol # staging
PROD_URL=https://clients2.google.com/cr/symbol # production
URL=

if [ $# -ne 2 ] || ( [ $1 != 'staging' ] && [ $1 != 'prod' ] ) ; then
    echo "Usage: `basename "$0"` <staging|prod> <path_to_symbols_dir>" >&2
    exit 1
fi

if [ $1 == 'staging' ]; then
    URL=$STAGING_URL
else
    URL=$PROD_URL
fi
SYMBOL_DIR=$2

process_symbol () {
    SYMBOL_FILE="$1"
    LINE=$(sed -n -e 1p "$SYMBOL_FILE")
    read MODULE OS ARCH DEBUG_IDENTIFIER DEBUG_FILE <<<"$LINE"
    if [ "$MODULE" != MODULE ]; then
        echo "Corrupt symbol file $SYMBOL_FILE" >&2
        exit 1
    fi

    CODE_FILE=$DEBUG_FILE
    CODE_IDENTIFIER=$DEBUG_IDENTIFIER

    curl \
        --dump-header /dev/null \
        --form product="$PRODUCT" \
        --form codeFile="$CODE_FILE" \
        --form codeIdentifier="$CODE_IDENTIFIER" \
        --form debugFile="$DEBUG_FILE" \
        --form debugIdentifier="$DEBUG_IDENTIFIER" \
        --form symbolFile="@\"$SYMBOL_FILE\"" \
        "$URL"

    rc=$?

    if [[ $rc != 0 ]]; then
        echo "Curl failed with return code $rc" >&2
        exit 1
    fi
}


find $SYMBOL_DIR -type f -print0 -name "*.sym" | while read -d $'\0' file; do
    echo "Processing $file"
    process_symbol $file
done
