#!/bin/python
import argparse
import itertools
import json
import fnmatch
import logging
import os
import sys
import time

this = sys.modules[__name__]
this.counter = 1


def cleanup_json(data):
    '''Cleans up the json structure by removing empty "", and empty key value
    pairs'''
    if (isinstance(data, unicode)):
        copy = data.strip()
        return None if len(copy) == 0 else copy

    if (isinstance(data, dict)):
        copy = {}
        for key, value in data.iteritems():
            rem = cleanup_json(value)
            if (rem is not None):
                copy[key] = rem
        return None if len(copy) == 0 else copy

    if (isinstance(data, list)):
        copy = []
        for elem in data:
            rem = cleanup_json(elem)
            if (rem is not None):
                copy.append(rem)

        if len(copy) == 0:
            return None
        if len(copy) == 1:
            return copy[0]
        return copy


class AttrDict(dict):
    def __init__(self, *args, **kwargs):
        super(AttrDict, self).__init__(*args, **kwargs)
        self.__dict__ = self

    def as_list(self, name):
        v = self.get(name, [])
        if (isinstance(v, list)):
            return v

        return [v]


def get_next_id():
    '''Generates a unique id'''
    this.counter += 1
    return 'gen_{}'.format(this.counter)


class Module:

    type_map = {
        'STATIC_LIBRARY': 'ARCHIVE',
        'STATIC_LIBRARIES': 'ARCHIVE',
        'SHARED_LIBRARY': 'LIBRARY',
        'SHARED_LIBRARIES': 'LIBRARY',
        'EXECUTABLE': 'RUNTIME',
        'EXECUTABLES': 'RUNTIME'
    }

    flag_filter = staticmethod(
        lambda x: (not x.startswith('-D') and not x.startswith('-I'))
    )

    emugen_map = {
        'libGLESv1_dec': 'gles1',
        'libGLESv2_dec': 'gles2',
        'lib_renderControl_dec': 'renderControl'
    }

    def __init__(self, js, parent):
        self.js = js
        self.parent = parent
        self.name = js['LOCAL_MODULE']
        self.loc = js['LOCAL_PATH']
        self.definitions = self.parse_definitions(js)
        self.libs = []
        for  l in js.as_list('LOCAL_LDLIBS'):
          if l.endswith('a'):
            self.libs.append(os.path.splitext(os.path.basename(l))[0])
          elif l.startswith('-'):
            self.libs.append(l)

        self.link_paths = [l[2:] for l in js.as_list('LOCAL_LDFLAGS')
                           if l.startswith('-L')]

        self.type = self.get_type()
        self.src = []
        self.gen = []
        self.includes = []
        self.depends_on = []
        if not self.is_prebuilt():
            self.init_sources()

    def init_sources(self):
        self.includes = [self.loc]
        self.includes += [self.get_path(x) for x
                          in self.js.as_list('LOCAL_C_INCLUDES')]
        self.src = [self.parse_src(x) for x in ['LOCAL_SRC_FILES',
                                                'LOCAL_QT_UI_SRC_FILES',
                                                'LOCAL_QT_RESOURCES',
                                                'LOCAL_QT_MOC_SRC_FILES']]
        self.src.append(
            self.parse_proto(
                self.js.as_list('LOCAL_PROTO_SOURCES')))
        self.src = list(itertools.chain.from_iterable(self.src))

        if (self.name in Module.emugen_map):
            self.src += self.parse_emugen(os.path.abspath(self.loc),
                                          Module.emugen_map[self.name])

        if not self.src:
            self.src = self.parse_dummy()

        # Add our dependencies.
        self.gen += self.parent.get_generated_configure_srcs(
            self.js.as_list('LOCAL_GENERATED_SOURCES'))
        self.depends_on = (
            self.js.as_list('LOCAL_STATIC_LIBRARIES') +
            self.js.as_list('LOCAL_WHOLE_STATIC_LIBRARIES'))
        if ('qemu2' in self.name):
            self.depends_on.append('libqemu2-common')

    def get_path(self, x):
        return os.path.abspath(x)

    def is_prebuilt(self):
        src = self.js.as_list('LOCAL_SRC_FILES')
        return src and src[0].endswith('.a')

    def get_type(self):
        typ = self.js.get('LOCAL_MODULE_CLASS', None)
        return Module.type_map.get(typ, None)

    def parse_src(self, ls):
        if ls in self.js:
            return [{'src': [self.get_path(self.loc + '/' + x)
                             for x in self.js.as_list(ls)],
                     'id': self.name + '_' + get_next_id(),
                     'gen': None
                     }]
        return []

    def parse_proto(self, proto):
        protoc = os.path.abspath(
                 get_root() +
                 '/../../prebuilts/android-emulator-build/protobuf/' +
                 self.js['HOST'] + '/bin/protoc')
        lst = []
        for x in proto:
            path = os.path.dirname(x)
            fname, ext = os.path.splitext(os.path.basename(x))
            src_loc = os.path.abspath(self.loc + '/' + path)
            lst.append(
                {'src': [
                    '{}/{}.pb.h'.format(path, fname),
                    '{}/{}.pb.cc'.format(path, fname)
                ],
                    'id': self.name + '_' + get_next_id(),
                    'gen': 'mkdir -p {0} && {1} -I{2} --cpp_out={0} {3}'.format(
                        path, protoc,
                        src_loc, src_loc + '/' + fname + ext
                    )
                })
        return lst

    def parse_dummy(self):
        return [{'src': ['empty-dummy.cpp'],
                 'id': get_next_id(),
                 'gen': 'touch {out}/empty-dummy.cpp'}]

    def parse_emugen(self, location, typ):
        return [{
            'src': [typ + x for x in ['_dec.cpp', '_dec.h', '_opcodes.h',
                                      '_server_context.h',
                                      '_server_context.cpp']
                    ],
            'depends_on': ['emugen'],
            'id': get_next_id(),
            'gen': 'emugen -D {{out}} -i {} {}'.format(location, typ)
        }]

    def parse_flags(self, js, prop, predicate):
        return [i for i in js.get(prop, []) if predicate(i)]

    def parse_definitions(self, js):
        return (
            self.parse_flags(js, 'LOCAL_CFLAGS', is_definition) +
            self.parse_flags(js, 'LOCAL_CXXFLAGS', is_definition)
        )

    # Printing functions
    def print_includes(self):
        target = ''
        for inc in self.includes:
            target += 'target_include_directories({} PRIVATE '.format(self.name)
            target += '{})\n'.format(inc)
        return target

    def print_link(self):
        if not self.libs:
            return ''

        target = '\n'
        target += '\n'.join(['target_link_libraries({} {})'
                             .format(self.name, lib) for lib in self.libs])
        return target

    def print_target(self):
        if self.is_prebuilt():
            src = self.js['LOCAL_SRC_FILES']
            target = 'add_library({} STATIC IMPORTED GLOBAL)\n'.format(
                self.name)
            target += 'set_property(TARGET {} '.format(self.name)
            target += 'PROPERTY IMPORTED_LOCATION "{}")\n'.format(src)
            return target

        type_target_map = {
            'ARCHIVE': 'add_library({} {})\n',
            'LIBRARY': 'add_library({} SHARED {})\n',
            'RUNTIME': 'add_executable({} {})\n'
        }
        source_set = ' '.join(
            ['${{SRC_{}}}'.format(s['id']) for s in (self.src + self.gen)])
        return type_target_map.get(self.type, '').format(
            self.name, source_set)


