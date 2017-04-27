#!/bin/python
from pprint import pprint
import argparse
import json
import json
import logging
import os
import sys

def remove_empty(data):
    if (isinstance(data, unicode)):
        copy = data.strip()
        return None if len(copy) == 0 else copy
    if (isinstance(data, dict)):
        copy = {}
        for key,value in data.iteritems():
            rem = remove_empty(value)
            if (rem is not None):
                copy[key] = rem
        return None if len(copy) == 0 else copy

    if (isinstance(data, list)):
        copy = []
        for elem in data:
            rem = remove_empty(elem)
            if (rem is not None and not rem in copy):
                copy.append(rem)

        return None if len(copy) == 0 else sorted(copy)

def clean_includes(node):
    if 'LOCAL_CFLAGS' in node:
        cflags = []
        for cflag in node['LOCAL_CFLAGS']:
            if not cflag.startswith('-I'): cflags.append(cflag)
        node['LOCAL_CFLAGS'] = cflags

def is_prebuilt(module):
    return 'LOCAL_SRC_FILES' in module and module['LOCAL_SRC_FILES'][0].endswith('.a')

def is_leaf(module):
   return not 'LOCAL_STATIC_LIBRARIES' in module



def get_target_type(module):
    typ = module['TYPE']
    name = get_module_name(module)

    target = 'if(NOT TARGET {})\n'.format(name)
    # Check if it is an imported target
    if 'LOCAL_SRC_FILES' in module:
        src = module['LOCAL_SRC_FILES'][0]
        if  src.endswith('.a'):
             target += 'add_library({} STATIC IMPORTED GLOBAL)\n'.format(name)
             target += 'set_property(TARGET {} PROPERTY IMPORTED_LOCATION "{}")\n'.format(name, src)
             target += 'endif()\n'
             return target

    source_set = ''
    for src in module['SOURCE_SET']:
        source_set += '${{{}}} '.format(src)

    if (typ == 'STATIC_LIBRARIES' or typ == 'STATIC_LIBRARY'):
        target += 'add_library({} {})\n'.format(name, source_set)
    if (typ == 'SHARED_LIBRARY' or typ == 'SHARED_LIBRARIES'):
        target + 'add_library({} SHARED {})\n'.format(name, source_set)
    if (typ.startswith('EXECUTABLE')):
        target += 'add_executable({} {})\n'.format(name, source_set)

    target += 'endif()\n'
    return target

def get_include_directories(module):
    if 'LOCAL_C_INCLUDES' not in module: return ''

    location = get_module_location(module)
    incs = module['LOCAL_C_INCLUDES']
    incs.append(module['LOCATION'])
    lst = [os.path.abspath(x) for x in incs]
    return 'include_directories({})\n'.format(' '.join(lst))


def get_root():
    return os.path.abspath(os.getcwd())

def get_module_name(module):
    return module['MODULE']

def get_module_location(module):
    return os.path.abspath('objs/cmake/' + module['LOCATION'] + '/' + get_module_name(module))

def get_src_list(module):
    name = get_module_name(module)
    location = '{}/{}/'.format(get_root(), module['LOCATION'])
    target = ''

    if 'LOCAL_SRC_FILES' in module:
        src = [os.path.abspath(location + s) for s in module['LOCAL_SRC_FILES']]
        module['SOURCE_SET'].append('SOURCES_{}'.format(name))
        target += 'set (SOURCES_{0} {1})\n'.format(name, ' '.join(src))

    if 'LOCAL_QT_RESOURCES' in module: 
        src = [os.path.abspath(location + s) for s in module['LOCAL_QT_RESOURCES']]
        module['SOURCE_SET'].append('SOURCES_QT_{}'.format(name))
        target += 'set (SOURCES_QT_{0} {1})\n'.format(name, ' '.join(src))

    return target




def get_project_header(module):
    name = get_module_name(module)
    location = get_module_location(module)

    target = 'cmake_minimum_required(VERSION 3.6)\n'
    target += 'project({})\n'.format(name)
    return target

