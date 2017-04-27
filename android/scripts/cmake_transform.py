#!/bin/python
'''A script to transform a json representation of our build dependencies into a
   set of cmake files'''
import fnmatch
import uuid
import argparse
import json
import logging
import os
import sys
import subprocess

def cleanup_json(data):
  '''Cleans up the json structure by removing empty "", and empty key value
    pairs'''
  if (isinstance(data, unicode)):
    copy = data.strip()
    return None if len(copy) == 0 else copy
  if (isinstance(data, dict)):
    copy = {}
    for key,value in data.iteritems():
      rem = cleanup_json(value)
      if (rem is not None):
        copy[key] = rem
    return None if len(copy) == 0 else copy

  if (isinstance(data, list)):
    copy = []
    for elem in data:
      rem = cleanup_json(elem)
      if (rem is not None and not rem in copy):
        copy.append(rem)

    if len(copy) == 0:
      return None
    if len(copy) == 1:
      return copy[0]
    return copy

class Module:

  def __init__(self, json_hash, json_modules, basedir):
    self.module   = json_hash
    self.modules  = json_modules
    self.basedir  = basedir
    self.src_set  = []
    self.includes = [self.module['LOCAL_PATH']]
    self.dependencies = []
    if 'LOCAL_C_INCLUDES' in self.module:
      self.includes += self.get_property_as_list('LOCAL_C_INCLUDES')
    if 'LOCAL_CFLAGS' in self.module:
      lst = [x for x in self.get_property_as_list('LOCAL_CFLAGS') if not x.startswith('-I')]
      self.module['LOCAL_CFLAGS'] = lst

  def is_prebuilt(self):
    lst = self.get_property_as_list('LOCAL_SRC_FILES')
    return len(lst) == 1 and lst[0].endswith('.a')

  def is_leaf(self):
    return not 'LOCAL_STATIC_LIBRARIES' in self.module

  def get_type(self):
    if 'LOCAL_MODULE_CLASS' not in self.module:
      return None

    typ = self.module['LOCAL_MODULE_CLASS']
    if (typ == 'STATIC_LIBRARIES' or typ == 'STATIC_LIBRARY'):
      return 'ARCHIVE'
    if (typ == 'SHARED_LIBRARY' or typ == 'SHARED_LIBRARIES'):
      return 'LIBRARY'
    if (typ.startswith('EXECUTABLE')):
      return 'RUNTIME'

  def get_target_type(self):
    target = '\n ## Module output ##\n'
    if self.get_type() is None:
      return target

    name = self.get_module_name()
    if self.is_prebuilt():
      src = self.module['LOCAL_SRC_FILES']
      target += 'add_library({} STATIC IMPORTED GLOBAL)\n'.format(name)
      target += 'set_property(TARGET {} PROPERTY IMPORTED_LOCATION '.format(name)
      target += '"{}")\n'.format(src)
      return target


    source_set = ' '.join(
        ['${{{}}}'.format(src) for src in self.src_set]
    )
    typ = self.get_type()

    if (typ == 'ARCHIVE'):
        target += 'add_library({} {})\n'.format(name, source_set)
    if (typ == 'LIBRARY'):
        target += 'add_library({} SHARED {})\n'.format(name, source_set)
    if (typ == 'RUNTIME'):
        target += 'add_executable({} {})\n'.format(name, source_set)
    return target

  def get_include_directories(self):
    includes = [self.get_relative_to_location(x) for x in self.includes]

    target = '\n ## Includes ##\n'
    target += 'include_directories({})\n'.format('\n  '.join(includes))
    return target

  def get_relative_to_location(self, x):
    return os.path.relpath(os.path.abspath(x), self.get_module_location())

  def get_module_name(self):
    return self.module['LOCAL_MODULE']

  def get_module_location(self):
    rel = os.path.relpath(
        os.path.abspath(self.module['LOCAL_PATH']),
        get_root())
    return os.path.abspath(self.basedir + '/' +
                           rel + '/' +
                           self.get_module_name())



  def get_dummy_src(self):
    name = self.get_module_name()
    target = 'file(WRITE empty_dummy.cpp "")\n'
    target += 'set (SOURCES_{}_dummy empty_dummy.cpp)\n'.format(name)
    self.src_set.append('SOURCES_{}_dummy'.format(name))
    return target

  def get_src_list_tag(self, tag):
    name = self.get_module_name()
    location = '{}/{}/'.format(get_root(), self.module['LOCAL_PATH'])
    target = ''
    if tag in self.module:
      src = [x if os.path.isfile(x) else location + x for x in self.get_property_as_list(tag)]
      src = [self.get_relative_to_location(x) for x in src]
      self.src_set.append('SOURCES_{}_{}'.format(name, tag))
      target += 'set (SOURCES_{}_{} {})\n'.format(name, tag,
                                                  '\n  '.join(src))
    return target

  def get_src_list(self):
    target = '\n ## Sources ##\n'
    src = ['LOCAL_SRC_FILES', 'LOCAL_QT_RESOURCES', 'LOCAL_QT_MOC_SRC_FILES']
    target += '\n'.join([self.get_src_list_tag(x) for x in src])

    if len(self.src_set) == 0:
      target += self.get_dummy_src()

    return target

  def get_custom_cmd(self, cmd, gen):
    name = self.get_module_name()
    target  = 'add_custom_command(OUTPUT {} \n'.format(' '.join(gen))
    target += 'COMMAND {})\n'.format(cmd)

    src_name = uuid.uuid4().hex
    src_list = ' '.join(['${CMAKE_CURRENT_BINARY_DIR}/' + x for x in gen]) 
    target += 'set(GEN_{0}_{1} "{2}")\n'.format(name, src_name, src_list)
    self.src_set.append('GEN_{}_{}'.format(name, src_name))
    return target

  def get_gen_qapi(self, typ, cmd):
    return self.get_custom_cmd('python {0}/scripts/{1} -o'
                               ' ${{CMAKE_CURRENT_BINARY_DIR}}'
                               ' {0}/qapi-schema.json'.format(get_root(), cmd),
			       [typ + '.c'])

  def generate_src(self, src):
    if 'qapi-event' in src:
      return self.get_gen_qapi('qapi-event', 'qapi-event.py')
    if 'qapi-types' in src:
      return self.get_gen_qapi('qapi-types', 'qapi-types.py --builtins')
    if 'qapi-visit' in src:
      return self.get_gen_qapi('qapi-visit', 'qapi-visit.py --builtins')
    if 'qmp-introspect' in src:
      return self.get_gen_qapi('qmp-introspect', 'qapi-introspect.py')
    if 'qmp-marshal' in src:
      return self.get_gen_qapi('qmp-marshal', 'qapi-commands.py')
    if 'generated-tracers' in src:
      matches = []
      for root, dirnames, filenames in os.walk(get_root()):
        for filename in fnmatch.filter(filenames, 'trace-events'):
          matches.append(os.path.join(root, filename))

      return self.get_custom_cmd('python {0}/scripts/tracetool.py '
                                 '--backends=nop --format=c --target-type '
                                 'system "{1}" > ${{CMAKE_CURRENT_BINARY_DIR}}/'
                                 'generated-tracers.c'.format(
                                     get_root(),
                                     [matches[0]]),
                                 'generated-tracers.c')
      if 'gdbstub-xml-arm64.c' in src:
        return self.get_custom_cmd('bash {0}/scripts/feature_to_c.sh ${{CMAKE_CURRENT_BINARY_DIR}}/gdbstub-xml-arm64.c '
                                   ' {0}/gdb-xml/aarch64-core.xml'
                                   ' {0}/gdb-xml/aarch64-fpu.xml'
                                   ' {0}/gdb-xml/arm-core.xml'
                                   ' {0}/gdb-xml/arm-vfp.xml'
                                   ' {0}/gdb-xml/arm-vfp3.xml'
                                   ' {0}/gdb-xml/arm-neon.xml'.format(get_root()),
                                   ['gdbstub-xml-arm64.c'])
        if 'gdbstub-xml-arm.c' in src:
          return self.get_custom_cmd('bash'
                                     '{0}/scripts/feature_to_c.sh'
                                     '${{CMAKE_CURRENT_BINARY_DIR}}/gdbstub-xml-arm.c'
                                     '{0}/gdb-xml/arm-core.xml'
                                     '{0}/gdb-xml/arm-vfp.xml'
                                     '{0}/gdb-xml/arm-vfp3.xml'
                                     '{0}/gdb-xml/arm-neon.xml'.format(get_root()), ['gdbstub-xml-arm.c'])
    return ''


  def get_generated(self):
    target = '\n ## GENERATED SOURCES ##\n'
    if 'LOCAL_GENERATED_SOURCES' not in self.module:
      return target

    if 'libGLESv2_dec' in self.get_module_name():
      target += 'message("Looking for emugen in ${emugen_BIN}")\n'
      target += self.get_custom_cmd('echo ${{emugen_BIN}}/emugen -D ${{CMAKE_BINARY_DIR}} -i ${0} gles2'.format(self.get_relative_to_location('.')),
	['gles2_dec.cpp', 'gles2_dec.h', 'gles2_opcodes.h', 'gles2_server_context.h', 'gles2_server_context.cpp'])
      target += 'add_dependencies({} emugen)\n'.format(self.get_module_name())
      self.module['LOCAL_STATIC_LIBRARIES'].append(self.find_target('emugen').get_module_name())
      return target

    target += '\n'.join([self.generate_src(x) for x in self.get_property_as_list('LOCAL_GENERATED_SOURCES')])
    return target

  def get_project_header(self):
    target = 'cmake_minimum_required(VERSION 2.8)\n'
    target += 'project({})\n'.format(self.get_module_name())
    return target

  def get_proto_compile(self):
    if 'LOCAL_PROTO_SOURCES' not in self.module:
      return ''

    name = self.get_module_name()
    location = self.module['LOCAL_PATH']
    protodir = os.path.abspath(
        get_root() + '/../protobuf/' + self.module['HOST'])
    protopre = os.path.abspath(
        get_root() +
        '/../../prebuilts/android-emulator-build/protobuf/' +
        self.module['HOST'])

    protos = self.get_property_as_list('LOCAL_PROTO_SOURCES')
    target  = '\n ## Protocol buffers ##\n'
    target += 'include(FindProtobuf)\n'
    target += 'set(Protobuf_LIBRARIES   {}/lib/libprotobuf.a)\n'.format(protodir)
    target += 'set(Protobuf_INCLUDE_DIR {}/include)\n'.format(protodir)
    target += 'set(Protobuf_PROTOC_EXECUTABLE {}/bin/protoc)\n'.format(protopre)

    for proto in protos:
      proto = os.path.abspath(location + '/' + proto)
      name, ext =  os.path.splitext(os.path.basename(proto))
      target += 'PROTOBUF_GENERATE_CPP({0}_SRC {0}_HDRS {1})\n'.format(name, proto)
      self.src_set.append('{}_SRC'.format(name))

    return target

  def get_linker_flags(self):
    target = '\n ## Linker Flags ##\n'
    if 'LOCAL_LDFLAGS' not in self.module:
      return target

    libs = ['link_directories({})\n'.format(l[2:])
            for l in self.get_property_as_list('LOCAL_LDFLAGS') if l.startswith('-L')]
    target += '\n '.join(libs)
    return target

  def get_target_link(self):
    name = self.get_module_name()
    location = self.get_module_location()
    target = '\n ## Link libraries ## \n'

    if 'LOCAL_PROTO_SOURCES' in self.module:
      for proto in self.get_property_as_list('LOCAL_PROTO_SOURCES'):
        proto, ext =  os.path.splitext(os.path.basename(proto))
        target += 'add_custom_command(TARGET {} POST_BUILD\n'.format(name)
        target += '  COMMAND ${{CMAKE_COMMAND}} -E copy ${{{0}_HDRS}} ${{CMAKE_BINARY_DIR}}/{1}/{0}.pb.h'.format(proto, location)
        target += ')\n'

    libs = []
    if 'LOCAL_LDLIBS' in self.module:
      for lib in self.get_property_as_list('LOCAL_LDLIBS'):
        if lib.endswith('a'):
          lib, ext =  os.path.splitext(os.path.basename(lib))
          libs.append(lib)
        elif lib.startswith('-'):
          libs.append(lib)


    target += '\n'.join(['target_link_libraries({} {})'.format(name, lib) for lib in libs])

    return target

  def get_definitions(self):
    target = '\n## Definitions ##\n'
    flags = []

    target += '\n'.join(['add_definitions({})'.format(f) for f in self.get_property_as_list('LOCAL_CFLAGS') if f.startswith('-D')])
    target += '\n'
    target += '\n'.join(['add_definitions({})'.format(f) for f in self.get_property_as_list('LOCAL_CXXFLAGS') if f.startswith('-D')])
    return target

  def get_compiler_flags(self):
    target = '\n## Compiler flags ##\n'
    flags = []

    for flag in self.get_property_as_list('LOCAL_CFLAGS'):
      if not flag.startswith('-D') and not '-Werror' in flag:
        target += 'set(CMAKE_C_FLAGS "${{CMAKE_C_FLAGS}} {}")\n'.format(flag)
        target += 'set(CMAKE_CXX_FLAGS "${{CMAKE_CXX_FLAGS}} {}")\n'.format(flag)

    for flag in self.get_property_as_list('LOCAL_CXXFLAGS'):
      if not flag.startswith('-D') and not '-Werror' in flag:
        target += 'set(CMAKE_CXX_FLAGS "${{CMAKE_CXX_FLAGS}} {}")\n'.format(flag)

    target += 'set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")\n'
    target += 'if (UNIX)\n'
    target += '  include_directories({0}/linux-headers)\n'.format(self.get_relative_to_location('.'))
    target += 'endif() \n'
    target += 'if (APPLE)\n'
    target += '  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reserved-user-defined-literal -Winvalid-constexpr")\n'
    target += 'endif()\n'
    return target

  def get_toolchain(self):

    target = '\n ## Toolchain settings.. ##\n'
    if 'BUILD_HOST_CC' not in self.module:
      return target
    target += 'if (WITH_TOOLCHAIN)\n'
    target += '  set(CMAKE_C_COMPILER {}/{} CACHE STRING "C Compiler")\n'.format(get_root(), self.module['BUILD_HOST_CC'])
    target += '  set(CMAKE_CXX_COMPILER {}/{} CACHE STRING "C++ Compiler")\n'.format(get_root(), self.module['BUILD_HOST_CXX'])
    target += '  set(CMAKE_C_LINK_EXECUTABLE {}/{} CACHE STRING "Linker")\n'.format(get_root(), self.module['BUILD_HOST_LD'])
    target += '  set(CMAKE_AR {}/{} CACHE STRING "Archiver")\n'.format(get_root(), self.module['BUILD_HOST_AR'])
    target += '  set(WITH_TOOLCHAIN "Only setting toolchain once" OFF)\n'
    target += '  message("Configured toolchain")\n'
    target += 'endif()\n'
    return  target

  def find_target(self, name):
    for m in self.modules:
      if name == m['LOCAL_MODULE']:
        return Module(m, self.modules, self.basedir)

  def get_dependencies(self):
    dependencies = []
    modules = (self.get_property_as_list('LOCAL_STATIC_LIBRARIES')
               + self.get_property_as_list('LOCAL_WHOLE_STATIC_LIBRARIES'))
    for name in sorted(modules):
      submodule = self.find_target(name)
      dependencies += [submodule] + [d for d in submodule.get_dependencies()]

    return dependencies

  def add_dependency(self, module):
    self.dependencies.append(module)

  def get_subprojects(self):
    target = '\n ## Dependencies ##\n'

    deps = self.get_dependencies() + self.dependencies
    for m in deps:
      # We have to guard against double inclusion
      target += 'if(NOT TARGET {})\n'.format(m.get_module_name())
      target += '  message("adding {}")\n'.format(m.get_module_name())
      target += '  add_subdirectory({} {})\n'.format(m.get_module_location(), m.get_module_name())
      target += 'endif()\n'
    return target

  def get_export(self):
    target = '\n ## External Variables ## \n'
    target += 'set(${PROJECT_NAME}_BIN ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "${PROJECT_NAME}: Binary Directories" FORCE)'
    return target

  def get_qt_moc(self):
    if 'LOCAL_QT_MOC_SRC_FILES' not in self.module:
      return ''

    target = '\n ## QT Settings ## \n'
    target += 'include_directories(${CMAKE_CURRENT_BINARY_DIR})\n'
    target += 'if(NOT TARGET Qt5::moc)\n'
    target += '  set(QT_VERSION_MAJOR 5)\n'
    target += '  set(CMAKE_AUTOMOC TRUE)\n'
    target += '  set(CMAKE_AUTOUIC TRUE)\n'
    target += '  set(CMAKE_AUTORCC TRUE)\n'
    target += '  set(QT_MOC_EXECUTABLE {}/bin/moc)\n'.format(self.module['QT_TOP64_DIR'])
    target += '  set(QT_UIC_EXECUTABLE {}/bin/uic)\n'.format(self.module['QT_TOP64_DIR'])
    target += '  set(QT_RCC_EXECUTABLE {}/bin/rcc)\n'.format(self.module['QT_TOP64_DIR'])
    target += '  add_executable(Qt5::moc IMPORTED GLOBAL)\n'
    target += '  set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION ${QT_MOC_EXECUTABLE})\n'
    target += '  add_executable(Qt5::uic IMPORTED GLOBAL)\n'
    target += '  set_property(TARGET Qt5::uic PROPERTY IMPORTED_LOCATION ${QT_UIC_EXECUTABLE})\n'
    target += '  add_executable(Qt5::rcc IMPORTED GLOBAL)\n'
    target += '  set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION ${QT_RCC_EXECUTABLE})\n'
    target += 'endif()\n'
    return target

  def get_install(self):
    target = '\n ## install ##\n'
    if self.get_type() != 'RUNTIME' or 'LOCAL_INSTALL' not in self.module:
      return target

    target += 'install(TARGETS {} DESTINATION /tmp/emu/{})'.format(self.get_module_name(), self.module['LOCAL_INSTALL_DIR'])
    return target

  def get_property_as_list(self, name):
    if name not in self.module:
      return []
    v = self.module[name]
    if (isinstance(v, list)):
      return v

    return [v]

  def to_project(self):
    target = '## AUTOGENERATED ##\n'
    target += self.get_project_header()

    if not self.is_prebuilt():
      target += self.get_toolchain()
      target += self.get_qt_moc()
      target += self.get_compiler_flags()
      target += self.get_definitions()
      target += self.get_include_directories()
      target += self.get_src_list()
      target += self.get_proto_compile()
      target += self.get_linker_flags()
      target += self.get_generated()
      target += self.get_subprojects()

    target += self.get_target_type()
    target += self.get_target_link()
    target += self.get_export()
    target += self.get_install()
    return target

  def write(self):
    location = self.get_module_location()

    if not os.path.exists(location):
      os.makedirs(location)

    logging.info('Writing %s -> %s', self.get_module_name(), location)
    with open(location + '/CMakeLists.txt', 'w') as target:
      target.write(self.to_project())

