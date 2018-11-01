#!/bin/sh
#
# this script is used to rebuild the Android emulator from sources
# in the current directory. It also contains logic to speed up the
# rebuild if it detects that you're using the Android build system
#
# here's the list of environment variables you can define before
# calling this script to control it (besides options):
#
#

# first, let's see which system we're running this on
PROGNAME=`basename $0`
PROGDIR=`dirname $0`

${PROGDIR}/scripts/config-cmake.sh "$@"
