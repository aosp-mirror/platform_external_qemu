#!/usr/bin/env python

# This script is used to generate Makefile.qemu2-sources.mk from
# the build output of build-qemu-android.sh. To use it:
#
#  rm -rf /tmp/qemu2-build
#  external/qemu/android/scripts/build-qemu-android.sh \
#      --darwin-ssh=<hostname> \
#      --install-dir=/tmp/qemu2-build
#
#  external/qemu/android-qemu2-glue/scripts/gen-qemu2-sources-mk.py \
#      /tmp/qemu2-build \
#      > external/qemu/android-qemu2-glue/build/Makefile.qemu2-sources.mk
#
from __future__ import print_function
import argparse
import logging
import os
import random
import sys
import itertools
import shutil
import subprocess


class SetLikeList(list):
  """A set like list has set operations defined on a standard list.

  This means that every object only occurs once, and you can do the usual
  set operations on this list.

  Note that this thing is super expensive, and should only be used if
  insertion order is important.
  """

  def add(self, item):
    if item not in self:
      self.append(item)

  def __sub__(self, other):
    for idx in xrange(len(self) - 1, 0, -1):
      if self[idx] in other:
        del self[idx]
    return self

  def __and__(self, other):
    for idx in xrange(len(self) - 1, 0, -1):
      if self[idx] not in other:
        del self[idx]
    return self

  # def __or__(self, other):
  #   for item in other:
  #     self.add(item)


class GitFiles(object):
  IGNORED_OBJECTS = [
      '../audio/sdlaudio.o',
      'gdbstub-xml.o',
      'hax-stub.o',
      'hw/i386/pc_piix.o',

      # these aren't used in the emulator but add 1MB+ to binary size
      '../hw/net/e1000.o',
      '../hw/net/e1000e.o',
      '../hw/net/e1000e_core.o',
      '../hw/net/e1000x_common.o',
      '../ui/sdl_zoom.o',
      '../ui/sdl.o',
      '../ui/sdl2.o',
      '../ui/sdl2-2d.o',
      '../ui/sdl2-input.o',
      '../vl.o',
      '/version.o',  # something from the Windows build
  ]

  def __init__(self, root):
    self.files = set(
        subprocess.check_output(['git', 'ls-files'], cwd=root).split('\n'))

  @staticmethod
  def base_name(fname):
    if fname.startswith('../'):
      fname = fname[3:]
    if '.' in fname:
      return os.path.splitext(fname)[0]
    else:
      return fname

  def is_c_file(self, fname):
    return self.base_name(fname) + '.c' in self.files

  def is_cc_file(self, fname):
    return self.base_name(fname) + '.cc' in self.files

  def ignore(self, fname):
    return fname in self.IGNORED_OBJECTS

  def is_generated(self, fname):
    """Returns true if the fname is generated, ie.

    not found in the qemu root.
    """
    if fname.startswith('../'):
      fname = fname[3:]

    fname = os.path.splitext(fname)[0] + '.c'

    gen_file = os.path.join('qemu2-auto-generated', fname)
    return gen_file in self.files

  @staticmethod
  def is_archive(obj):
    return obj[-2:] == '.a'

  def object_to_file(self, obj):
    if obj.startswith('../'):
      obj = obj[3:]

    obj = os.path.splitext(obj)[0]

    if (self.is_generated(obj)):
      return obj + '.c'

    if (self.is_c_file(obj)):
      return self.base_name(obj) + '.c'
    if (self.is_cc_file(obj)):
      return self.base_name(obj) + '.cc'
    logging.info('Unknown %s', obj)
    return None


TARGET_SUFFIX = '-softmmu'

EXPECTED_HOSTS = set(
    ['linux-x86_64', 'windows-x86', 'windows-x86_64', 'darwin-x86_64'])

INTEREST = [
    'libqemustub',
    'libqemuutil',
    'qemu-system-aarch64',
    'qemu-system-arm',
    'qemu-system-i386',
    # 'qemu-system-mips64el', 'qemu-system-mipsel',
    'qemu-system-x86_64'
]


