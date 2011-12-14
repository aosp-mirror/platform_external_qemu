#!/bin/sh
#
# This is a wrapper around our x86 toolchain that allows us to add a few
# compiler flags.
# The issue is that our x86 toolchain is NDK-compatible, and hence enforces
# -mfpmath=sse and -fpic by default. When building the kernel, we need to
# disable this.
#
# Also support ccache compilation if USE_CCACHE is defined as "1"
#

# REAL_CROSS_COMPILE must be defined, and its value must be one of the
# CROSS_COMPILE values that are supported by the Kernel build system
# (e.g. "i686-android-linux-")
#
if [ -z "$REAL_CROSS_COMPILE" ]; then
    echo "ERROR: The REAL_CROSS_COMPILE environment variable should be defined!"
    exit 1
fi

# ARCH must also be defined before calling this script, e.g. 'arm' or 'x86'
#
if [ -z "$ARCH" ]; then
    echo "ERROR: ARCH must be defined!"
    exit 1
fi

# Common prefix for all fake toolchain programs, which are all really
# symlinks to this script, i.e.
#
# $PROGPREFIX-gcc  --> $0
# $PROGPREFIX-ld   --> $0
# etc...
#
PROGPREFIX=android-kernel-toolchain-

# Get program name, must be of the form $PROGPREFIX-<suffix>, where
# <suffix> can be 'gcc', 'ld', etc... We expect that the fake toolchain
# files are all symlinks to this script.
#
PROGNAME=$(basename "$0")
PROGSUFFIX=${PROGNAME##$PROGPREFIX}

EXTRA_FLAGS=

# Special case #1: For x86, disable SSE FPU arithmetic, and PIC code
if [ "$ARCH" = "x86" -a "$PROGSUFFIX" = gcc ]; then
    EXTRA_FLAGS=$EXTRA_FLAGS" -mfpmath=387 -fno-pic"
fi

# Invoke real cross-compiler toolchain program now
${REAL_CROSS_COMPILE}$PROGSUFFIX $EXTRA_FLAGS "$@"
