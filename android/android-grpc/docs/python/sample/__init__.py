import os
import sys
import six

# Python3 importing is not compatible with protobuf.
if six.PY3:
    sys.path.insert(
        0, os.path.abspath(os.path.join(os.path.dirname(__file__), "proto"))
    )
    sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))
