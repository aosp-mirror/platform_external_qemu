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


from absl import app
from absl import flags
from absl import logging

from definitions import SymbolUris

PRODUCT = 'AndroidEmulator'

FLAGS = flags.FLAGS

flags.DEFINE_enum('symbol_dest', 'production', SymbolUris.values(),
                  'Target we want to build, if any')


def post(uri, product, dbg_file, dbg_id, symbol_file):
    req = urlfetch.post(
        uri,
        data={
            'product': PRODUCT,
            'codeIdentifier': dbg_d,
            'debugIdentifier': dbg_id,
        },
        files={
            'codeFile': open(dbg_file, 'rb'),
            'symbolFile': open(symbol_file, 'rb'),
            'debugFile': open(dbg_file, 'rb')
        }
    )
    logging.info("Posted %s to %s, status: %d", symbol_file, url, req.status)


def upload_symbols(symbol_file):
    uri = SymbolUris.from_string(FLAGS.symbol_dest).value
    logging.info("Upload %s -> %s", symbol_file, uri)



def main(argv=None):
    del argv # Unused
    result = (chain.from_iterable(
        glob.glob(os.path.join(x[0], '*.sym')) for x in os.walk('.')))
    map(upload_symbols, result)




if __name__ == '__main__':
    app.run(main)
