#!/bin/bash

# This post install script can be run on aarch64-linux to modify the
# interpreter and rpath for emulator and qemu-system-aarch64* binaries
# to use the lib64/compat/gcclib which has GCC 2.29
#
# Alternatively,
# The user can also run this on x86_64 machine with a given gcclib path
# and re-package emulator and deploy it on aarch64-linux host

gcclibpath=""
SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")

if [ "$#" -eq 1 ]; then
gcclibpath=$1
echo "Using user provided gcc lib path $gcclibpath"
elif [ "$#" -eq 0 ]; then 
gcclibpath=$BASEDIR/lib64/compat/gcclib
echo "Using default gcc lib path $gcclibpath"
if [ ! -f $gcclibpath/ld-linux-aarch64.so.1 ]; then
  if [ -f $BASEDIR/lib64/compat/gcclib.tgz ]; then
    echo "unpacking default gcc lib"
    tar zxvf $BASEDIR/lib64/compat/gcclib.tgz -C $BASEDIR/lib64/compat
    if [ ! -f $gcclibpath/ld-linux-aarch64.so.1 ]; then
      echo "cannot find ld-linux-aarch64.so.1 in given gcc lib path: $gcclibpath"
      exit 1
    fi
  else
    echo "cannot find default gcc lib"
    exit 2
  fi
fi
else
echo "Usage $0 <gcc-lib-path>"
echo "Usage $0"
exit 3
fi


set -e

PATCHELF="/usr/bin/patchelf"

if [ ! -f $PATCHELF ]; then
echo "please ensure $PATCHELF exists and try again."
exit 4
fi

# now run patchelf on emulator and qemu-system-aarch64-headless
echo "Modifying emulator..."
$PATCHELF --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $BASEDIR/emulator

echo "Modifying qemu-system-aarch64-headless ..."
$PATCHELF --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $BASEDIR/qemu/linux-aarch64/qemu-system-aarch64-headless

echo "Modifying qemu-system-aarch64 ..."
$PATCHELF --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $BASEDIR/qemu/linux-aarch64/qemu-system-aarch64
echo "Done"
