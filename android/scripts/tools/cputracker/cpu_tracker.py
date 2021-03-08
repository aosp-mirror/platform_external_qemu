#!/usr/bin/python
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
#
# This script is a lightweight CPU Usage tracker. It will generate a CSV file
# that contains the CPU usage of a process over a specified interval of time.
#
# The script requires python >= 2.7.

import argparse
from contextlib import contextmanager
import psutil
import sys
from time import sleep

# python -m pip install --user psutil

# argparse.FileType has issues on some python installations, thus the need
# for this function.
@contextmanager
def open_with_fallback(filename=None):
    # Open file with a fallback to stdout if filename is invalid.
    if filename and filename != '-':
        fh = open(filename, 'w')
    else:
        fh = sys.stdout

    try:
        yield fh
    finally:
        if fh is not sys.stdout:
            fh.close()

def main():
    parser = argparse.ArgumentParser(description='Track CPU Usage over time of a process.')
    parser.add_argument('--pid', type=int, help='The process ID to track', required=True)
    parser.add_argument('--filename', type=str, help='The output CSV file.')
    parser.add_argument('--duration', type=int, dest='duration',
                        default=10, help='The duration of the recording (seconds)')
    parser.add_argument('--samplerate', type=int, dest='samplerate',
                        default=3, help='Number of samples per second')
    args = parser.parse_args()

    num_samples = args.samplerate * args.duration
    sleep_sec = 1.0 / args.samplerate
    
    proc = psutil.Process(args.pid);
    with open_with_fallback(args.filename) as outfile:
        outfile.write('secs,cpu%\n')
        for i in range(num_samples):
            outfile.write("{},{}\n".format(float(i) / args.samplerate, proc.cpu_percent(interval=0.1)))
            sleep(sleep_sec);

def run_cpu_tracker():
    pass

if __name__ == "__main__":
    main()
