# Lint as: python3
"""Extracts a def file from a statically linked windows library (.LIB/,a)"""

from __future__ import absolute_import, division, print_function

import os
import platform
import re
import subprocess

from absl import app, flags, logging

FLAGS = flags.FLAGS
flags.DEFINE_string('objdump', None, 'Path to (llvm) objdump executable.')
flags.DEFINE_string('nm', None, 'Path to (llvm) nm executable.')
flags.DEFINE_list('libs', None, 'Library to read for extraction')
flags.DEFINE_string('name', None, 'Name the libarary')
flags.DEFINE_string('defs', None, 'Name of the def file to generate')

"""The regular expression below is capable of parsing out data obtained by running objdump -t on a .lib file:

[ 21](sec  1)(fl 0x00)(ty  20)(scl   2) (nx 0) 0x0000000000000080
win32GetCurrentDirectory

where the number inside the square brackets is the number of the entry in the symbol table,
the sec number is the section number,
the fl value are the symbol's flag bits,
the ty number is the symbol's type,
the scl number is the symbol's storage class and
the nx value is the number of auxiliary entries associated with the symbol.
The last two fields are the symbol's value and its name.
"""
OBJDUMP_PATTERN = re.compile(
    r'\[\s*(\d+)\]\(sec\s*(-?\d+)\)\(fl\s*0x([0-9a-fA-F]+)\)\(ty\s+(\d+)\)\(scl\s+(\d+)\)\s+\(nx\s+(\d+)\)\s+0x([0-9a-fA-F]+)\s+(.*)'
)

"""
For each symbol, nm shows:
The symbol value, in the radix selected by options (see below), or hexadecimal by default.
The symbol type.
The symbol name.
"""
NM_PATTERN = re.compile(r'([0-9a-fA-F]+)\s+([ABbCDdGgIiNpRrSsTtUuVvWw-])\s+(.*)')

class ObjEntry(object):
  """An entry obtained by running objdump -t."""

  def __init__(self, line):
    match = OBJDUMP_PATTERN.match(str(line))
    self.valid = match is not None
    if match:
      self.nr, self.sec, self.fl, self.ty, self.scl, self.nx, self.val, self.name = match.groups(
      )

  def __str__(self):
    return '[{}](sec {})(fl 0x{})(ty {})(scl {})(nx {}) {} {}'.format(
        self.nr, self.sec, self.fl, self.ty, self.scl, self.nx, self.val,
        self.name)


class NmEntry(object):
  """An entry obtained by running nm."""
  def __init__(self, line):
    match = NM_PATTERN.match(str(line))
    self.valid = match is not None
    if match:
      self.val, self.typ, self.name = match.groups()

  def __str__(self):
    return '{} {} {}'.format(  self.val, self.typ, self.name)



class LibAnalyer(object):
  """An archive analyzer that uses objdump to extract all symbols from an archive."""

  def __init__(self, objdump, nm):
    self.exports = set()
    self.obj_dump = os.path.abspath(objdump)
    self.nm = os.path.abspath(nm)
    logging.info('Using %s - %s', self.obj_dump, self.nm)

  def parse_lib_file(self, lib_filename):
    """Parse the given lib file and add symbols to be exported to the export set.

       Entries are created by parsing the results of calling objdump -t on the lib_filename
    """
    fname = os.path.abspath(lib_filename)
    if not os.path.exists(fname):
      logging.info("Ignoring non existent %s", fname)
      return

    cmd = [self.obj_dump, '-t', fname]
    logging.debug("Running %s", ' '.join(cmd))
    lines = subprocess.check_output(cmd).splitlines()
    for line in lines:
      entry = ObjEntry(line)
      # Only export declarations that are defined here are of interest
      # That means storage class = 2, and actually defined in a section.
      if entry.valid and int(entry.scl) == 2 and int(entry.sec) > 0:
        logging.debug("Exporting symbol: %s", entry)
        self.exports.add(entry.name)
      else:
        logging.debug("Ignoring: %s --  %s", line, entry)


  def create_exports(self, def_file, library_name):
    """Writes a def file, with all the symbols in sorted order."""
    with open(def_file, 'w') as fw:
      fw.writelines(['LIBRARY {}\n'.format(library_name), 'EXPORTS\n'])
      for e in sorted(self.exports):
        fw.write(e + '\n')

def main(argv):
  if len(argv) > 1:
    raise app.UsageError('Too many command-line arguments.')

  lib = LibAnalyer(FLAGS.objdump, FLAGS.nm)
  for library in set(FLAGS.libs):
    lib.parse_lib_file(library)
  lib.create_exports(FLAGS.defs, FLAGS.name)


if __name__ == '__main__':
  app.run(main)
