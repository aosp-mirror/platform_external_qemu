# The posix way of getting the full path..
PROGDIR="$( cd "$(dirname "$0")" ; pwd -P )"
export ASAN_OPTIONS=`cat android/asan_overrides`
export PATH=$PATH:$PROGDIR/third_party/chromium/depot_tools