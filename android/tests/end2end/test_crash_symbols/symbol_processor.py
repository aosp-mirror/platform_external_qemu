#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""A breakpad symbol processor."""
import os
import re
import shutil
import subprocess
import tempfile

try:
    from absl import logging
except ImportError:
    import logging


class SymbolProcessor(object):

    def __init__(self, src_dir, stackwalk_exe, dest=None):
        self.symbol_dir = tempfile.mkdtemp("symbol_proc")
        self.stackwalk = stackwalk_exe
        self.relocate_symfiles(src_dir, self.symbol_dir)
        self.decoded = None

    def decode_stack_trace(self, dmpfile):
        """Decodes a minidump file by invoking the stackwalker."""
        with open(os.devnull, 'w') as devnull:
            self.decoded = subprocess.check_output([self.stackwalk, dmpfile, self.symbol_dir],
                                                   stderr=devnull)
        return self.decoded

    def relocate_symfiles(self, src_dir, dest):
        """Relocates all the .sym files to a location that can be used by minidump_stackwalker."""
        for sym_file in self.recursive_iglob(src_dir, '.sym'):
            self.move_symbol_file(sym_file, dest)

    def move_symbol_file(self, symbol_file, symbol_root):
        """Moves a single symbol file to proper place in the symbol directory."""
        _, _, _, dbg_id, dbg_file = self.extract_symbol_info(symbol_file)
        dest_path = os.path.join(symbol_root, dbg_file, dbg_id)
        if not os.path.exists(dest_path):
            os.makedirs(dest_path)
        final_dst = os.path.join(dest_path, '%s.sym' % dbg_file)
        logging.info("Copying %s -> %s", symbol_file, final_dst)
        shutil.copy2(symbol_file, final_dst)

    def extract_symbol_info(self, symbol_file):
        """Parses the symbol info (first line of a .sym file).

        Returns:
            module, os, arch, dbg_id, dbg_file
        """
        with open(symbol_file, 'r') as f:
            info = f.readline().split()

        if len(info) != 5 or info[0] != 'MODULE':
            raise('Corrupt symbol file: %s, %s' % (symbol_file, info))

        return info

    def trace_has(self, regex):
        """Returns true if the given regex occurs in the decoded stack trace."""
        return re.compile(regex, re.M).findall(self.decoded)

    def recursive_iglob(self, rootdir, suffix):
        """Recursively glob the rootdir for any file that matches the given suffix."""
        for root, _, filenames in os.walk(rootdir):
            for filename in filenames:
                fname = os.path.join(root, filename)
                if fname.endswith(suffix):
                    yield fname
