# -*- coding: utf-8 -*-
#!/usr/bin/env python3
#
# Copyright 2023 - The Android Open Source Project
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
# limitations under the License.

import argparse
import logging
from pathlib import Path
import sys
from typing import Dict, List, Set


from aemu.log import configure_logging
import pydot


class CustomFormatter(
    argparse.RawTextHelpFormatter, argparse.ArgumentDefaultsHelpFormatter
):
  pass


from typing import Dict, Set, List


def dependency_tree(
    graph: Dict[str, List[str]], seen: Set[str], target: str, prefix: str = ""
):
  """Generate a dependency tree representation for a given target in a dependency graph.

  Args:

  - graph (Dict[str, List[str]]): A dictionary representing the dependency
    graph.
  - seen (Set[str]): A set to keep track of seen targets to avoid cyclic
    dependencies.
  - target (str): The current target for which to generate the tree.
  - prefix (str, optional): The prefix string for indentation, used for
    recursive calls.

  Yields:
  - str: Lines representing the dependency tree for the given target.
  """
  # Prefix components:
  space = "    "
  branch = "│   "
  # Pointers:
  tee = "├── "
  last = "└── "

  seen.add(target)
  contents = [x for x in graph.get(target, []) if x not in seen]

  # Each content gets pointers that are '├──' with a final '└──':
  pointers = [tee] * (len(contents) - 1) + [last]

  # The last target will have a '└──'
  for pointer, next_target in zip(pointers, contents):
    # this will result in a line like f'    ├── {next_target}'
    yield prefix + pointer + next_target

    # Check if the next_target has dependencies:
    if graph.get(next_target):
      # Two cases, we are the last entry or not.
      # if we are not the last we need to draw the line to us.
      extension = branch if pointer == tee else space
      yield from dependency_tree(
          graph, seen, next_target, prefix=prefix + extension
      )


def print_dependencies(graph, target: str):
  print(target)
  for line in dependency_tree(graph, set(), target):
    print(line)


def main():
  parser = argparse.ArgumentParser(
      formatter_class=CustomFormatter,
      description="""

        """,
  )
  parser.add_argument(
      "--target",
      type=str,
      help="Source dot file to process",
  )
  parser.add_argument(
      "src",
      type=str,
      help="Source dot file to process",
  )
  parser.add_argument(
      "-v",
      "--verbose",
      dest="verbose",
      default=False,
      action="store_true",
      help="Verbose logging",
  )

  args = parser.parse_args()

  lvl = logging.DEBUG if args.verbose else logging.INFO
  configure_logging(lvl)
  print("Please be patient, parsing a depfile is very slow...")
  graph = pydot.graph_from_dot_file(args.src)
  result = {}

  for g in graph:
    for node in g.get_node_list():
      if node.get("label") != None:
        result[node.get("label")] = []

    for edge in g.get_edges():
      result[g.get_node(edge.get_source())[0].get("label")].append(
          g.get_node(edge.get_destination())[0].get("label")
      )

  print_dependencies(result, f'"{args.target}"')


if __name__ == "__main__":
  main()
