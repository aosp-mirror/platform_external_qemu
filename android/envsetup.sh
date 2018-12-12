#!/bin/bash
# We forcefully use bash to make sure we can find the proper location of this script
# So we can add ninja to the path, and setup the ASAN_OPTIONS
PROGDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
export ASAN_OPTIONS=$(cat ${PROGDIR}/asan_overrides)
if [ $(which ninja) ] ; then
  echo "Ninja is already on the path, no need to add it."
else 
  export PATH=$PATH:$PROGDIR/third_party/chromium/depot_tools
fi
