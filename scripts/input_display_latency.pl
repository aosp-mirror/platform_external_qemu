#!/usr/bin/python

from sys import argv

MILLION = 1000000
BILLION = 1000000000

INPUT_EVENT_TAG = 'Ta9I'
DISPLAY_EVENT_TAG = 'Ta9D'

# Qualifying window is 2 Seconds.
# The unit of time is mostly nano second in this program.
QUALIFYING_WINDOW = BILLION * 2


class Event:

  def __init__(self, event_time, event_type):
    self.time = event_time
    self.type = event_type


class Latency:

  def __init__(self, input_time, display_time):
    self.input = input_time
    self.display = display_time


class ResultOfType:

  def __init__(self, input_event_type):
    self.type = input_event_type
    self.latency = []


# Assume the given |list| is already sorted in ascending order.
def FindFirstLargerThan(target, list, start=0):
  index = start
  while (index < len(list)):
    if (list[index] > target):
      return index
    else:
      index = index + 1
  return -1


# A qualified input event is one that there is no other input or display event
# happening in the qualifying window (e.g., 2 seconds) before the event, and
# there is a display event happening in the qualifying window after the event.
def FindQualifiedEvents(results):
  script, filename = argv
  lines = [line.rstrip('\n') for line in open(filename)]

  inputs = []
  displays = []
  for line in lines:
    if line.startswith(INPUT_EVENT_TAG):
      tag, sec, nsec, type = line.split()
      inputs.append(Event(int(sec) * BILLION + int(nsec), type))
    elif line.startswith(DISPLAY_EVENT_TAG):
      tag, sec, nsec = line.split()
      displays.append(int(sec) * BILLION + int(nsec))

  inputs.sort(key=lambda event: event.time)
  displays.sort()
  d = 0
  for i in range(len(inputs)):
    input_time = inputs[i].time
    if (i == 0 or input_time - inputs[i - 1].time > QUALIFYING_WINDOW):
      # No other input events in the past qualifying window.
      d = FindFirstLargerThan(input_time, displays)
      if (d >= 0 and displays[d] - input_time <= QUALIFYING_WINDOW):
        # There is a display event within the qualifying window after the input
        # event.
        if (d == 0 or input_time - displays[d - 1] > QUALIFYING_WINDOW):
          # No other display events in the past qualifying window.
          # Therefore, this is a qualified input event.
          RecordLatency(results, input_time, inputs[i].type, displays[d])


def RecordLatency(results, input_time, input_type, display_time):
  # print 'inupt(ns)  ', input_time, input_type
  # print 'display(ns)', display_time, 'latency(milliseconds)', (
  #     display_time - input_time) / MILLION
  result = 0
  for result_of_a_type in results:
    if result_of_a_type.type == input_type:
      result = result_of_a_type
  if result == 0:
    # A quaified input event type never seen before.
    result = ResultOfType(input_type)
    results.append(result)
  result.latency.append(Latency(input_time, display_time))


# Print results in a format that's easy for a spreadsheet application to read.
def PrintLatency(results):
  max_row = 0
  # Print input types.
  for column in range(len(results)):
    print('input-%s,' % results[column].type),
    if (len(results[column].latency) > max_row):
      max_row = len(results[column].latency)
  print
  # Print latency of each type.
  for row in range(max_row):
    for column in range(len(results)):
      if (row < len(results[column].latency)):
        input = results[column].latency[row].input
        display = results[column].latency[row].display
        # Uncomment the following line for debug
        # print("%d-%d-" % (input,display)),
        print(display - input) / MILLION,
      print ',',
    print


def ComputeLatency():
  results = []
  FindQualifiedEvents(results)
  PrintLatency(results)


if __name__ == '__main__':
  ComputeLatency()
