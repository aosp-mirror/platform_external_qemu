#!/usr/bin/python
#
# This script is used to measure the boot time of an Android device or AVD
# in a precision of 0.1 second. It works on OS X, Linux, and Windows.
#
# Usage: Run this script when boot starts. The following is an example of
# how to use this utility.
#
# On OS X, Linux:
#
#   $ ~/bin/boottime.py & emulator -avd Nexus_5_API_23_x86
#
# On Windows:
#
#   start cmd /k python .\boottime.py & emulator.exe -avd Nexus_5_API_23_x86
#
# The script will kill itself if it does not detect any completed boot after
# 10 minutes. The length of waiting is configurable by changing the value of
# TIMEOUT_IN_MINUTES. This could be useful in cases such as the system never
# boots or gets killed before boot completes.
#
# If you run the script in the background (as in the OS X/Linux example above),
# you can stop the script immediately by running "kill $!" .
#
# The script assumes adb is on the path and the implementation depends on the
# output of "adb" command
#
#     adb shell getprop sys.boot_completed
#
# There are online discussion about using "sys.boot_completed" versus
# "init.svc.bootanim" (http://stackoverflow.com/questions/3634041/detect-when-android-emulator-is-fully-booted)
# but the implementation should be sufficient for benchmarking purpose.

import time
import shlex
import subprocess
import sys

TIMEOUT_IN_MINUTES = 10

DETECTION_COMMAND_LINE = 'adb shell getprop sys.boot_completed'

SIGNALS = ['1\r\n',  # On OS X and Linux.
           '1\r\r\n'  # On Windows.
          ]


def TimeBoot():
  start = time.time()
  timeout_in_min = TIMEOUT_IN_MINUTES
  args = shlex.split(DETECTION_COMMAND_LINE)
  while True:
    now = time.time()
    if (now - start > timeout_in_min * 60):
      print 'No completed boot detected for', timeout_in_min, (
          'minutes. boottime.py has quitted automatically.')
      quit()

    proc = subprocess.Popen(args,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    for line in proc.stdout:
      if line in SIGNALS:
        print 'Boot completed. Boot time:', '%.1f' % (now - start), 'seconds.'
        quit()
    time.sleep(0.1)


if __name__ == '__main__':
  TimeBoot()
