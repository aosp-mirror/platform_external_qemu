#!/usr/bin/python
"""Sanitizes the output from a run build with the sanitizer.
   This script will attempt to translate addresses to locations.

   Simply pipe the results from a run through this script. For example:

    Create a build with the sanitizer enabled.

   ./android/configure.sh --no-strip --sanitizer=address
   make -j48

   and run it as follows;

   ./objs/emulator @x86_24 2>&1 | ./android/scripts/sanitize.py
"""
import re
import sys
import subprocess

for line in sys.stdin.readlines():
  match = re.search(r'(#[0-9]+) (0x[A-Fa-f0-9]+) \((.*)\+0x(.*)\)$', line.rstrip())
  if match and match.groups() > 2:
    sys.stdout.write("    %s %s %s" % (match.group(1), match.group(2), subprocess.check_output( ["addr2line",  "-e", match.group(3), match.group(2)])))
  else:
    sys.stdout.write(line)
