#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2024 - The Android Open Source Project
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

# This script merges multiple Bazel build files, handling platform-specific
# configurations and resolving conflicts between rules.
# It produces a single, unified build file as output.

import argparse
import sys
import typing as T


class BazelValue:
    """Class representing a Bazel value that can be merged across different configurations."""

    def __init__(
        self, configuration: str, value: T.Union[str | T.Set[str]]
    ):
        """Initialize a BazelValue instance.

        Note that a BazelValue is always part of a BazelRule. If a BazelRule is unique to
        a single condition (for example objc_library is unique to darwin) then all the values
        are unique.

        For example a rule like:

        objc_library(name = 'foo', linkopts=['-framework A]) does not have to be rendered as
        objc_library(name = 'foo', linkopts=select({'@platform//:a' : ['-framework A], ...)

        Whereas a rule that is not unique, but has a property that is unique to a condition
        does need to be selected.

        For example if we merge:
        cc_library(name = 'foo', linkopts=['-framework A'])  # From platform a
        cc_library(name = 'foo')                             # From platform b

        we would like to see:
        cc_library(name = 'foo', linkopts=select({['@platfrom://:a': ['-framework A'], ...)

        Args:
            configuration (str): The configuration associated with the value.
            value (Union[str, Set[str]]): The value itself, either a string or a set of strings.
            unique (bool, optional): Whether the value should be treated as unique (e.g., for objc_library). Defaults to False.
        """
        if not (
            isinstance(value, str) or isinstance(value, bool) or isinstance(value, int)
        ):
            self.value = set(value)
        else:
            self.value = value

        self.label = configuration
        self.select = {configuration: self.value}
        self.merge_count = 0

    def _escape(self, value):
        return value.replace(r"\"", r'\\"')

    def _to_str(self, v):
        if isinstance(v, str):
            return f"'{self._escape(v)}'"
        if isinstance(v, set):
            return "[" + ", ".join(sorted([f"'{self._escape(x)}'" for x in v])) + "]"

        return str(v)

    def _same_values(self):
        """Check if all values across configurations are the same.

        Returns:
            bool: True if all values are the same, False otherwise.
        """
        return not any([x != self.value for x in self.select.values()])

    def merge(self, other, label: str):
        """Merge another BazelValue instance with this one.

        Args:
            other (BazelValue): The other BazelValue instance to merge.
            label (str): The label or configuration associated with the other value.

        Raises:
            ValueError: If attempting to merge strings or bools, which is not supported.
        """
        self.select[label] = other.value
        if other.value == self.value:
            return

        if isinstance(other.value, str) or isinstance(other.value, bool):
            raise ValueError(
                f"Do not know how to merge strings ({self.value}/{other.value})"
            )

        everything = self.value
        for value in self.select.values():
            everything = everything.intersection(value)

        for k, v in self.select.items():
            self.select[k] = v.difference(everything)

        self.value = everything

    def __eq__(self, other):
        return self.value == other.value and self.select == other.select

    def __str__(self):
        v = ""
        if self.merge_count > 0:
            # We are not part of a unique rule. i.e. the rule is in many configurations
            if self._same_values():
                # But every configuration has the same value..  (For example, name = '..')
                if len(self.select) == 1:
                    # In this case only one configuration has this value. It might be unique
                    # to my platform. Let's make sure we explicitly select it.
                    v += "select({"
                    for plat, value in self.select.items():
                        v += f"'{plat}':  {self._to_str(value)},"
                    v += "'//conditions:default': [], })"
                else:
                    # We all have the same..
                    v = self._to_str(self.value)
            else:
                # Set of common values (could be empty)
                v = self._to_str(self.value) + " + "
                v += "select({"
                for plat, value in self.select.items():
                    v += f"'{plat}':  {self._to_str(value)},"
                v += "'//conditions:default': [], })"
        else:
            # This rule was exclusive to one platform
            v = self._to_str(self.value)

        return v