def get_proto_compile(module):
    if 'LOCAL_PROTO_SOURCES' not in module: return ''

    name = get_module_name(module)
    location = module['LOCATION'] 
    protodir = os.path.abspath(get_root() + '/../protobuf/' + module['HOST'])
    protopre =os.path.abspath(get_root() + '/../../prebuilts/android-emulator-build/protobuf/' + module['HOST'])
 
    protos = module['LOCAL_PROTO_SOURCES']
    target  = 'include(FindProtobuf)\n'
    target += 'set(Protobuf_LIBRARIES   {}/lib/libprotobuf.a)\n'.format(protodir)
    target += 'set(Protobuf_INCLUDE_DIR {}/include)\n'.format(protodir)
    target += 'set(Protobuf_PROTOC_EXECUTABLE {}/bin/protoc)\n'.format(protopre)
    source_set = []

    for proto in protos:
        proto = os.path.abspath(location + '/' + proto)
        name, ext =  os.path.splitext(os.path.basename(proto))
        target += 'PROTOBUF_GENERATE_CPP({0}_SRC {0}_HDRS {1})\n'.format(name, proto)
        module['SOURCE_SET'].append('{}_SRC'.format(name))


    return target

def get_linker_flags(module):
    target = ''
    if 'LOCAL_LDFLAGS' in module:
        for lib in module['LOCAL_LDFLAGS']:
            if lib.startswith('-L'):
                target += 'link_directories({})\n'.format(lib[2:])
    return target

def get_target_link(module):
    name = get_module_name(module)
    location = get_module_location(module)
    target = ''

    if 'LOCAL_PROTO_SOURCES' in module:
        for proto in module['LOCAL_PROTO_SOURCES']:
            proto, ext =  os.path.splitext(os.path.basename(proto))
            target += 'add_custom_command(TARGET {} POST_BUILD\n'.format(name)
            target += '  COMMAND ${{CMAKE_COMMAND}} -E copy ${{{0}_HDRS}} ${{CMAKE_BINARY_DIR}}/{1}/{0}.pb.h'.format(proto, location)
            target += ')\n'

    if 'LOCAL_STATIC_LIBRARIES' in module:
        for lib in module['LOCAL_STATIC_LIBRARIES']:
            target += 'target_link_libraries({} {})\n'.format(name, lib)

    if 'LOCAL_LDLIBS' in module: 
        for lib in module['LOCAL_LDLIBS']:
            if lib.endswith('a'):
                lib, ext =  os.path.splitext(os.path.basename(lib))
                target += 'target_link_libraries({} {})\n'.format(name, lib)
            elif lib.startswith('-'):
                target += 'target_link_libraries({} {})\n'.format(name, lib)


    return target


def get_compiler_flags(module):
    target = ''
    flags = []

    if 'LOCAL_CFLAGS' in module:
        for flag in module['LOCAL_CFLAGS']:
            if flag.startswith('-D'):
                target += 'add_definitions({})\n'.format(flag)
        else:
            target += 'set(CMAKE_C_FLAGS "${{CMAKE_C_FLAGS}} {}")\n'.format(flag)
    if 'LOCAL_CXXFLAGS' in module:
        for flag in module['LOCAL_CXXFLAGS']:
            if flag.startswith('-D'):
                target += 'add_definitions({})\n'.format(flag)
        else:
            target += 'set(CMAKE_CXX_FLAGS "${{CMAKE_CXX_FLAGS}} {}")\n'.format(flag)

    target += 'set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y  -Wno-reserved-user-defined-literal -Winvalid-constexpr")\n'
    return target

