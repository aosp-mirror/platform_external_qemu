#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys
import urlfetch
import time

PRODUCT = 'AndroidEmulator'


def upload_symbol(symbol_file, uri):
    with open(symbol_file, 'r') as f:
        info = f.readline().split()

    if len(info) != 5 or info[0] != 'MODULE':
        print('Corrupt symbol file: %s, %s',
              symbol_file, info)
        return 1

    module, os, arch, dbg_id, dbg_file = info

    start = time.time()
    req = urlfetch.post(
        uri,
        data={
            'product': PRODUCT,
            'codeIdentifier': dbg_id,
            'debugIdentifier': dbg_id,
            'codeFile': dbg_file,
            'debugFile': dbg_file,
        },
        files={
            'symbolFile': open(symbol_file, 'rb')
        }
    )
    if req.status != 200:
        print("Failure: %s -X-> %s, status: %d in %s seconds." % (
              symbol_file, uri, req.status, time.time() - start))
        return 1
    print("%s -> %s in %s seconds." % (
          symbol_file, uri, time.time() - start))
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage %s [symbol] [uri]" % sys.argv[0])
        sys.exit(1)
    else:
        sys.exit(upload_symbol(sys.argv[1], sys.argv[2]))
