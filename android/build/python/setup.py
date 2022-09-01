#!/usr/bin/env python3
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
import setuptools

try:
    with open("README.md", "r") as fh:
        LONG_DESCRIPTION = fh.read()
except IOError:
    LONG_DESCRIPTION = ""

setuptools.setup(
    python_requires=">=2",
    name="emu-build",
    version="1.0",
    test_suite="nose.collector",
    tests_require=["coverage", "mock"],
    author="Erwin Jansen",
    author_email="jansene@google.com",
    description="The android emulator python build scripts",
    long_description=LONG_DESCRIPTION,
    long_description_content_type="text/markdown",
    packages=["aemu"],
    platforms="POSIX",
    entry_points={
        "console_scripts": ["emu-build=aemu:aemu"],
    },
    license="Apache License, Version 2.0",
    classifiers=[
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Natural Language :: English",
        "Environment :: Console",
        "Intended Audience :: Developers",
        "Topic :: Utilities",
    ],
)