def get_root():
  return os.path.abspath(os.getcwd())

def subprocess_cmd(command):
  logging.info('Executing %s', command)
  process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
  proc_stdout, proc_stderr = process.communicate()
  process.wait()
  return [process, proc_stdout, proc_stderr]

def main(argv=None):
  parser = argparse.ArgumentParser(
      description='Generates a set of cmake files'
      'based up the json representation.'
      'Use this to generate cmake files that can be consumed by clion')
  parser.add_argument('-i', '--input', dest='input', type=str, required=True,
                      help='json file containing the build tree')
  parser.add_argument('-d', '--dir',
                      dest='outdir', type=str, default='objs/cmake',
                      help='Output directory for created cmakefiles')
  parser.add_argument('-v', '--verbose',
                      action='store_const', dest='loglevel',
                      const=logging.INFO, default=logging.ERROR,
                      help='Log what is happening')
  parser.add_argument('-c', '--clean', dest='output', type=str,
                      default=None,
                      help='Write out the cleaned up json')
  parser.add_argument('-t', '--test', dest='test', action='store_true',
                      help='Build all the generated projects (slow!)')
  args = parser.parse_args()

  logging.basicConfig(level=args.loglevel)

  with open(args.input) as data_file:
    data = json.load(data_file)

  modules = cleanup_json(data)

  if (args.output is not None):
    with open(args.output, 'w') as out_file:
      out_file.write(json.dumps(modules, indent=4))

  logging.info('Writing cmake files to %s', args.outdir)

  all = Module({'LOCAL_PATH': '.', 'LOCAL_MODULE' : 'binaries'}, modules, args.outdir)
  tests = Module({'LOCAL_PATH': '.', 'LOCAL_MODULE' : 'tests'}, modules, args.outdir)

  for module in modules:
    m = Module(module, modules, args.outdir)
    all.add_dependency(m)
    if 'unittest' in m.get_module_name():
      tests.add_dependency(m)
    m.write()

  all.write()
  tests.write()

if __name__ == "__main__":
    sys.exit(main())