def read_link_file(link_file, gitfiles):
  """Parses the link file.

  The link file contains all the file included in a lib/exe. We have to look
  at everything between the (rcs for archive) and (-o for executable) and the
  first - which start the link needs.

  Returning (Name, Type, [Files], [GeneratedFiles], [Dependencies])
  """
  files = SetLikeList()
  generated = SetLikeList()
  deps = SetLikeList()
  with open(link_file) as lfile:
    content = lfile.readlines()
    content = [
        x.strip() for x in itertools.dropwhile(
            lambda x: x.strip() != '-o' and x.strip() != 'rcs', content)
    ]
    typ = content[0]
    name = gitfiles.base_name(content[1])
    for obj in content[2:]:
      # Check if we have seen all files.
      if obj[0] == '-':
        break
      if gitfiles.ignore(obj):
        continue

      if gitfiles.is_archive(obj):
        deps.add(gitfiles.base_name(obj))
      elif gitfiles.is_generated(obj):
        generated.add(gitfiles.object_to_file(obj))
      else:
        fname = gitfiles.object_to_file(obj)
        if fname:
          files.add(gitfiles.object_to_file(obj))
        else:
          logging.info('Ignore %s', obj)
  return name, typ, files, generated, deps


def find_link_files(build_path, host, gitfiles):
  discovered = []
  build_path = os.path.join(build_path, host)
  link_prefix = 'LINK-'
  for subdir, _, files in os.walk(build_path):
    for efile in files:
      if efile.startswith(link_prefix):
        (name, tpe, files, generatedfiles, dependencies) = read_link_file(
            os.path.join(subdir, efile), gitfiles)
        if name in INTEREST:
          discovered.append((name, tpe, files, generatedfiles, dependencies))

  return discovered


def bucketize(list_of_files, generated_files):
  buckets = {}
  for name, g in itertools.groupby(list_of_files, os.path.dirname):
    if name in buckets:
      buckets[name] += list(g)
    else:
      buckets[name] = list(g)

  for name, g in itertools.groupby(generated_files, os.path.dirname):
    generated = ['${ANDROID_AUTOGEN}/%s' % x for x in g]
    if name in buckets:
      buckets[name] += generated
    else:
      buckets[name] = generated
  return buckets


def bucket_to_lib_name(name):
  name = os.path.normpath(name).replace('/', '-')
  if name == '.':
    return 'root'
  return name


def to_filename(file_list):
  return [os.path.basename(x) for x in file_list]


def cmake_lib_definition(target, directory, file_list):
  # TODO handle empty file_list
  lib_name = '%s-%s' % (bucket_to_lib_name(directory), target)
  lib = 'add_library(%s %s)' % (lib_name, '\n'.join(file_list))
  deps = 'target_include_directories(%s PRIVATE ${ANDROID_AUTOGEN}/%s \n %s)' % (
      lib_name, directory, directory)
  link = 'target_link_libraries(%s PRIVATE android-qemu-deps)\n' % lib_name
  link += 'target_link_libraries(%s PRIVATE android-%s-deps) \n' % (lib_name,
                                                                    target)
  return '\n'.join([lib, deps, link]) + '\n\n'


def general_cmake_file():
  return 'include(cmake.${ANDROID_TARGET_TAG}.inc)'


def main_cmake(include_dirs):
  subdirs = ['add_subdirectory(%s)' % x for x in include_dirs]
  return '\n'.join(subdirs) + '\n'


def target_exe(target, dependencies):
  cmake = 'add_executable(%s android-qemu2-glue/main.cpp)\n' % target
  cmake += ('target_prebuilt_dependency(%s "${RUNTIME_OS_DEPENDENCIES}" '
            '"${RUNTIME_OS_PROPERTIES}")\n') % target
  cmake += ('target_link_libraries(%s PRIVATE android-%s-deps libqemu2-glue '
            'android-emu\n %s %s)\n\n') % (target, target,
                                           '\n'.join(dependencies),
                                           '\n'.join(dependencies))

  return cmake


def target_lib(target, dependencies):
  cmake = 'add_library(%s dummy.c)\n' % target
  cmake += 'target_link_libraries(%s PRIVATE android-%s-deps\n%s)\n\n' % (
      target, target, '\n'.join(dependencies))
  return cmake


def get_cpu_map(target):
  cpu = target.split('-')[-1]
  droid_map = {
      'aarch64': 'arm',
      'arm': 'arm',
      'i386': 'i386',
      'mips64el': 'mips',
      'mipsel': 'mips',
      'x86_64': 'i386'
  }
  return droid_map[cpu]


