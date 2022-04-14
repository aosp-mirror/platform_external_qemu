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

PYTHON=python

# Prefer python3 if it is available.
if ! exists $PYTHON; then
    PYTHON=python3 
    echo "Using python3"
    echo "This mmight not work with QEMU dependencies (b/229251286)!" 
fi

$PYTHON $PROGDIR/build/python/cmake.py   $*
