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

from distutils.spawn import find_executable
import os
import subprocess
import sys

import setuptools


try:
    with open("README.md", "r") as fh:
        LONG_DESCRIPTION = fh.read()
except IOError:
    LONG_DESCRIPTION = ""

setuptools.setup(
    python_requires=">=2",
    name="aemu",
    version="1.0",
    test_suite="nose.collector",
    tests_require=["coverage", "mock"],
    author="Erwin Jansen",
    author_email="jansene@google.com",
    description="Set of tools to build the android emulator on every supported platform",
    long_description=LONG_DESCRIPTION,
    long_description_content_type="text/markdown",
    packages=setuptools.find_packages(),
    platforms="POSIX",
    entry_points={
        "console_scripts": ["aemu-build=aemu.cmake:launch"],
    },
    install_requires=[
        "absl-py", "requests", "enum34", "pip"
    ],
    license="Apache License, Version 2.0",
    classifiers=[
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Environment :: Console",
        "Intended Audience :: Developers",
        "Topic :: Utilities",
    ])
