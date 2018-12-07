from enum import Enum
import os
import platform

QEMU_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', '..', '..', '..'))
AOSP_ROOT = os.path.abspath(os.path.join(QEMU_ROOT, '..', '..'))
CLANG_VERSION = 'clang-r346389b'
CLANG_WIN_DIR = os.path.join(AOSP_ROOT, 'prebuilts', 'clang',
                         'host', '%s-x86' % platform.system().lower(), CLANG_VERSION, 'bin')

CMAKE_DIR = os.path.join(AOSP_ROOT, 'prebuilts/cmake/%s-x86/bin/' % platform.system().lower())
EXE_POSTFIX = ''
if platform.system() == 'Windows':
    EXE_POSTFIX = '.exe'



class ArgEnum(Enum):
    '''A class that can parse argument enums'''

    def __str__(self):
        return self.name

    def to_cmd(self):
       return self.value

    @classmethod
    def values(clzz):
        return [n.name for n in clzz]

    @classmethod
    def from_string(clzz, s):
        try:
            return clzz[s.lower()]
        except KeyError:
            raise ValueError()


class Crash(ArgEnum):
    '''All supported crash backends.'''
    prod = ['-DOPTION_CRASHUPLOAD=PROD']
    staging = ['-DOPTION_CRASHUPLOAD=STAGING']
    none = ['-DOPTION_CRASHUPLOAD=NONE']


class BuildConfig(ArgEnum):
    '''Supported build configurations.'''
    debug = ['-DCMAKE_BUILD_TYPE=Debug']
    release = ['-DCMAKE_BUILD_TYPE=Release']

class Make(ArgEnum):
    config = 'configure only'
    check = 'check'

class Generator(ArgEnum):
    '''Supported generators.'''
    visualstudio = ['-G', 'Visual Studio 15 2017 Win64']
    xcode = ['-G', 'Xcode']
    ninja = ['-G', 'Ninja']
    make = ['-G', 'Unix Makefiles']


class Toolchain(ArgEnum):
    '''Supported toolchains.'''
    windows = 'toolchain-windows-msvc-x86_64.cmake'
    linux = 'toolchain-linux-x86_64.cmake'
    darwin = 'toolchain-darwin-x86_64.cmake'

    def to_cmd(self):
        return ['-DCMAKE_TOOLCHAIN_FILE=%s' % os.path.join(QEMU_ROOT, 'android', 'build', 'cmake', self.value)]

