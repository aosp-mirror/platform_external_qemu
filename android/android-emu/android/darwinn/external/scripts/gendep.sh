#!/bin/bash

# Generates dependencies from Blaze BUILD
# Usage: . ./gendep.sh <client name>
#
# This script is hardcoded to generate the dependencies for:
# //third_party/darwinn/driver/beagle:beagle_all_driver_provider and
# //third_party/darwinn/driver/noronha:noronha_driver_provider
CLIENT=$1

# Queries and converts a given Google3 Blaze target to Android Blueprint
# dependencies.
# Only takes dependencies from within DarwiNN third_party/.
function find_google3_dep {
  pushd . > /dev/null
  g4d $CLIENT
  echo "        // from $1"
  # produce the deps within //third_party/darwinn
  blaze cquery \
    --config=android_arm64 \
    "filter('//third_party/darwinn', filter('\.cc$', kind('source file', deps($1))))" | \
    awk -F '[ :]' '{ print "        \"" substr($1,3) "/"$2"\"," }' | sort
  popd > /dev/null
}

function expand_dep {
  m4 -D$1="`find_google3_dep $2`" $3
}

# Populates dependencies.
# TODO (ijsung): need better ways to update multiple targets.
echo "// Generated file. Do not edit." > src/Android.bp
echo "// by gendep.sh from src/Android.bp.m4." >> src/Android.bp
expand_dep __BEAGLE_DEP__ \
  //third_party/darwinn/driver/beagle:beagle_all_driver_provider src/Android.bp.m4 | \
expand_dep __NORONHA_DEP__ \
  //third_party/darwinn/driver/noronha:noronha_driver_provider | \
expand_dep __TEST_UTIL_DEP__ \
  //third_party/darwinn/driver:test_util - \
  >> src/Android.bp