def get_target_arch(target):
  cpu = target.split('-')[-1]
  droid_map = {
      'aarch64': 'arm64',
      'arm': 'arm',
      'i386': 'x86',
      'mips64el': 'mips64',
      'mipsel': 'mips',
      'x86_64': 'x86_64'
  }
  return droid_map[cpu]


def link_dependencies(target):
  if 'qemu-system-' not in target:
    return ''
  cpu = get_target_arch(target)
  deps = 'add_library(android-%s-deps INTERFACE)\n' % target
  deps += ('target_include_directories(android-%s-deps INTERFACE '
           '${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/target-%s)\n'
          ) % (target, cpu)
  cpu = get_cpu_map(target)
  deps += ('target_include_directories(android-%s-deps INTERFACE '
           '${ANDROID_QEMU2_TOP_DIR}/target/%s)\n') % (target, cpu)
  deps += ('target_compile_definitions(android-%s-deps INTERFACE '
           '-DNEED_CPU_H )\n' % target)
  return deps


def main(argv):
  parser = argparse.ArgumentParser(description='Generate the sub-Makefiles '
                                   'describing the common and '
                                   'target-specific sources for the QEMU2 '
                                   'build performed with the emulator\'s '
                                   'build system. This is done by looking at '
                                   'the output of a regular QEMU2 build, and '
                                   'finding which files were built and '
                                   'where.')

  parser.add_argument(
      '-i',
      '--input',
      dest='inputs',
      type=str,
      required=True,
      help='The input directory containing all the '
      'build binaries.')
  parser.add_argument(
      '-r',
      '--root',
      dest='root',
      type=str,
      required=True,
      default='.',
      help='The qemu root directory')
  parser.add_argument(
      '-o',
      '--output',
      dest='output',
      help='the output file to write the resulting '
      'makefile to')
  parser.add_argument(
      '-s',
      '--host_set',
      dest='hosts',
      action='append',
      help='Restrict the generated sources only to this host set. '
      'DO NOT USE, ONLY USED FOR MAKING MERGES EASIER"')
  parser.add_argument(
      '-v',
      '--verbose',
      action='store_const',
      dest='loglevel',
      const=logging.INFO,
      help='Be more verbose')

  args = parser.parse_args()

  if not args.hosts:
    args.hosts = EXPECTED_HOSTS

  if args.output is None:
    output = sys.stdout
  else:
    output = open(args.output, 'w')

  logging.basicConfig(level=args.loglevel)

  build_dir = args.inputs
  combined_targets = {}
  target_deps = {}
  include_dirs = SetLikeList()
  for host in args.hosts:
    all_sources = set()
    all_generated = set()
    deps = find_link_files(build_dir, host, GitFiles(args.root))
    cmake = ''
    for (target, typ, files, generated, libdeps) in deps:
      cmake += transform(target, typ, files, generated, libdeps)
      all_sources |= set(files)
      all_generated |= set(generated)

    cmake += source_properties(all_sources, all_generated)
    # Now add to the final cmake
    with open('cmake-main.%s.inc' % host, 'w') as f:
      f.write(cmake)


def source_properties(files, generated_files):
  cmake = ''
  buckets = bucketize(files, generated_files)
  for dir, filelist in buckets.iteritems():
    sources = '${ANDROID_QEMU2_TOP_DIR}/' + ' ${ANDROID_QEMU2_TOP_DIR}/'.join(
        filelist)
    cmake += ('set_source_files_properties(%s PROPERTIES COMPILE_FLAGS " '
              '-I ${ANDROID_QEMU2_TOP_DIR}/${ANDROID_AUTOGEN}/%s")\n') % (sources, dir)
  return cmake


def transform(target, typ, files, generated, libdeps):
  cmake = 'set(%s_sources \n' % target
  cmake += '\n   '.join(files)
  cmake += ')\n'
  cmake += 'set(%s_generated_sources \n' % target
  cmake += '${ANDROID_AUTOGEN}/' + '\n  ${ANDROID_AUTOGEN}/'.join(generated)
  cmake += ')\n'
  cmake += 'set(%s_dep %s)\n\n' % (target, ' '.join(libdeps))
  return cmake


if __name__ == '__main__':
  main(sys.argv)
