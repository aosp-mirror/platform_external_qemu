#!/bin/sh
echo Wrapper for UIC on linux
DIR=$(dirname "$0")
BASEDIR=$(realpath $DIR/../../../../prebuilts/android-emulator-build/qt/linux-x86_64)
LD_LIBRARY_PATH=$BASEDIR/lib $BASEDIR/bin/uic $@
