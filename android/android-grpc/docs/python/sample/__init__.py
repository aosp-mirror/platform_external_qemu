import sys
import os

# Workaround for protobuf paths not working well on Py3.
if sys.version_info[0] == 3:
    sys.path.insert(
            0, os.path.abspath(os.path.join(os.path.dirname(__file__), "proto"))
        )
    sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))