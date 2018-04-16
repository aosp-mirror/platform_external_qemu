#!/usr/bin/env python
# Copyright (c) 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A simple wrapper for qt moc tool.
Script for qt_library.gni .
"""

import argparse
import os.path
import subprocess
import sys

def StripHeaderFileExtension(filename):
  if not filename.endswith(".h"):
    raise RuntimeError("Invalid header filename extension: "
                       "{0} .".format(filename))
  return filename.rsplit(".", 1)[0]


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument("--moc",
                      help="Relative path to moc compiler.")

  parser.add_argument("--moc-in-dir",
                      help="Base directory with source header files.")
  parser.add_argument("--cc-out-dir",
                      help="Output directory for generated code.")
  parser.add_argument("--include",
                      help="Optional #include file name to insert into generated code.")

  parser.add_argument("header_files", nargs="+",
                      help="Input header file(s).")

  options = parser.parse_args()

  moc_cmd = [os.path.realpath(options.moc)]

  header_files = options.header_files

  out_dir = options.cc_out_dir

  if options.moc_in_dir:
    moc_in_dir = os.path.relpath(options.moc_in_dir)
    moc_cmd += ["-p", moc_in_dir]

  if options.include:
    moc_cmd += ["-f", options.include]

  for filename in header_files:
    stripped_name = StripHeaderFileExtension(filename)
    cpp_file = os.path.join(cc_out_dir, stripped_name + ".moc.cpp")
    moc_cmd += ["-o", cpp_file]
    moc_cmd += [filename]
    ret = subprocess.call(moc_cmd)
    if ret != 0:
      if ret <= -100:
        # Windows error codes such as 0xC0000005 and 0xC0000409 are much easier to
        # recognize and differentiate in hex. In order to print them as unsigned
        # hex we need to add 4 Gig to them.
        error_number = "0x%08X" % (ret + (1 << 32))
      else:
        error_number = "%d" % ret
      raise RuntimeError("moc has returned non-zero status: "
                         "{0}".format(error_number))


if __name__ == "__main__":
  try:
    main(sys.argv)
  except RuntimeError as e:
    print(e, file=sys.stderr)
    sys.exit(1)