class BazelRule:

    def __init__(self, label: str, sort: str, params):
        """Initialize a BazelRule instance.

        Args:
            label (str): The label or configuration associated with the rule.
            sort (str): The type of the rule (e.g., 'cc_library', 'cc_binary').
            params (Dict[str, Union[str, Set[str]]]): The parameters of the rule.
        """
        self.sort: str = sort
        self.label = label
        self.params: T.Dict[str, BazelValue] = {}
        for k, v in params.items():
            self.params[k] = BazelValue(label, v)

    @property
    def name(self):
        return self.params["name"].value

    def __lt__(self, other):
        if self.sort == other.sort:
            return self.name < other.name
        return self.sort < other.sort

    def copy(self):
        return BazelRule(self.sort, self.params.copy())

    def merge(self, other):
        for v in self.params.values():
            v.merge_count += 1

        for k, v in other.params.items():
            if k not in self.params:
                self.params[k] = v
                continue

            self.params[k].merge(v, other.label)

    def __eq__(self, other):
        return self.name == other.name

    def __str__(self):
        bazel_cmd = "\n"
        for k, v in sorted(self.params.items()):
            bazel_cmd += f"   {k} = {v},\n"
        return f"{self.sort}({bazel_cmd})"


class BazelRuleLibrary:

    def __init__(self):
        self.library: T.Dict[str, BazelRule] = {}
        self.configurations: T.Dict[str, T.Set[str]] = {}

    def register(self, rule: BazelRule):
        if rule.name in self.library:
            return self.merge_rule(rule)

        self.library[rule.name] = rule

    def merge_rule(self, rule: BazelRule):
        existing = self.library[rule.name]
        existing.merge(rule)

    def get(self, name: str) -> BazelRule:
        return self.library[name]

    def is_registered(self, name: str) -> bool:
        return name in self.library

    def serialize(self, stream):
        stream.write("# This build file was autogenerated by:\n")
        stream.write(f"# {' '.join(sys.argv)}\n")

        for target in sorted(self.library.values()):
            stream.write(str(target))
            stream.write("\n")


def GlobalNamespace(obj):
    ret = {}
    for k in dir(obj):
        if not k.startswith("_"):
            ret[k] = getattr(obj, k)
    return ret


class BuildFileFunctions:
    """Provides functions for processing Bazel rules during build file merging.

    This class defines methods corresponding to different Bazel rule types (e.g.,
    `cc_library`, `genrule`). These methods are dynamically called during the
    `exec()` phase to create `BazelRule` objects from the input build files.

    Each method takes keyword arguments representing the rule's parameters and
    creates a `BazelRule` object, which is then registered in the provided
    `BazelRuleLibrary`.
    """

    def __init__(self, library: BazelRuleLibrary, configuration: str):
        self.library: BazelRuleLibrary = library
        self.configuration: str = configuration

    def cc_library(self, **kwargs):
        rule = BazelRule(self.configuration, "cc_library", kwargs)
        self.library.register(rule)

    def cc_binary(self, **kwargs):
        rule = BazelRule(self.configuration, "cc_binary", kwargs)
        self.library.register(rule)

    def objc_library(self, **kwargs):
        rule = BazelRule(self.configuration, "objc_library", kwargs)
        self.library.register(rule)

    def genrule(self, **kwargs):
        rule = BazelRule(self.configuration, "genrule", kwargs)
        self.library.register(rule)

    def py_binary(self, **kwargs):
        rule = BazelRule(self.configuration, "py_binary", kwargs)
        self.library.register(rule)


def transform_bazel(bld_file, configuration, library):
    """Processes a Bazel build file, creating and registering rules.

    This function reads a Bazel build file, executes its content using `exec()`, and
    provides the `BuildFileFunctions` class as the global namespace. This allows the
    build file's code to call the `BuildFileFunctions` methods to create and register
    `BazelRule` objects.

    Args:
        bld_file: The path to the Bazel build file.
        configuration: The current platform configuration.
        library: The `BazelRuleLibrary` where the created rules will be registered.
    """
    exec(
        compile(open(bld_file, "rb").read(), bld_file, "exec"),
        GlobalNamespace(BuildFileFunctions(library, configuration)),
    )


def main():
    parser = argparse.ArgumentParser(
        description="This script merges multiple Bazel build files, "
        " handling platform-specific configurations and resolving "
        "conflicts between rules. It produces a single, "
        "unified build file as output."
    )
    parser.add_argument(
        "--buildfile",
        action="append",
        nargs=2,
        metavar=("Platform", "Bazel build file"),
        help="Platform and path to bazel file",
    )
    args = parser.parse_args()

    library = BazelRuleLibrary()
    for file_path, configuration in args.buildfile:
        transform_bazel(file_path, configuration, library)

    stream = sys.stdout
    library.serialize(stream)


if __name__ == "__main__":
    main()
