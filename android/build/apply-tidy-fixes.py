#!/usr/bin/env python
"""
Applies a set of fixes obtained by running clang-tidy.
"""

from __future__ import print_function

import argparse
import fnmatch
import itertools
import logging
import glob
import math
import os
import re
import sys
import traceback
import yaml
from functools import partial
from multiprocessing.dummy import Pool as ThreadPool
try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader


def collapse(group, mergef, tuples):
  """Groups a list of tuples and merges the values into a dictionary.

  the merge function will be invoked if the first element in the tuple is
  the same. For example:

  [('a', 1), ('a', 2)] will result in the dictionary:
  { 'a' : mergef(1, 2) }

  Args:
    group: The function used to group the keys by
    mergef: The function used to merge values.
    tuples: The list of tuples to be merged.

  Returns:
    A dictionary of all the tuples with collisions merged.
  """
  res = {}
  for k, v in tuples:
    dest = group(k)
    if dest in res:
      res[dest] = mergef(res[dest], v)
    else:
      res[dest] = v
  return res


def merge_dict(mergef, left, right):
  """Merges the left dictionary and the right dictionary.

     When the same key is encountered the mergef function will
     be invoked with the left and right value.

  Args:
    mergef: Function used to resolve conflicts (both key in left/right)
    left: Left dictionary to be merged
    right: Right dictionary to be merged

  Returns:
    A new dictionary [left + right] with resolved keys.
  """
  return collapse(lambda x: x, mergef,
                  itertools.chain(left.iteritems(), right.iteritems()))


def pool_merge(yaml_files, accept, reject, count = 8):
  """Merges a set of yaml_files using a thread pool

  Args:
    yaml_files: The list of files that should be merged
    accept: Compiled regex of paths we should apply
    reject: Compiled regex of paths we should ignore
    count: Size of the threadpool to use.

  Returns:
    A new containing all the replacement operations
  """
  pool = ThreadPool(count)
  replace = partial(extract_replacements, accept=accept, reject=reject)
  extract = pool.map(replace, yaml_files)
  pool.close()
  pool.join()

  logging.info("Final merge..")
  patchset = {}
  for r in extract:
    patchset = merge_dict(lambda x,y: x + y, patchset, r)
  return patchset

def find_yaml_files(root_dir):
  """Finds all the yaml files under the given directory.

  Args:
    root_dir: The root directory to start the search from

  Returns:
    All the yaml files underneath the root directory.
  """
  # Find all the yaml files.
  matches = []
  for root, _, filenames in os.walk(root_dir):
    for filename in fnmatch.filter(filenames, '*.yaml'):
      matches.append(os.path.join(root, filename))

  return matches


def extract_replacements(yaml_file, accept=None, reject=None):
  """Extracts all the replacements described in the yaml file.

  Args:
    yaml_file: The generated clang-tidy file containing all
               the diagnostics.
    accept: compiled regex with acceptable paths.
    reject: compiled regex with paths we decline.

  Returns:
    All map from file -> [replacements].
  """
  files = {}
  content = yaml.load(open(yaml_file, 'r'), Loader=Loader)

  # Only merge if we have something
  if not content:
    return files
  logging.info("Processing %s", yaml_file)

  # Extract the replacements.
  for diagnostic in content.get('Diagnostics',[]):
    if diagnostic['Replacements']:
      for replacement in diagnostic['Replacements']:
        path = replacement['FilePath']
        if ((accept and accept.match(path)) and not
            (reject and reject.match(path))):
          if path not in files:
            files[path] = []
          files[path].append(replacement)
  return files



def apply_fixes(file_fixes, accept, reject):
  """Applies all the fixes to the given source files.

  Args:
    file_fixes: The map of file->fixes
    accept: Compiled regex of paths we should apply
    reject: Compiled regex of paths we should ignore
  """
  # Now we apply them
  for path in file_fixes.keys():
    if not os.path.exists(path) or reject.match(path) or not accept.match(path):
      logging.info("Ignoring: %s", path)
      continue

    # Application of fixes should happen in reverse offset order
    fixes = sorted(file_fixes[path],
                   key=lambda repl: repl['Offset'],
                   reverse=True)
    source = None
    with open(path, 'r') as cpp:
      logging.debug("Fixing up %s", path)
      source = cpp.read()
      offset = -1
      for fix in fixes:
        # During merge we can have the same offset mentioned multiple times..
        # we don't want to apply the same fix multiple times.
        if offset == fix['Offset']:
          continue
        logging.debug("Applying fix to %s at [%d:%d]",
                      fix['ReplacementText'],
                      offset,
                      fix['Length'])
        offset = fix['Offset']
        source = (source[:offset] +
                  fix['ReplacementText'] +
                  source[offset + fix['Length']:])

    if source:
      with open(path, 'w') as cpp:
        cpp.write(source)
        logging.info("Fixed: %s" % path)


def main():
  parser = argparse.ArgumentParser(description='Applies all the fixes '
                                   'found by clang-tidy.')
  parser.add_argument('-p', dest='build_path', default=None,
                      help='Path used to find the clang-tidy .yaml files. '
                      'All yaml files on the path will be merged and fixes will'
                      ' be applied')
  parser.add_argument('-i', dest='file', default=None,
                      help='clang-tidy .yaml file to apply.')
  parser.add_argument('--accept', dest='accept', default='.*',
                      help='Regex of files for which changes should '
                      'be applied.')
  parser.add_argument('--reject', dest='reject', default='$^',
                      help='Regex of files for which changes should '
                      'not be applied.')
  parser.add_argument('-vv', '--veryverbose', action="store_const",
                      dest="loglevel",
                      const=logging.DEBUG,
                      default=logging.ERROR,
                      help="Print lots of debugging statements")
  parser.add_argument('-v', '--verbose',
                      action="store_const", dest="loglevel", const=logging.INFO,
                      help="Be more verbose")

  args = parser.parse_args()
  if not (args.build_path or args.file):
    parser.print_help()
    sys.exit(1)

  logging.basicConfig(level=args.loglevel)

  return_code = 0
  accept = re.compile(args.accept)
  reject = re.compile(args.reject)

  try:
    files = [args.file]
    if (args.build_path):
      files = find_yaml_files(args.build_path)

    content = pool_merge(files, accept, reject)
    apply_fixes(content, accept, reject)
  except:
    return_code = 1
    print('Error applying fixes.\n', file=sys.stderr)
    traceback.print_exc()

  sys.exit(return_code)

if __name__ == '__main__':
  main()
