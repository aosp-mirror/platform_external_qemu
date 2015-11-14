#!/usr/bin/python

from sys import argv

BILLION = 1000000000

INPUT_EVENT_TAG = 'Ta9I'
DISPLAY_EVENT_TAG = 'Ta9D'

def ComputeLatency():
  script, filename = argv
  lines = [line.rstrip('\n') for line in open(filename)]

  inputs = []
  displays = []
  for line in lines:
    if line.startswith(INPUT_EVENT_TAG):
      tag, sec, nsec, type = line.split()
      inputs.append(int(sec) * BILLION + int(nsec))
    elif line.startswith(DISPLAY_EVENT_TAG):
      tag, sec, nsec = line.split()
      displays.append(int(sec) * BILLION + int(nsec))

  print len(inputs), "input events,", len(displays), "display actions"


if __name__ == '__main__':
  ComputeLatency()
