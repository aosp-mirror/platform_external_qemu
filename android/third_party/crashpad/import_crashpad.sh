#!/bin/bash
# Syncs all the crasphad dependencies.
#
# ./import_crashpad.sh /path/to/the/crashpad/sources

function sync_deps {
    rsync -rav  \
      --exclude 'third_party' \
      --exclude 'out' \
      --exclude '.git' \
      --include '*/' \
      --include '*.asm' --include '*.S' --include 'NOTICE' \
      --include 'BUILD.gn' --include 'README*' --include '*.txt' --include '*.dat' \
      --include '*.py' --include '*.defs' --include '*.proctype' \
      --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp'  --include '*.mm' --include '*.m' \
      --exclude '*' \
      $1/crashpad ..

  for dep in cpp-httplib  glibc lss mini_chromium xnu
  do
    rsync -rav  \
      --exclude '.git' \
      --include '*/' \
      --include '*.asm' --include '*.S' --include 'NOTICE' \
      --include 'README*' \
      --include 'BUILD.gn' --include '*.defs' \
      --include '*.h' --include '*.cc' --include '*.c' --include '*.cpp' --include '*.mm' \
      --exclude '*' \
      $1/crashpad/third_party/${dep} third_party/
    done

  # Zlib..
  rsync -rav  \
      --exclude 'zlib/zlib/**' \
      --exclude '.git' \
      --include '*/' \
      --include 'NOTICE'  --include 'README*' \
      --include '*.h' \
      --exclude '*' \
      $1/crashpad/third_party/zlib third_party/
}

sync_deps $1

