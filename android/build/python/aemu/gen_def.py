# Lint as: python3
"""Extracts a def file from a statically linked windows library (.LIB)

"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl import app
from absl import flags
from absl import logging
import re
import os
import platform
import subprocess

FLAGS = flags.FLAGS
flags.DEFINE_string('objdump', None,
                    'Path to (llvm) objdump executable.')
flags.DEFINE_string('lib', None,
                    'Library to read for extraction' )
flags.DEFINE_string('defs', None,
                    'Name of the def file to generate' )


"""
The regular expression below is capable of parsing out data obtained by running objdump -t on a .lib file:

[ 21](sec  1)(fl 0x00)(ty  20)(scl   2) (nx 0) 0x0000000000000080 win32GetCurrentDirectory

where the number inside the square brackets is the number of the entry in the symbol table,
the sec number is the section number,
the fl value are the symbol's flag bits,
the ty number is the symbol's type,
the scl number is the symbol's storage class and
the nx value is the number of auxilary entries associated with the symbol.
The last two fields are the symbol's value and its name.
"""

OBJDUMP_PATTERN = re.compile(
r"\[\s*(\d+)\]\(sec\s*(-?\d+)\)\(fl\s*0x([0-9a-fA-F]+)\)\(ty\s+(\d+)\)\(scl\s+(\d+)\)\s+\(nx\s+(\d+)\)\s+0x([0-9a-fA-F]+)\s+(.*)")


class ObjEntry(object):

    def __init__(self, line):
        match = OBJDUMP_PATTERN.match(line)
        self.valid = match is not None
        if match:
            self.nr, self.sec, self.fl, self.ty, self.scl, self.nx, self.val, self.name = match.groups()

    def __str__(self):
        return "[{}](sec {})(fl 0x{})(ty {})(scl {})(nx {}) {} {}".format(self.nr, self.sec, self.fl, self.ty, self.scl, self.nx, self.val, self.name)


class LibAnalyer(object):

    def __init__(self, objdump):
        self.entries = []
        self.obj_dump = objdump
        logging.info("Using %s", self.obj_dump)


    def parse_lib_file(self, fname):
        lines = subprocess.check_output([self.obj_dump, "-t", fname]).splitlines()
        for line in lines:
            entry = ObjEntry(line)
            if entry.valid:
              self.entries.append(entry)


    def create_exports(self, defs):
        with open(defs, 'w') as fw:
            fw.writelines(['LIBRARY\n', 'EXPORTS\n'])
            for e in self.entries:
              logging.info('---- %s', e)
              if int(e.scl) == 2 and not e.name.startswith('__'):
                fw.write(e.name + '\n')


def main(argv):
  if len(argv) > 1:
    raise app.UsageError('Too many command-line arguments.')
  lib = LibAnalyer(FLAGS.objdump)
  lib.parse_lib_file(FLAGS.lib)
  lib.create_exports(FLAGS.defs)


if __name__ == '__main__':
  app.run(main)