class ModuleCollection:

    def __init__(self, modules):
        # Get the qt dir
        module = next(m for m in modules if m.get('QT_TOP64_DIR', None))
        self.host = module.get('HOST', None)
        self.id = get_next_id()
        self.qt_dir = module.get('QT_TOP64_DIR')
        self.src = self.gen_configure()
        self.modules = [Module(AttrDict(m), self) for m in modules
                        if 'qemu1' not in m['LOCAL_PATH'] and 'upstream' not in m['LOCAL_PATH']]
        self.version = time.strftime('+%Y-%m-%d')
        self.link_dirs = set()
        for ld in [m.link_paths for m in self.modules]:
            for l in ld:
                self.link_dirs.add(l)

    def get_generated_configure_srcs(self, files):
        gen = []
        basenames = [os.path.basename(f) for f in files]
        for f in basenames:
            gen += [s for s in self.src if f in
                    [os.path.basename(x) for x in s['src']]]
        return gen

    def gen_configure(self):
        '''Set of generated files as part of the configure script,
           almost all modules require this'''
        traces = []
        for root, dirnames, filenames in os.walk(get_root()):
            for filename in fnmatch.filter(filenames, 'trace-events'):
                traces.append(os.path.join(root, filename))
        trcs = ' '.join(traces)
        return [
            {
                'src': ['android/avd/hw-config-defs.h'],
                'id': get_next_id(),
                'gen': 'python {root}/android/scripts/gen-hw-config.py '
                '{root}/android/android-emu/'
                'android/avd/hardware-properties.ini '
                '{out}/android/avd/hw-config-defs.h '
            },
            {
                'src': ['gdbstub-xml-arm64.c'],
                'id': get_next_id(),
                'gen': 'bash {root}/scripts/feature_to_c.sh '
                '{out}/gdbstub-xml-arm64.c'
                ' {root}/gdb-xml/aarch64-core.xml'
                ' {root}/gdb-xml/aarch64-fpu.xml'
                ' {root}/gdb-xml/arm-core.xml'
                ' {root}/gdb-xml/arm-vfp.xml'
                ' {root}/gdb-xml/arm-vfp3.xml'
                ' {root}/gdb-xml/arm-neon.xml'
            },
            {
                'src': ['gdbstub-xml-arm.c'],
                'id': get_next_id(),
                'gen': 'bash {root}/scripts/feature_to_c.sh'
                ' {out}/gdbstub-xml-arm.c'
                ' {root}/gdb-xml/arm-core.xml'
                ' {root}/gdb-xml/arm-vfp.xml'
                ' {root}/gdb-xml/arm-vfp3.xml'
                ' {root}/gdb-xml/arm-neon.xml'
            },
            {
                'src': ['qapi-event.c'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/qapi-event.py -o {out} '
                '{root}/qapi-schema.json'
            },
            {
                'src': ['qapi-types.c'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/qapi-types.py --builtins -o '
                '{out} {root}/qapi-schema.json'
            },

            {
                'src': ['qapi-visit.c'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/qapi-visit.py --builtins -o '
                '{out} {root}/qapi-schema.json'
            },
            {
                'src': ['qmp-introspect.c'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/qapi-introspect.py -o {out} '
                '{root}/qapi-schema.json'
            },
            {
                'depends_on': [
                    'hmp-commands.h',
                    'hmp-commands-info.h',
                    'qemu-img-cmds.h'
                ],
                'src': ['qmp-marshal.c'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/qapi-commands.py  '
                '-o {out} '
                '{root}/qapi-schema.json'
            },
            {
                'src': ['hmp-commands.h'],
                'id': get_next_id(),
                'gen': 'sh {root}/scripts/hxtool -h < {root}/hmp-commands.hx '
                ' > {out}/hmp-commands.h'
            },
            {
                'src': ['hmp-commands-info.h'],
                'id': get_next_id(),
                'gen': 'sh {root}/scripts/hxtool -h <'
                ' {root}/hmp-commands-info.hx '
                ' > {out}/hmp-commands-info.h'
            },
            {
                'src': ['qemu-img-cmds.h'],
                'id': get_next_id(),
                'gen': 'sh {root}/scripts/hxtool -h < {root}/qemu-img-cmds.hx '
                ' > {out}/qemu-img-cmds.h'
            },
            {
                'src': ['trace-events-all'],
                'id': 'gen-all-events',
                'gen': 'cat {} > '.format(trcs) +
                ' {out}/trace-events-all'
            },
            {
                'depends_on': ['generated-tracers.h',
                               'generated-helpers.h',
                               'generated-helpers-wrappers.h',
                               'generated-tcg-tracers.h'],
                'src': ['generated-tracers.c'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/tracetool.py  --backends=nop '
                '--format=c --target-type system --out '
                '{out}/generated-tracers.c '
                '{out}/trace-events-all'
            },
            {
                'depends_on': ['trace-events-all'],
                'src': ['generated-tracers.h'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/tracetool.py  --backends=nop '
                '--format=h --target-type system --out '
                '{out}/trace/generated-tracers.h '
                '{out}/trace-events-all'
            },
            {
                'depends_on': ['trace-events-all'],
                'src': ['generated-helpers.h'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/tracetool.py  --backends=nop '
                '--format=tcg-helper-h --target-type system  --out '
                '{out}/trace/generated-helpers.h '
                '{out}/trace-events-all'
            },
            {
                'depends_on': ['trace-events-all'],
                'src': ['generated-helpers-wrappers.h'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/tracetool.py  --backends=nop '
                '--format=tcg-helper-h --target-type system --out '
                '{out}/trace/generated-helpers-wrappers.h '
                '{out}/trace-events-all'
            },
            {
                'depends_on': ['trace-events-all'],
                'src': ['generated-tcg-tracers.h'],
                'id': get_next_id(),
                'gen': 'python {root}/scripts/tracetool.py  --backends=nop '
                '--format=tcg-h --target-type system  --out '
                '{out}/trace/generated-tcg-tracers.h '
                '{out}/trace-events-all'
            }
            ]

    def print_qt(self):
        if not self.qt_dir:
            return ''

        target = '\n ## QT Settings ## \n'
        target += 'if(NOT TARGET Qt5::moc)\n'
        target += '  set(QT_VERSION_MAJOR 5)\n'
        target += '  set(CMAKE_AUTOMOC TRUE)\n'
        target += '  set(CMAKE_AUTOUIC TRUE)\n'
        target += '  set(CMAKE_AUTORCC TRUE)\n'
        target += '  set(QT_MOC_EXECUTABLE {}/bin/moc)\n'.format(self.qt_dir)
        target += '  set(QT_UIC_EXECUTABLE {}/bin/uic)\n'.format(self.qt_dir)
        target += '  set(QT_RCC_EXECUTABLE {}/bin/rcc)\n'.format(self.qt_dir)
        target += '  add_executable(Qt5::moc IMPORTED GLOBAL)\n'
        target += '  set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION'
        target += '  ${QT_MOC_EXECUTABLE})\n'
        target += '  add_executable(Qt5::uic IMPORTED GLOBAL)\n'
        target += '  set_property(TARGET Qt5::uic PROPERTY IMPORTED_LOCATION'
        target += '  ${QT_UIC_EXECUTABLE})\n'
        target += '  add_executable(Qt5::rcc IMPORTED GLOBAL)\n'
        target += '  set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION'
        target += '  ${QT_RCC_EXECUTABLE})\n'
        target += 'endif()\n'
        return target

    def print_gen_src(self):
        target = '\n ## Generated Config ## \n'
        target += print_src(self.src)
        return target + '\n'

    def print_modules(self):
        target = '\n ## Source sets ##\n'

        for m in self.modules:
            target += print_src(m.src)

        target += '\n ## Targets ##\n'

        for m in self.modules:
            target += m.print_target()

        target += '\n ## Target definitions ##\n'
        for m in self.modules:
            if m.definitions:
                target += 'target_compile_definitions({} PRIVATE {})\n'.format(
                        m.name, ' '.join(m.definitions))

        target += '\n ## Includes ##\n'
        for m in self.modules:
            target += m.print_includes()

        target += '\n ## Link ##\n'
        for m in self.modules:
            target += m.print_link()

        target += '\n ## Install ##\n'
        target += 'get_filename_component(INSTALL_PATH '
        target += '${CMAKE_BINARY_DIR} ABSOLUTE)\n'
        for m in self.modules:
            if 'LOCAL_INSTALL_DIR' in m.js:
                target += ('install(TARGETS {} DESTINATION '
                           '${{INSTALL_PATH}}/{})\n'
                           .format(m.name,
                                   m.js['LOCAL_INSTALL_DIR']))
        return target

    def print_dependency_graph(self):
        target = '\n ## Dependencies ##\n'

        # Setup module dependencies
        for m in self.modules:
            for dep in m.depends_on:
                target += 'add_dependencies({} {})\n'.format(m.name, dep)
            for s in m.src + self.src:
                if s.get('gen', None) and m.name != s.get('depends_on', ''):
                    target += ('add_dependencies({} {})\n'
                               .format(m.name, s['id']))
        return target
 

    def print_platform_headers(self):
        target = 'if (UNIX)\n'
        target += '  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")\n'
        target += '  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")\n'
        target += '  include_directories("linux-headers")\n'.format(get_root())
        target += 'endif()\n'
        target += 'if (APPLE)\n'
        target += '  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}'
        target += '  -Wno-reserved-user-defined-literal")\n'
        target += 'endif()\n'
        return target

    def to_project(self):
        target = '## AUTOGENERATED ##\n'
        target += 'cmake_minimum_required(VERSION 3.0)\n'
        target += 'include_directories(${CMAKE_CURRENT_BINARY_DIR})\n'
        target += 'set(CMAKE_CXX_STANDARD 14)\n'
        target += 'set(CMAKE_CXX_STANDARD_REQUIRED ON)\n'
        target += 'set(HOST "{}")\n'.format(self.host)
        target += self.print_platform_headers()
        target += ('file(WRITE qemu-version.h "#define QEMU_PKGVERSION '
                   '\\"{})\\"")\n'.format(self.version))
        target += ('file(COPY {}/android-qemu2-glue/config/${{HOST}}'
                   '/config-host.h DESTINATION '
                   '${{CMAKE_CURRENT_BINARY_DIR}})\n'.format(get_root()))

        target += '\n ## Link directories ##\n'
        target += '\n'.join([
            'link_directories({})\n'.format(l)
            for l in self.link_dirs])
        target += self.print_qt()
        target += self.print_gen_src()
        target += self.print_modules()
        target += self.print_dependency_graph()

        return target


def is_definition(x):
    return x.startswith('-D')


def print_src(srcs):
    target = ''
    for src in srcs:
        if src['gen']:
            src['src'] = ['${CMAKE_CURRENT_BINARY_DIR}/' +
                          x for x in src['src']]
            output = ' '.join(src['src'])
            target += 'add_custom_command(OUTPUT {} '.format(output)
            target += 'COMMAND ' + src['gen'].format(
                **{'out': '.',
                   'root': get_root()
                   })
            if ('depends_on' in src):
                src['depends_on'] = ['${CMAKE_CURRENT_BINARY_DIR}/' +
                                     x for x in src['depends_on']]
                target += ' DEPENDS {} '.format(' '.join(src['depends_on']))
            target += ' VERBATIM )\n'
            for s in src['src']:
                target += 'set_source_files_properties({} '.format(s)
                target += 'PROPERTIES GENERATED TRUE)\n'
            target += 'add_custom_target({} DEPENDS {})\n'.format(src['id'],
                                                                  output)

        target += 'set(SRC_{} {})\n'.format(src['id'],
                                            ' '.join(src['src']))

    return target


def get_root():
    return os.path.abspath(os.getcwd())


def main(argv=None):
    parser = argparse.ArgumentParser(
        description='Generates a set of cmake files'
        'based up the js representation.'
        'Use this to generate cmake files that can be consumed by clion')
    parser.add_argument('-i', '--input', dest='input', type=str, required=True,
                        help='js file containing the build tree')
    parser.add_argument('-o', '--output',
                        dest='outdir', type=str, default=None,
                        help='Output directory for create CMakefile.txt')
    parser.add_argument('-v', '--verbose',
                        action='store_const', dest='loglevel',
                        const=logging.INFO, default=logging.ERROR,
                        help='Log what is happening')
    parser.add_argument('-c', '--clean', dest='output', type=str,
                        default=None,
                        help='Write out the cleaned up js')
    parser.add_argument('-t', '--test', dest='test', action='store_true',
                        help='Build all the generated projects (slow!)')
    args = parser.parse_args()

    logging.basicConfig(level=args.loglevel)

    with open(args.input) as data_file:
        data = json.load(data_file)

    modules = cleanup_json(data)

    if (args.output is not None):
        with open(args.output, 'w') as out_file:
            out_file.write(json.dumps(modules, indent=2))

    collection = ModuleCollection(modules)

    target = collection.to_project()
    if (args.outdir is None):
        print target
    else:
        outfile = args.outdir + '/CMakeLists.txt'
        with open(outfile, 'w') as out_file:
            out_file.write(target)

if __name__ == "__main__":
    sys.exit(main())
