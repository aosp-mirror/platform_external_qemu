"""A python 3 script to crash the emulator, and verify that the symbols
are available.

It will connect to a running emulator if one is available, otherwise it will
launch one. You must have the 'adb' executable available on the path.

Running against the emulator build in the current branch for example:
You will need:
  - absl.py
  - trollius
  - psutil

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

from emulator_connection import EmulatorConnection
from emulator import Emulator
from symbol_processor import SymbolProcessor

import os
import platform
import sys
import tempfile
import time


try:
    import asyncio
except ImportError:
    import trollius as asyncio
    from trollius import From

FLAGS = flags.FLAGS

flags.DEFINE_string('build', os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(
    __file__)), '..', '..', '..', '..', 'objs')), 'The build directory')
flags.DEFINE_string('avd', None,
                    'The avd to test (or pick first available from emulator)')
flags.DEFINE_string('emulator', None, 'Emulator executable')
flags.DEFINE_integer('maxboot', 120, 'Maximum time to start emulator')
flags.DEFINE_string('stackwalker', None,
                    'Breakpad minidump_stackwalker executable')
flags.DEFINE_string('symbol_src', None, 'Location of the .sym files')

ext = ''
if platform.system().lower() == 'windows':
    ext = '.exe'


@asyncio.coroutine
def crasher(connection):
    """Send the crash command to the emulator.

       This should cause an immediate crash, which will cause
       a disconnect, so the while loop should exit immediately.

       Note: This will NOT WORK with emulators that are not build
       with crash support!!
    """
    while connection.is_connected():
        time.sleep(1)
        yield connection.send("crash")


def get_stackwalker():
    return FLAGS.stackwalker or os.path.join(FLAGS.build, 'minidump_stackwalk%s' % ext)


def get_emulator_exe():
    return FLAGS.emulator or os.path.join(FLAGS.build, 'emulator%s' % ext)


def check_trace(syms, regex):
    trace = syms.trace_has(regex)
    if not trace:
        logging.error("Expected [%s] in minidump", regex)
        sys.exit(1)
    logging.info("Found %s", trace)


def main(argv):
    minidump = tempfile.mktemp('minidump')
    symbol_src = FLAGS.symbol_src or os.path.join(FLAGS.build, 'build')
    emulator = Emulator(get_emulator_exe(), FLAGS.maxboot, FLAGS.avd)
    if not emulator.avd:
        logging.info("No avds, available.. pretending to succeed.")
        return

    emulator.launch()

    # Connection to the emulator
    connection = EmulatorConnection.connect(emulator.console_port)
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(crasher(connection))

    # Now let's analyze the crash report..
    emulator.grab_minidump(minidump)

    # Translate the dump into something human readable.
    syms = SymbolProcessor(symbol_src, get_stackwalker())
    syms.decode_stack_trace(minidump)

    # And validate that we the following entries on the trace.
    # We are expecting the stack trace below in the minidump
    # note that we have translated the function calls.
    EXPECTED_SYMBOLS = [
        r'.*0.*!android::crashreport::CrashReporter::GenerateDumpAndDie\(char const\*\).*',
        r'.*1.*!crashhandler_die.*',
        r'.*2.*!crash\(\).*',
        r'.*3.*!do_crash\(ControlClientRec_\*, char\*\).*',
        r'.*4.*!control_client_do_command\(ControlClientRec_\*\).*',
        r'.*5.*!control_client_read\(void\*, int, unsigned int\).*',
    ]
    for sym in EXPECTED_SYMBOLS:
        check_trace(syms, sym)


def launch():
    app.run(main)


if __name__ == '__main__':
    app.run(main)
