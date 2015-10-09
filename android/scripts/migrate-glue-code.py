#!/usr/bin/env python

import os
import re

# Set to True to enable debugging.
DEBUG = False

def print_files(list_of_files, name):
    """Print a list of files."""
    print "[ START %s ]" % name
    for a_file in list_of_files:
        print a_file
    print "[   END %s ]" % name

def get_script_dir():
    """Return the current script's directory as an absolute path."""
    return os.path.dirname(os.path.abspath(__file__))

def get_src_top_dir():
    """Return the top-level QEMU1 source path, as an absolute path."""
    script_dir = get_script_dir()
    assert script_dir.endswith("android/scripts")
    return os.path.dirname(os.path.dirname(script_dir))

def is_source_filename(name):
    """Returns true if |name| is a source or header filename."""
    return any(name.endswith(ext) for ext in [".c", ".cpp", ".h", ".m", ".mm"])

def regex_escape(string):
    """Return an escaped version of |string| that can be used as an exact
       match in a regular expression."""
    return string.replace('"', '\\"').replace('.', '\\.')

RE_INCLUDE_BRACKETS = re.compile(r"^\s*#\s*include\s*<(.*)>")
RE_INCLUDE_QUOTES = re.compile(r"^\s*#\s*include\s*\"(.*)\"")

def get_source_includes(filename):
    """Return the list of source files included from |filename|."""
    result = []
    with open(filename, "rb") as lines:
        for line in lines:
            line = line.rstrip('\n')
            match = RE_INCLUDE_BRACKETS.match(line)
            if not match:
                match = RE_INCLUDE_QUOTES.match(line)
            if match:
                result.append(match.group(1))

    return sorted(result)

QEMU_HEADERS = ["elf.h", "qemu-common.h", "trace.h"]
QEMU_SUBDIRS = ["block", "disas", "exec", "fpu", "hw", "migration",
                "monitor", "net", "qapi", "qemu", "qom", "sysemu", "ui"]

def is_qemu_include(filepath):
    """Return True iff |filepath| is a QEMU header file name."""
    if filepath in QEMU_HEADERS:
        return True
    return any(filepath.startswith(x + "/") for x in QEMU_SUBDIRS)

def main():
    """
    A script used to migrate source files that depend on QEMU declarations
    from android/ to android-qemu1-glue/.

    This works by:

    1) Finding all source and header files under android/ that include
    QEMU-specific headers (e.g. "qemu-common.h", "qemu/<something>")

    2) Compute the closure of all other source files that include any of those
    found during 1)

    3) Move all such sources into android-qemu1-glue/, adjusting source and
    build files accordingly.
    """

    src_subdir = "android"
    dst_subdir = "android-qemu1-glue"

    qemu_top_dir = get_src_top_dir()
    android_dir = os.path.join(qemu_top_dir, src_subdir)

    # Step A: Get all source files and their includes from android/
    all_includes = {}

    for dirpath, _, filenames in os.walk(qemu_top_dir):
        for filename in [f for f in filenames if is_source_filename(f)]:
            filepath = os.path.join(dirpath, filename)
            all_includes[filepath] = get_source_includes(filepath)

    android_sources = set([x for x in all_includes.keys() if \
        x.startswith(android_dir)])

    if DEBUG:
        sources = ["%s: %s" % (src, all_includes[src]) for src in \
            sorted(android_sources)]
        print_files(sources, "Android source files")

    # Step B: Determine which source files include QEMU-specific headers.

    qemu_depends = set()
    for filepath in android_sources:
        for include in all_includes[filepath]:
            if is_qemu_include(include):
                qemu_depends.add(filepath)
                break

    if DEBUG:
        print_files(sorted(qemu_depends), "First QEMU depends")

    # Step C: Compute closure of all QEMU depends sources.

    retry = True
    while retry:
        retry = False
        for src in all_includes.keys():
            if src in qemu_depends:
                continue
            #print "? " + src
            for include in get_source_includes(src):
                if not include.startswith(src_subdir):
                    continue
                include_path = os.path.join(qemu_top_dir, include)
                #print "?? %s -> %s" % (include, include_path)
                if include_path in qemu_depends:
                    #print "\n! " + src
                    qemu_depends.add(src)
                    retry = True
                    break

    qemu_depends = sorted(qemu_depends)

    if DEBUG:
        print_files(qemu_depends, "All QEMU depends")

    # Step D: Print commands to move and rename files from src_dir into dst_dir
    renames = {}
    for filepath in qemu_depends:
        if not filepath.startswith(android_dir):
            continue
        src_subpath = filepath[len(android_dir) + 1:]
        src_path = src_subdir + '/' + src_subpath
        dst_path = dst_subdir + '/' + src_subpath
        renames[src_path] = dst_path

    dst_dirs = set()
    for dst_path in renames.values():
        dst_dirs.add(os.path.dirname(dst_path))

    for dst_dir in sorted(dst_dirs):
        print "mkdir -p %s" % (dst_dir)

    for key, value in renames.iteritems():
        print "git mv %s %s" % (key, value)

    for key, value in renames.iteritems():
        print "git grep -l -F \"%s\" | xargs sed -i -e 's|%s|%s|g'" % \
                (key, regex_escape(key), value)

if __name__ == "__main__":
    main()
