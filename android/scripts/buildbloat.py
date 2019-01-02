#!/usr/bin/env python
import json
import os
import sys

"""Converts a ninja build log to text treemap format.

Run `ninja -t recompact` first ot make sure that no duplicate entries are
in the build log, and use a ninja newer than 1.4 to make sure recompaction
removes old, stale buildlog entries.

Usage:
  buildbloat.py out/Release/.ninja_log > data.json
"""

class Node(object):
  def __init__(self, size):
    self.children = {}
    self.size = size

def Insert(data, path, duration):
  """Takes a directory path and a build duration, and inserts nodes for every
  path component into data, adding duration to all directory nodes along the
  path."""
  if '/' not in path:
    if path in data.children:
      Insert(data, os.path.join(path, 'postbuild'), duration)
      return
    assert not path in data.children
    data.children[path] = Node(size=duration)
    data.size += duration
    return

  prefix, path = path.split('/', 1)
  if prefix not in data.children:
    data.children[prefix] = Node(size=0)
  data.size += duration
  Insert(data.children[prefix], path, duration)


def FormatTime(t):
  """Converts a time into a human-readable format."""
  if t < 60:
    return '%.1fs' % t
  if t < 60 * 60:
    return '%dm%.1fs' % (t / 60, t % 60)
  return '%dh%dm%.1fs' % (t / (60 * 60), t % (60 * 60) / 60, t % 60)


def ToDicts(node, name):
  """Converts a Node tree to an object tree usable by webtreemap."""
  d = {
    'id': "'%s' %s" % (name, FormatTime(node.size)),
    'size': node.size
  }
  if node.children:
    d['children'] = [ToDicts(v, k) for k, v in node.children.iteritems()]
  return d


def collectStats(x, level = 0):
    limit = 5
    print "%s%s: %f" % (level * "    ", x['id'], x['size'])
    if x.has_key('children'):
        i = 0
        for c in sorted(x['children'], key = lambda d: d['size'], reverse = True):
            collectStats(c, level = level + 1)
            i += 1
            if i == limit:
                break

def main(args):
  data = Node(size=0)
  times = set()
  did_policy = False

  jsonOutput = 0
  if args.size() >= 2:
    jsonOutput = int(args[1])

  for line in open(args[0], 'r').readlines()[1:]:
    start, finish, _, output, _ = line.split('\t')
    duration = (int(finish) - int(start)) / 1000.0

    # Multiple outputs with exactly the same timestamps were very likely part
    # of a single multi-output edge. Count these only once.
    if (start, finish) in times:
      continue
    times.add((start, finish))

    # Massage output paths a bit.
    if output.startswith('obj/') or output.startswith('gen/'):
      output = output[4:]
    if output.endswith('_unittest.o') or output.endswith('Test.o'):
      output = 'test/' + output
    elif output.endswith('.o'):
      output = 'source/' + output

    Insert(data, output, duration)

  obj = ToDicts(data, 'everything')
  if jsonOutput == 1:
    print json.dumps(obj, indent=2)
  else:
    collectStats(obj)

if __name__ == '__main__':
  main(sys.argv[1:])
