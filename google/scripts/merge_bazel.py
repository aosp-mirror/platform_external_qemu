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
import collections

_T = T.TypeVar("_T")


class OrderedSet(T.MutableSet[_T]):
    # Taken and extended from //external/meson/mesonbuild/utils/universal.py
    # Licensed under the Apache License, Version 2.0 (the "License");
    # you may not use this file except in compliance with the License.
    # You may obtain a copy of the License at
    # Copyright 2012-2020 The Meson development team

    """A set that preserves the order in which items are added, by first
    insertion.
    """

    def __init__(self, iterable: T.Optional[T.Iterable[_T]] = None):
        self.__container: T.OrderedDict[_T, None] = collections.OrderedDict()
        if iterable:
            self.update(iterable)

    def __eq__(self, other: object) -> bool:
        return not any(x not in self for x in other) and not any(
            x not in other for x in self
        )

    def __contains__(self, value: object) -> bool:
        return value in self.__container

    def __iter__(self) -> T.Iterator[_T]:
        return iter(self.__container.keys())

    def __len__(self) -> int:
        return len(self.__container)

    def __repr__(self) -> str:
        # Don't print 'OrderedSet("")' for an empty set.
        if self.__container:
            return "OrderedSet([{}])".format(
                ", ".join(repr(e) for e in self.__container.keys())
            )
        return "OrderedSet()"

    def __reversed__(self) -> T.Iterator[_T]:
        return reversed(self.__container.keys())

    def add(self, value: _T) -> None:
        self.__container[value] = None

    def discard(self, value: _T) -> None:
        if value in self.__container:
            del self.__container[value]

    def move_to_end(self, value: _T, last: bool = True) -> None:
        self.__container.move_to_end(value, last)

    def pop(self, last: bool = True) -> _T:
        item, _ = self.__container.popitem(last)
        return item

    def update(self, iterable: T.Iterable[_T]) -> None:
        for item in iterable:
            self.__container[item] = None

    def intersection(self, set_: T.Iterable[_T]) -> "OrderedSet[_T]":
        return type(self)(e for e in self if e in set_)

    def difference(self, set_: T.Iterable[_T]) -> "OrderedSet[_T]":
        return type(self)(e for e in self if e not in set_)

    def difference_update(self, iterable: T.Iterable[_T]) -> None:
        for item in iterable:
            self.discard(item)

    def indexOf(self, target: T) -> int:
        position = 0
        for item in self:
            if item == target:
                return position
            position += 1
        return -1

    def first(self):
        for item in self:
            return item

        return None


