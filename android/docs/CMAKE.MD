Building the Emulator
=====================

The emulator is build for basically 3 targets:

- Windows
- Linux
- Darwin (MacOs)

The python scripts are cross platform, and work on all supported platforms.

*Note:
The existing android/rebuild.sh script will continue to work on linux/darwin but will start accepting different parameters.*

## Just get me started

First we need to install all the dependencies:

```sh
   python android/build/python/setup.py develop --user
```

*Note: This only needs to happen once. It makes sure you have all the python dependencies available locally.*

Next you can run the build as follows:

```sh
   python android/build/python/cmake.py --build check
```

This will compile and run all the tests.  If you wish to just recompile and you have ninja on the path you can use:

```sh
   ninja -C objs
```

There are many more parameters that can be provided, you can get a list by:

```sh
   python android/build/python/cmake.py --helpfull
```

The flags of interrest are in the aemu.cmake module:

```
aemu.cmake:
  --build: <check|config>: Target that should be build after configuration. The config target will only configure the build, no symbol processing or testing will take place.
    (default: 'check')
  --[no]clean: Clean the destination build directory before configuring. Setting this to false will attempt an incremental build. Note that this can introduce cmake caching issues.
    (default: 'true')
  --config: <debug|release>: Whether we are building a release or debug configuration.
    (default: 'release')
  --crash: <none|prod|staging>: Which crash server to use or none if you do not want crash uploads.
    (default: 'none')
  --generator: <ninja|make|visualstudio|xcode>: CMake generator to use.
    (default: 'ninja')
  --out: Use specific output directory.
    (default: '/usr/local/google/home/jansene/src/emu-master-dev/external/qemu/objs')
  --[no]qtwebengine: Build with QtWebEngine support
    (default: 'false')
  --sanitizer: List of sanitizers ([address, thread]) to enable in the built binaries.
    (default: '')
    (a comma separated list)
  --sdk_build_number: The emulator sdk build number.
  --sdk_revision: The emulator sdk revision.
  --symbol_dest: <none|staging|prod>: Environment where we are uploading the symbols to. None will not upload any symbols.
    (default: 'none')
  --[no]symbols: Strip binaries and generate symbols after build.
    (default: 'false')
  --target: <darwin|linux|mingw|windows>: Which platform to target. This will attempt to cross compile if the target does not match the current platform (linux)
    (default: 'linux')
```


If you wish to generate a release configuration that does not get build you can:

```sh
   python android/build/python/cmake.py --config release --build config
```
### On Windows

You will need to have:

- Visual Studio
- Python
- Ninja

For example "Visual Studio Enterprise" provides all the required dependencies. Otherwise you can use [Chocolatey](https://chocolatey.org/install) to install all the dependencies:

In an administrative shell execute the following:

```sh
  C:> @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
  C:> choco install visualstudio2017professional
  C:> choco install ninja
  C:> choco install python2
```

Next open up a visual studio command prompt with admin privileges and follow the steps described above.


## Why?

In order to target these 3 platforms we make use of a build system that can run natively on all these platforms. We prefer this over cross compiling since:

- We want to run the unit tests natively on each platform.
- We want to be able to develop on the release architecture.
- We want build bots and extended tests for each architecture.

To do so we've use the following language in our build system:

- CMake: A Makefile generator that can generate build scripts for each platform. Libraries, binaries and dependencies should be expressed in CMake. The cmake files should be able to create a build that can be run and imported into an IDE so we can have a good developer experience.
- Python: For build actions that cannot be expressed in CMake. Things such as uploading symbols can be done here.


Some general guidelines if you wish to add new functionality to the build:

- Do not use Shell scripts. Shell scripts are not portable, they do not work out of the box on Windows. Existing shell scripts should be slowly migrated to python where applicable.
- Target python 2.6. Unfortunately our mac build bots are not the latest and greatest, so be careful when adding new dependencies.