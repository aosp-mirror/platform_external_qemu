#!/bin/sh
#
# this script is used to rebuild all QEMU binaries for the host
# platforms.
#
# assume that the device tree is in TOP
#
. $(dirname "$0")/scripts/unix/utils/common.shi
PROGDIR=`dirname $0`
set -e
export LANG=C
export LC_ALL=C

exists() {
  # Returns true if $1 is on the path.
  command -v "$1" >/dev/null 2>&1
}

exists python || panic "No python interpreter available, please install python!"

python $PROGDIR/build/python/cmake.py   $*
