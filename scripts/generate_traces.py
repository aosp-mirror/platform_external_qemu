#!/usr/bin/env python
# Copyright (c) 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A simple python script to generate traces

similiar to external/qemu/android-qemu2-glue/build/configure.sh:

LINES=$(find . -type f -iname 'trace-events')
for LINE in $LINES; do
    TRACE=$(echo ${LINE} | sed 's/\.\///')
    DIR=$(dirname $TRACE)
    NAME=$(echo ${DIR} | sed 's/\//_/g')
    if [ "${NAME}" = "." ]; then
        # Special case root
        generate_trace trace-root.c root c trace-events
        generate_trace trace-root.h root h trace-events
        generate_trace trace/generated-helpers-wrappers.h root tcg-helper-wrapper-h trace-events
        generate_trace trace/generated-helpers.c root tcg-helper-c trace-events
        generate_trace trace/generated-helpers.h root tcg-helper-h trace-events
        generate_trace trace/generated-tcg-tracers.h root tcg-h trace-events
    else
        generate_trace $DIR/trace.h $NAME h $TRACE
        generate_trace $DIR/trace.c $NAME c $TRACE
    fi
done

"""

from __future__ import print_function
import argparse
import fnmatch
import os.path
import subprocess
import sys

# find . -type f -iname 'trace-events'
def main(argv):

  parser = argparse.ArgumentParser()
  parser.add_argument("--out",
                      help="Output dir to store generated files.")

  options = parser.parse_args()
  output_rootdir = options.out

  indir = "."
  pattern = "trace-events"
  files = []

  # walk through directory
  for dName, sdName, fList in os.walk(indir):
    for fileName in fList:
      if fnmatch.fnmatch(fileName.lower(), pattern.lower()): # Match search string
        files.append(os.path.join(dName, fileName))

  # make sure current genate_traces.py and tracetool_wrapper.py are in the same folder
  program_dir = os.path.dirname(sys.argv[0])

  for path in files:
    # tracetool_wrapper.py --out trace-root.c --group root --format c --tracefile trace-events
    cmd = os.path.join(program_dir, "tracetool_wrapper.py")

    cpp_file = os.path.join(cc_out_dir, stripped_name + ".cpp")

    dirname = os.path.dirname(path)
    fn = os.path.basename(path)

    out_dir = os.path.join(output_rootdir, dirname)

    c_file = os.path.join(options.moc_in_dir, "trace.c")
    h_file = os.path.join(options.moc_in_dir, "trace.h")

    ret = subprocess.call(cmd)
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
