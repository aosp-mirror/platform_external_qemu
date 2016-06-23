#!/usr/bin/python
# This script sets a path for shared objects/dlls in a cross platform way
# and then executes a command
#
# Usage:
# ld_library_run.py <ld_library_path> <command> [<arg1> [... <argN>]]
import sys
import os
from subprocess import call

# pop script name
sys.argv.pop(0)

# pop shared object path
ld_library_path_value=sys.argv.pop(0)

if sys.platform == 'linux2':
    ld_library_path = 'LD_LIBRARY_PATH'
elif sys.platform == 'darwin':
    ld_library_path = 'DYLD_LIBRARY_PATH'
elif sys.platform == 'win32' or sys.platform == 'cygwin':
    ld_library_path = 'PATH'

original = os.environ.get(ld_library_path)

if original is None:
    os.environ[ld_library_path] = ld_library_path_value
else:
    os.environ[ld_library_path] = ld_library_path_value + os.pathsep + original

call(sys.argv)

# actually not sure if this is necessary
if original is None:
    del os.environ[ld_library_path]
else:
    os.environ[ld_library_path] = original

