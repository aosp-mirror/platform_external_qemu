#!/usr/bin/env python
# Copyright (c) 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A simple wrapper for qt rcc tool.
Script for qt_library.gni.
"""

from __future__ import print_function
import argparse
import ntpath
from os import environ
import os.path
import subprocess
import sys

def StripQrcFileExtension(filename):
  if not filename.endswith(".qrc"):
    raise RuntimeError("Invalid qrc filename extension: "
                       "{0} .".format(filename))
  return filename.rsplit(".", 1)[0]

# a typical invocation
# LD_LIBRARY_PATH=prebuilts/android-emulator-build/qt/linux-x86_64/lib 
# prebuilts/android-emulator-build/qt/linux-x86_64/bin/rcc 
# -o external/qemu/objs/build/intermediates64/emulator-libui/rcc_static_resources.cpp 
# -name static_resources external/qemu/android/android-emu/android/android-emu/android/skin/qt/static_resources.qrc
def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument("--ldpath",
                      help="Relative path to dynamic Qt libs.")
  parser.add_argument("--rcc",
                      help="Relative path to rcc compiler.")
  parser.add_argument("--qrc-in-dir",
                      help="Base directory for the input resource files.")
  parser.add_argument("--cc-out-dir",
                      help="Output directory for generated cpp files.")
  
  parser.add_argument("--dynamic", action='store_true',
                     help="Output a binary file for use as a dynamic resource.")

  parser.add_argument("qrc_files", nargs="+",
                      help="Input qrc file(s).")

  options = parser.parse_args()

  my_env = os.environ.copy()
  if (options.ldpath):
    my_env["LD_LIBRARY_PATH"] = os.path.realpath(options.ldpath)

  qrc_files = options.qrc_files

  cc_out_dir = options.cc_out_dir

  for filename in qrc_files:
    rcc_cmd = [os.path.realpath(options.rcc)]
    stripped_name = StripQrcFileExtension(filename)
    h_file = os.path.join(cc_out_dir, stripped_name + ".cpp")
    rcc_cmd += ["-o", h_file]
    rcc_cmd += ["--name", ntpath.basename(stripped_name)]
    if (options.dynamic):
      print("dynamic")
      rcc_cmd += ["--binary"]

    if options.qrc_in_dir:
      qrc_file = os.path.join(options.qrc_in_dir, filename)     
    else:
      qrc_file = filename
    rcc_cmd += [qrc_file]
    ret = subprocess.call(rcc_cmd, env=my_env)
    if ret != 0:
      if ret <= -100:
        # Windows error codes such as 0xC0000005 and 0xC0000409 are much easier to
        # recognize and differentiate in hex. In order to print them as unsigned
        # hex we need to add 4 Gig to them.
        error_number = "0x%08X" % (ret + (1 << 32))
      else:
        error_number = "%d" % ret
      raise RuntimeError("rcc has returned non-zero status: "
                         "{0}".format(error_number))


if __name__ == "__main__":
  try:
    main(sys.argv)
  except RuntimeError as e:
    print(e, file=sys.stderr)
    sys.exit(1)
