#!/bin/bash -e
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This tool is used to update libvpx source code to a revision of the upstream
# repository. Modified from Chromium src/third_party/libvpx/update_libvpx.sh

# Usage:
#
# $ ./update_libvpx.sh [branch | revision | file or url containing a revision]
# When specifying a branch it may be necessary to prefix with origin/

# Tools required for running this tool:
#
# 1. Linux / Mac
# 2. git

export LC_ALL=C

# Location for the remote git repository.
GIT_REPO="https://chromium.googlesource.com/webm/libvpx"

# Update to TOT by default.
GIT_BRANCH="origin/master"

# Relative path of target checkout.
LIBVPX_SRC_DIR="libvpx"

BASE_DIR=`pwd`

if [ -n "$1" ]; then
  GIT_BRANCH="$1"
  if [ -f "$1"  ]; then
    GIT_BRANCH=$(<"$1")
  elif [[ $1 = http* ]]; then
    GIT_BRANCH=`curl $1`
  fi
fi

prev_hash="$(egrep "^Commit: [[:alnum:]]" README.android | awk '{ print $2 }')"
echo "prev_hash:$prev_hash"

rm -rf $LIBVPX_SRC_DIR
mkdir $LIBVPX_SRC_DIR
cd $LIBVPX_SRC_DIR

# Start a local git repo.
git clone $GIT_REPO .

# Switch the content to the desired revision.
git checkout -b tot $GIT_BRANCH

add="$(git diff-index --diff-filter=A $prev_hash | \
tr -s [:blank:] ' ' | cut -f6 -d\ )"
delete="$(git diff-index --diff-filter=D $prev_hash | \
tr -s [:blank:] ' ' | cut -f6 -d\ )"

# Get the current commit hash.
hash=$(git log -1 --format="%H")

# README reminder.
echo "Update README.android:"
echo "==============="
echo "Date: $(date +"%A %B %d %Y")"
echo "Branch: $GIT_BRANCH"
echo "Commit: $hash"
echo "==============="
echo ""

# Commit message header.
echo "Commit message:"
echo "==============="
echo "libvpx: Pull from upstream"
echo ""

# Output the current commit hash.
echo "Current HEAD: $hash"
echo ""

# Output log for upstream from current hash.
if [ -n "$prev_hash" ]; then
  echo "git log from upstream:"
  pretty_git_log="$(git log \
                    --no-merges \
                    --topo-order \
                    --pretty="%h %s" \
                    --max-count=20 \
                    $prev_hash..$hash)"
  if [ -z "$pretty_git_log" ]; then
    echo "No log found. Checking for reverts."
    pretty_git_log="$(git log \
                      --no-merges \
                      --topo-order \
                      --pretty="%h %s" \
                      --max-count=20 \
                      $hash..$prev_hash)"
  fi
  echo "$pretty_git_log"
  # If it makes it to 20 then it's probably skipping even more.
  if [ `echo "$pretty_git_log" | wc -l` -eq 20 ]; then
    echo "<...>"
  fi
fi

# Commit message footer.
echo ""
echo "==============="

# Git is useless now, remove the local git repo.
rm -rf .git .gitignore .gitattributes

# Add and remove files.
echo "$add" | xargs -I {} git add {}
echo "$delete" | xargs -I {} git rm --ignore-unmatch {}

# Find empty directories and remove them.
find . -type d -empty -exec git rm {} \;

chmod 755 build/make/*.sh build/make/*.pl configure

cd $BASE_DIR
