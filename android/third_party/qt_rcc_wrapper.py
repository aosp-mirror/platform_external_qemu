#!/usr/bin/env python
# Copyright (c) 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A simple wrapper for qt rcc tool.
Script for qt_library.gni.
"""

import argparse
import ntpath
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
  parser.add_argument("--rcc",
                      help="Relative path to rcc compiler.")
  parser.add_argument("--cpp-out-dir",
                      help="Output directory for generated cpp files.")
 
  parser.add_argument("qrc-files", nargs="+",
                      help="Input qrc file(s).")

  options = parser.parse_args()

  rcc_cmd = [os.path.realpath(options.rcc)]

  rqc_files = options.qrc_files

  out_dir = options.cpp_out_dir

  for filename in header_files:
    stripped_name = StripQrcFileExtension(filename)
    h_file = os.path.join(cc_out_dir, stripped_name + ".cpp")
    rcc_cmd += ["-o", h_file]
    rcc_cmd += ["--name", ntpath.basename(stripped_name)]
    rcc_cmd += [filename]
    ret = subprocess.call(rcc_cmd)
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
