#!/bin/bash

# this script changes the gcclib path in the emulator
# binaries (emulator and qemu-system-aarch64*) to
# use the glibc 2.29 to work on older linux distro.
#
# this script should be run when emulator is installed
# or moved to a different directory

RUNROOT=$PWD

set -e

GLIBC="${RUNROOT}/lib64/glibc-2.29.tgz"

if [ ! -f $GLIBC ]; then
echo "please ensure $GLIBC exists in current directory and try again."
exit 1
fi

# need this to modify the interpreter path and rpath
PATCHELF="${RUNROOT}/patchelf.tgz"

if [ ! -f $PATCHELF ]; then
echo "please ensure $PATCHELF exists in current directory and try again."
exit 2
fi

mkdir -p $RUNROOT/lib64/glibc
tar xzvf $GLIBC -C $RUNROOT/lib64/glibc

mkdir -p $RUNROOT/bin64
tar xzvf $PATCHELF -C $RUNROOT/bin64

# now run patchelf on emulator and qemu-system-aarch64-headless
gcclibpath=$RUNROOT/lib64/glibc/lib
$RUNROOT/bin64/patchelf --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $RUNROOT/emulator

$RUNROOT/bin64/patchelf --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $RUNROOT/qemu/linux-aarch64/qemu-system-aarch64-headless

$RUNROOT/bin64/patchelf --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $RUNROOT/qemu/linux-aarch64/qemu-system-aarch64
