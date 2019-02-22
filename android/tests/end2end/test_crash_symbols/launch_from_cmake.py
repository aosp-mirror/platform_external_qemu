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
"""Test launcher for cmake that installs all the dependencies that are needed."""

import pip

def install(package):
    if hasattr(pip, 'main'):
        pip.main(['install', '--user', package])
    else:
        pip._internal.main(['install', '--user', package])


def install_and_import(module, package=None):
    import importlib
    try:
        importlib.import_module(module)
    except ImportError:
        if not package:
            package = module
        install(package)
    finally:
        globals()[module] = importlib.import_module(module)


# Absl py should be available.
install_and_import('absl', 'absl-py')
# Needed for the async emulator interaction.
install_and_import('trollius')

import test_crash_symbols
test_crash_symbols.launch()