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
    NAME=$(echo ${DIR} | sed 's/\//_/g' | sed 's/-/_/g')
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

exmaple usage:

cd external/qemu
scripts/generate_traces.py --out my_output

"""

from __future__ import print_function
import argparse
import fnmatch
import os.path
import subprocess
import sys

def main(argv):

  parser = argparse.ArgumentParser()
  parser.add_argument("--indir",
                      help="Input dir to scan trace-events files")
  parser.add_argument("--out",
                      help="Output dir to store generated files.")

  options = parser.parse_args()
  output_rootdir = options.out

  # make sure current genate_traces.py and tracetool_wrapper.py are in the same folder
  program_dir = os.path.dirname(sys.argv[0])

  # root case

  # generate_trace trace-root.c root c trace-events
  cmd = [os.path.join(program_dir, "tracetool_wrapper.py")]
  gen_file = os.path.join(output_rootdir, "trace-root.c")
  cmd += ["--out", gen_file]
  cmd += ["--group", "root"]
  cmd += ["--format", "c"]
  cmd += ["--tracefile", "trace-events"]
  print("GEN ", gen_file)
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

   # generate_trace trace-root.h root h trace-events
  cmd = [os.path.join(program_dir, "tracetool_wrapper.py")]
  gen_file = os.path.join(output_rootdir, "trace-root.h")
  cmd += ["--out", gen_file]
  cmd += ["--group", "root"]
  cmd += ["--format", "h"]
  cmd += ["--tracefile", "trace-events"]
  print("GEN ", gen_file)
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

  out_dir = os.path.join(output_rootdir, "trace")   
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  
  # generate_trace trace/generated-helpers-wrappers.h root tcg-helper-wrapper-h trace-events
  cmd = [os.path.join(program_dir, "tracetool_wrapper.py")]
  gen_file = os.path.join(out_dir, "generated-helpers-wrappers.h")
  cmd += ["--out", gen_file]
  cmd += ["--group", "root"]
  cmd += ["--format", "tcg-helper-wrapper-h"]
  cmd += ["--tracefile", "trace-events"]
  print("GEN ", gen_file)
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

  # generate_trace trace/generated-helpers.c root tcg-helper-c trace-events
  cmd = [os.path.join(program_dir, "tracetool_wrapper.py")]
  gen_file = os.path.join(out_dir, "generated-helpers.c")
  cmd += ["--out", gen_file]
  cmd += ["--group", "root"]
  cmd += ["--format", "tcg-helper-c"]
  cmd += ["--tracefile", "trace-events"]
  print("GEN ", gen_file)
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

  # generate_trace trace/generated-helpers.h root tcg-helper-h trace-events
  cmd = [os.path.join(program_dir, "tracetool_wrapper.py")]
  gen_file = os.path.join(out_dir, "generated-helpers.h")
  cmd += ["--out", gen_file]
  cmd += ["--group", "root"]
  cmd += ["--format", "tcg-helper-h"]
  cmd += ["--tracefile", "trace-events"]
  print("GEN ", gen_file)
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

  # generate_trace trace/generated-tcg-tracers.h root tcg-h trace-events
  cmd = [os.path.join(program_dir, "tracetool_wrapper.py")]
  gen_file = os.path.join(out_dir, "generated-tcg-tracers.h")
  cmd += ["--out", gen_file]
  cmd += ["--group", "root"]
  cmd += ["--format", "tcg-h"]
  cmd += ["--tracefile", "trace-events"]
  print("GEN ", gen_file)
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

  # process subdirs
  indir = options.indir
  pattern = "trace-events"
  files = []

  # find . -type f -iname 'trace-events'
  # walk through directory
  for dName, sdName, fList in os.walk(indir):
    for fileName in fList:
      if fnmatch.fnmatch(fileName.lower(), pattern.lower()): # Match search string
        files.append(os.path.join(dName, fileName))

  for path in files:
    if path.startswith("./") or path.startswith(".\\"):
        path = path[2:]

    dirname = os.path.dirname(path)
    name = dirname.replace('/', '_')
    name = name.replace('\\', '_')
    name = name.replace('-', '_')

    # speical root cast, already handled above before the loop
    if name == "":
        continue;


    out_dir = os.path.join(output_rootdir, dirname)   
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    h_file = os.path.join(out_dir, "trace.h")
    c_file = os.path.join(out_dir, "trace.c")

    # tracetool_wrapper.py --out trace-root.c --group root --format c --tracefile trace-events
    cmd_h = [os.path.join(program_dir, "tracetool_wrapper.py")]
    cmd_c = [os.path.join(program_dir, "tracetool_wrapper.py")]

    # generate_trace $DIR/trace.h $NAME h $TRACE
    cmd_h += ["--out", h_file]
    cmd_h += ["--group", name]
    cmd_h += ["--format", "h"]
    cmd_h += ["--tracefile", path]
    print("GEN ", h_file)

    # generate_trace $DIR/trace.c $NAME c $TRACE
    cmd_c += ["--out", c_file]
    cmd_c += ["--group", name]
    cmd_c += ["--format", "c"]
    cmd_c += ["--tracefile", path]
    print("GEN ", c_file)

    ret = subprocess.call(cmd_h)
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

    ret = subprocess.call(cmd_c)
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
