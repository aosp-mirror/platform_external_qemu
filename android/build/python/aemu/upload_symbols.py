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
from itertools import chain
import os
import glob
import time


from absl import app
from absl import flags
from absl import logging

from aemu.definitions import  recursive_iglob

PRODUCT = 'AndroidEmulator'



def upload_symbol(symbol_file, uri):
    logging.info('Checking %s', symbol_file)
    with open(symbol_file, 'r') as f:
        info = f.readline().split()


    if len(info) != 5 or info[0] != 'MODULE':
        logging.error('Corrupt symbol file: %s, %s',
                      symbol_file, info)
        return

    module, os, arch, dbg_id, dbg_file = info
    logging.info("Processing: %s, %s, %s, %s, %s", module, os, arch, dbg_id, dbg_file)

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
    if req.status == 200:
        logging.info("%s -> %s in %s seconds.",
                     symbol_file, uri, time.time() - start)
    else:
        logging.error("Failure: %s -X-> %s, status: %d in %s seconds.",
                      symbol_file, uri, req.status, time.time() - start)


def upload_symbols(out_dir, uri):
    for symbol_file in recursive_iglob(out_dir, [lambda fn: fn.endswith('.sym')]):
        try:
            upload_symbol(symbol_file, uri)
        except urlfetch.UrlfetchException as e:
            logging.error(e)


