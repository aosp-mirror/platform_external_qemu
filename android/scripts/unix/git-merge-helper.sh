#!/bin/sh

# Copyright 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

. $(dirname "$0")/utils/common.shi

shell_import utils/option_parser.shi

PROGRAM_DESCRIPTION=\
"A script that tries to make you merge life slightly easier.

It does this by:

Figuring out which individual commits to merge in to get to the destination tag.
Next it will merge in every Xth commit, and trying to build the repo.

If this fails, you will have to fix the build and manually resolve the merge.

Depending on your appetite for risk you can set the steps parameter to skip
commits, this will decrease the number of merges, but might make it harder to
resolve conflicts.
"

OPT_TAG=
option_register_var "--tag=<tag>" OPT_TAG "Tag, branch or sha1 that you wish to merge in."

OPT_AVD=
option_register_var "--avd=<avd>" OPT_AVD "Avd to launch for simple boot test."

OPT_STEP=5
option_register_var "--steps=<count>" OPT_STEP "The number of commits to skip per merge."

OPT_DIST=linux-x86_64
option_register_var "--dist=<dist>" OPT_DIST "Target distribution for compilation (linux-x86_64, windows-x86_64, darwin-x86_64)"


OPT_DEST=/mnt/ram/mrg
option_register_var "--dest=<dest>" OPT_DIST "Target destination for compilation"
option_parse "$@"

HOST_NUM_CPUS=1

HOST_OS=$(uname -s)
case $HOST_OS in
    Linux)
        HOST_NUM_CPUS=`cat /proc/cpuinfo | grep processor | wc -l`
        ;;
    Darwin|FreeBSD)
        HOST_NUM_CPUS=`sysctl -n hw.ncpu`
        ;;
esac
let HOST_NUM_CPUS="HOST_NUM_CPUS+=1"

do_tac() {
    case $HOST_OS in
        Linux)
            tac -- "$@"
            ;;
        Darwin|FreeBSD)
            tail -r -- "$@"
            ;;
    esac
}


build() {
    # First attempt a partial build
    echo "Building!"
    ninja -C $OPT_DEST ||
   (./android/scripts/unix/build-qemu-android.sh  --build-dir=$OPT_DEST/qemu-build --host=$OPT_DIST &&
    ./android-qemu2-glue/scripts/unix/gen-qemu2-sources-mk.py -i ../../prebuilts/android-emulator-build/qemu-upstream/ -o ./android-qemu2-glue/build/Makefile.qemu2-sources.mk -s $OPT_DIST -r . &&
    ./android/rebuild.sh --no-strip --no-tests --out $OPT_DEST)
}

COMMITS=$((git log --first-parent --format=format:%H HEAD..$OPT_TAG; echo) | do_tac)
LEFT=0
for l in $COMMITS; do
  let "LEFT+=1"
done

COUNTER=0
for SHA in $COMMITS; do
  echo "Considering: $SHA, $LEFT commits left to merge."
  let "COUNTER+=1"
  let "LEFT-=1"
  if [[ $COUNTER -eq $OPT_STEP ]]; then
    echo "Merging in $SHA"
    NEEDS_BUILD=y
    until git merge $SHA
    do
      read -p "Please fix merge $SHA before we continue"
      # Make usually succeeds, but sometimes we need to regenerate things.
      until build
      do
        read -p "Please fix the build before we can move on."
      done
      git add --update
      git merge --continue
    done
    # Make usually succeeds, but sometimes we need to regenerate things.
    build
    echo "Launching emulator!"
    until (timelimit -p -s 2 -t 15 ./objs/emulator -avd $OPT_AVD -wipe-data)
    do
      read -p "Please fix the runtime and update the commit!"
    done
    COUNTER=0
  fi
done;

# Do the left over
git merge $OPT_TAG