def get_qt_moc(module):
    if 'LOCAL_QT_MOC_SRC_FILES' not in module: return ''

    target = 'include_directories(${CMAKE_CURRENT_BINARY_DIR})\n'
    target+= 'if(NOT TARGET Qt5::moc)\n' 
    target+= 'set(QT_VERSION_MAJOR 5)\n'
    target+= 'set(CMAKE_AUTOMOC TRUE)\n'
    target+= 'set(CMAKE_AUTOUIC TRUE)\n'
    target+= 'set(CMAKE_AUTORCC TRUE)\n'
    target+= 'set(QT_MOC_EXECUTABLE {}/bin/moc)\n'.format(module['QT_TOP64_DIR'])
    target+= 'set(QT_UIC_EXECUTABLE {}/bin/uic)\n'.format(module['QT_TOP64_DIR'])
    target+= 'set(QT_RCC_EXECUTABLE {}/bin/rcc)\n'.format(module['QT_TOP64_DIR'])
    target+= 'add_executable(Qt5::moc IMPORTED GLOBAL)\n'
    target+= 'set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION ${QT_MOC_EXECUTABLE})\n'
    target+= 'add_executable(Qt5::uic IMPORTED GLOBAL)\n'
    target+= 'set_property(TARGET Qt5::uic PROPERTY IMPORTED_LOCATION ${QT_UIC_EXECUTABLE})\n'
    target+= 'add_executable(Qt5::rcc IMPORTED GLOBAL)\n'
    target+= 'set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION ${QT_RCC_EXECUTABLE})\n'
    target+= 'endif()\n'
    return target

def get_toolchain(module):
    target =  'set(CMAKE_C_COMPILER {}/{})\n'.format(get_root(), module['BUILD_HOST_CC'])
    target += 'set(CMAKE_CXX_COMPILER {}/{})\n'.format(get_root(), module['BUILD_HOST_CXX'])
    target += 'set(CMAKE_C_LINK_EXECUTABLE {}/{})\n'.format(get_root(), module['BUILD_HOST_LD'])
    target += 'set(CMAKE_AR {}/{})\n'.format(get_root(), module['BUILD_HOST_AR'])
    return target

def find_target(modules, name):
    for module in modules:
        if name == get_module_name(module): return module


def get_subprojects(modules, module):
    if 'LOCAL_STATIC_LIBRARIES' not in module: return ''

    target = ''
    for name in sorted(module['LOCAL_STATIC_LIBRARIES']):
        submodule = find_target(modules, name)
        if (submodule is None): print("Cannot find: ", name)
        target += 'add_subdirectory({} {})\n'.format(get_module_location(submodule), get_module_name(submodule))
    return target

def print_project(modules, module):
    module['SOURCE_SET'] = [] 
    location = get_module_location(module)
    name = get_module_name(module)
    if not os.path.exists(location): os.makedirs(location)
    print("Writing {} : {} -> {}".format(name, is_leaf(module), location))
    target = open(location + '/CMakeLists.txt', 'w')

    target.write(get_project_header(module))

    if not is_prebuilt(module):
        target.write(get_toolchain(module))
        target.write(get_compiler_flags(module))
        target.write(get_include_directories(module))
        target.write(get_src_list(module))
        target.write(get_proto_compile(module))
        target.write(get_qt_moc(module))
        target.write(get_linker_flags(module))
        target.write(get_subprojects(modules, module))

    target.write(get_target_type(module))
    target.write(get_target_link(module))
    target.close()

def main(argv=None):
    parser = argparse.ArgumentParser(description='Generates a set of cmake files'
            'based up the json representation')
    parser.add_argument('-i', '--input', dest='input', type=str, required=True,
                        help='json file containing the build tree')
    parser.add_argument('-d', '--debug', action="store_const", dest="loglevel", const=logging.DEBUG,
                        default=logging.ERROR, help="Print lots of debugging statements")
    parser.add_argument('-v', '--verbose',
                        action="store_const", dest="loglevel", const=logging.INFO,
                        help="Be more verbose")
    args = parser.parse_args()

    logging.basicConfig(level=args.loglevel)
    with open(args.input) as data_file: data = json.load(data_file)

    modules = remove_empty(data)
    for module in modules:
        clean_includes(module)
        print_project(modules, module)

if __name__ == "__main__":
    sys.exit(main())

