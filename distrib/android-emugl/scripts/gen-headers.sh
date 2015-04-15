#!/bin/sh

set -e
export LANG=C
export LC_ALL=C

panic () {
    echo "ERROR: $@"
    exit 1
}

PROGDIR=$(dirname "$0")

QEMU_TOP_DIR=$(cd $PROGDIR/../../.. && pwd -P)
SCRIPT_DIR=android/scripts
if [ ! -d "$QEMU_TOP_DIR/$SCRIPT_DIR" ]; then
    panic "Missing scripts directory: $QEMU_TOP_DIR/$SCRIPT_DIR"
fi

cd $QEMU_TOP_DIR
GEN_ENTRIES=$SCRIPT_DIR/gen-entries.py
if [ ! -f "$GEN_ENTRIES" ]; then
    panic "Missing script: $GEN_ENTRIES"
fi

GLCOMMON_SRC_DIR=distrib/android-emugl/host/libs/Translator
GLCOMMON_ENTRIES="gles_common gles_extensions gles1_only gles1_extensions gles2_only \
gles2_extensions"

FAILURES=
for ENTRY in $GLCOMMON_ENTRIES; do
    SRC_FILE=$GLCOMMON_SRC_DIR/GLcommon/${ENTRY}.entries
    DST_FILE=$GLCOMMON_SRC_DIR/include/GLcommon/${ENTRY}_functions.h
    if [ ! -f "$SRC_FILE" ]; then
        echo "ERROR: Missing source file: $SRC_FILE"
        FAILURES=true
    else
        echo "Generating $DST_FILE"
        $GEN_ENTRIES --mode=functions $SRC_FILE --output=$DST_FILE
    fi
done

if [ "$FAILURES" ]; then
    exit 1
fi
