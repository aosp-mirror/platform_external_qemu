"""A python script to crash the emulator, and verify that the symbols
are available.

It will connect to a running emulator if one is available, otherwise it will
launch one. You must have the 'adb' executable available on the path.

Running against the emulator build in the current branch for example:
You will need:
  - absl.py
  - trollius (Python 2 only)

 $ python test_crash_symbols.py --emulator $PWD/../../../objs/emulator
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

try:
    from absl import app
    from absl import flags
    from absl import logging
except ImportError:
    print('Make sure to: pip install absl-py')

from emulator import Emulator
from symbol_processor import SymbolProcessor

import os
import platform
import sys
import tempfile
import time


FLAGS = flags.FLAGS
flags.DEFINE_string('avd', None,
                    'The avd to test (or pick first available from emulator)')
flags.DEFINE_string('avd_root', None,
                    'The ANDROID_SDK_HOME to use, or existing environment var if not set.')
flags.DEFINE_string('sdk_root', None,
                    'The ANDROID_SDK_ROOT to use, or existing environment var if not set.')
flags.DEFINE_string('emulator', None, 'Emulator executable')
flags.DEFINE_integer('maxboot', 120, 'Maximum time to start emulator')
flags.DEFINE_string('stackwalker', None,
                    'Breakpad minidump_stackwalker executable')
flags.DEFINE_string('symbol_src', None, 'Location of the .sym files')
flags.DEFINE_list(
    'args', [], 'Additional parameters to pass to the emulator launcher')
ext = ''
if platform.system().lower() == 'windows':
    ext = '.exe'


def crash_emulator(connection):
    """Send the crash command to the emulator.

       This should cause an immediate crash, which will cause
       a disconnect, so the while loop should exit immediately.

       Note: This will NOT WORK with emulators that are not build
       with crash support!!
    """
    while connection.is_connected():
        time.sleep(1)
        connection.send("crash")


def check_trace(syms, regex):
    trace = syms.trace_has(regex)
    logging.info("%s -> %s", regex, trace)
    return trace

def check_minidump(minidump):
    # Translate the dump into something human readable.
    syms = SymbolProcessor(FLAGS.symbol_src, FLAGS.stackwalker)
    syms.decode_stack_trace(minidump)

    # There are 2 ways in which we can detect a crash.

    # 1. Immediate. Note that our symbols might have mangled C++
    # functions, which can be mangled differently from compiler
    # to compiler.
    IMMEDIATE_CRASH = [
        r'.*0.*!.*GenerateDumpAndDie.*',
        r'.*1.*!.*crashhandler_die.*',
        r'.*2.*!.*crash.*',
        r'.*3.*!.*do_crash.*',
        r'.*4.*!.*control_client_do_command.*',
        r'.*5.*!.*control_client_read.*',
    ]

    # 2. Through our hang detector (happens regularly on windows).
    HANG_DETECTED = [
        r'.*3.*!.*workerThread.*',
        r'.*4.*!.*HangDetector.*',
        r'.*5.*!.*Thread.*']

    if not (all([check_trace(syms, sym) for sym in IMMEDIATE_CRASH]) or
            all([check_trace(syms, sym) for sym in HANG_DETECTED])):
        logging.info(syms.decoded)
        sys.exit(1)


def main(argv):
    minidump = tempfile.mktemp('minidump')

    emulator = Emulator(FLAGS.emulator, FLAGS.maxboot, avd=FLAGS.avd,
                        sdk_root=FLAGS.sdk_root, avd_home=FLAGS.avd_root)

    if not emulator.avd:
        logging.info("No avds available!.")
        sys.exit(1)

    emulator.launch(FLAGS.args)

    # Connection to the emulator
    connection = emulator.connect_console()
    crash_emulator(connection)
    emulator.grab_minidump(minidump)

    # Now let's analyze the crash report..
    if not FLAGS.symbol_src or not FLAGS.stackwalker:
        logging.error("No symbols, or stackwalker defined, did not validate symbols.")
        sys.exit(1)

    check_minidump(minidump)



def launch():
    app.run(main)


if __name__ == '__main__':
    app.run(main)