class BazelValue:
    """Class representing a Bazel value that can be merged across different configurations."""

    def __init__(self, configuration: str, value: T.Union[str | T.Set[str]]):
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
        """
        if not (
            isinstance(value, str) or isinstance(value, bool) or isinstance(value, int)
        ):
            self.value = OrderedSet(value)
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
        if isinstance(v, OrderedSet):
            return "[" + ", ".join([f"'{self._escape(x)}'" for x in v]) + "]"

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

    def _compute_intersection(self):
        """This is a bit of a tricky function.
        For windows we *MUST* guarantee that we do not mess up order, lest we get compilation errors.

        So if we have the following:

        x['win'] = [a, b, c, d, e]
        x['lin'] = [a, b, d, e]
        x['mac'] = [a, c, e]

        We need to produce something like this:

        res = [a] + select{win: [b, c], lin: [b, d], mac: [c]} + [e]

        That way if bazel tries to constructh the win target it will construct the
        same array, in the same order as the source build.
        """
        if not isinstance(self.value, OrderedSet):
            return self.value, self.select, None
        shared: OrderedSet = self.value
        for v in self.select.values():
            shared = shared.intersection(v)

        individual = {}
        for k, v in self.select.items():
            individual[k] = v.difference(shared)

        # Ok, now we are going to split shared into before and after
        fst = []
        snd = []
        for share in shared:
            in_fst = True
            for k, v in self.select.items():
                source_idx = v.indexOf(share)
                if individual[k]:
                    dest_idx = v.indexOf(individual[k].first())
                    in_fst = source_idx < dest_idx

            if in_fst:
                fst.append(share)
            else:
                snd.append(share)

        return fst, individual, snd

    def __eq__(self, other):
        return self.value == other.value and self.select == other.select

    def __str__(self):
        v = ""
        fst, individual, snd = self._compute_intersection()
        if self.merge_count > 0:
            # We are not part of a unique rule. i.e. the rule is in many configurations
            if self._same_values():
                # But every configuration has the same value..  (For example, name = '..')
                if len(self.select) == 1:
                    # In this case only one configuration has this value. It might be unique
                    # to my platform. Let's make sure we explicitly select it.
                    v += "select({"
                    for plat, value in individual.items():
                        v += f"'{plat}':  {self._to_str(fst)},"
                    v += "'//conditions:default': [], })"
                else:
                    # We all have the same..
                    if snd:
                        v = self._to_str(fst + snd)
                    else:
                        v = self._to_str(fst)
            else:
                # Set of common values (could be empty)
                v = ""
                if fst:
                    v += self._to_str(fst) + " + "
                v += "select({"
                for plat, value in individual.items():
                    if value:
                        # Only add selection if we have anything..
                        v += f"'{plat}':  {self._to_str(value)},"
                v += "'//conditions:default': [], })"
                if snd:
                    v += f" + {self._to_str(snd)}"
        else:
            # This rule was exclusive to one platform
            if snd:
                v = self._to_str(fst + snd)
            else:
                v = self._to_str(fst)

        return v


class LoadCmd:

    def __init__(self, label: str, rules: T.List[str]):
        self.label = label
        self.rules = rules

    def __str__(self):
        if isinstance(self.rules, str):
            rules = f"'{self.rules}'"
        else:
            rules = ", ".join([f"'{x}'" for x in self.rules])
        return f"load('{self.label}', {rules})"


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
            if k == "imports":
                pass
            self.params[k] = BazelValue(label, v)

    @property
    def name(self):
        return self.params["name"].value

    def __lt__(self, other):
        if self.sort == other.sort:
            return self.name < other.name
        return self.sort < other.sort

    def merge(self, other, unique_keys):
        for k, v in self.params.items():

            if k in unique_keys:
                self.params[k].merge_count = -(2**31)  # Should be enough.
            else:
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
            if k == "includes":
                # Needed as windows relies on the include order
                bazel_cmd += "# buildifier: leave-alone\n"
            if k == "imports":
                pass
            bazel_cmd += f"   {k} = {v},\n"
        return f"{self.sort}({bazel_cmd})"


class BazelRuleLibrary:

    def __init__(self, unique: T.Set[str]):
        self.library: T.Dict[str, BazelRule] = {}
        self.load_cmds = set()
        self.configurations: T.Dict[str, T.Set[str]] = {}
        self.unique_keys = unique

    def register(self, rule: T.Union[BazelRule | LoadCmd]):
        if isinstance(rule, LoadCmd):
            self.load_cmds.add(rule)
            return

        if rule.name in self.library:
            return self.merge_rule(rule)

        self.library[rule.name] = rule

    def merge_rule(self, rule: BazelRule):
        existing = self.library[rule.name]
        existing.merge(rule, self.unique_keys)

    def get(self, name: str) -> BazelRule:
        return self.library[name]

    def is_registered(self, name: str) -> bool:
        return name in self.library

    def serialize(self, stream):
        stream.write("# This build file was autogenerated by:\n")
        stream.write(f"# {' '.join(sys.argv)}\n")

        for load in self.load_cmds:
            stream.write(str(load))
            stream.write("\n")

        stream.write("\n")

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

    def cc_shared_library(self, **kwargs):
        rule = BazelRule(self.configuration, "cc_shared_library", kwargs)
        self.library.register(rule)

    def cc_interface_binary(self, **kwargs):
        rule = BazelRule(self.configuration, "cc_interface_binary", kwargs)
        self.library.register(rule)

    def genrule(self, **kwargs):
        rule = BazelRule(self.configuration, "genrule", kwargs)
        self.library.register(rule)

    def pkg_files(self, **kwargs):
        rule = BazelRule(self.configuration, "pkg_files", kwargs)
        self.library.register(rule)

    def py_binary(self, **kwargs):
        rule = BazelRule(self.configuration, "py_binary", kwargs)
        self.library.register(rule)

    def windows_resources(self, **kwargs):
        rule = BazelRule(self.configuration, "windows_resources", kwargs)
        self.library.register(rule)

    def load(self, bzl, *files):
        rule = LoadCmd(bzl, *files)
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


def serialize(library, stream, verbatim):
    library.serialize(stream)
    for file_path in verbatim:
        with open(file_path, "r", encoding="utf-8") as inp:
            stream.write(inp.read())


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
    parser.add_argument(
        "--verbatim-buildfile",
        action="append",
        metavar="Bazel build file",
        help="Path to a Bazel build file to include verbatim (no merging applied).",
    )

    parser.add_argument(
        "--unique",
        action="append",
        help="Keys that should be treated as unique. For example: win_def_file",
    )
    parser.add_argument("-o", "--output", help="Output file, or stdout if not present")
    args = parser.parse_args()

    library = BazelRuleLibrary(args.unique)
    for file_path, configuration in args.buildfile:
        transform_bazel(file_path, configuration, library)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as stream:
            serialize(library, stream, args.verbatim_buildfile)

    else:
        serialize(library, sys.stdout, args.verbatim_buildfile)


if __name__ == "__main__":
    main()
