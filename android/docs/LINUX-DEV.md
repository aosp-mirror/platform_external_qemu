Android Emulator Linux Development
=====================================

This document describes how to get started with emulator development under linux.

# Software Requirements

The instructions here assume you are using a deb based distribution, such as Debian, Ubuntu or Goobuntu.

Install the following dependencies:
```sh
    sudo apt-get install -y git build-essential python qemu-kvm ninja-build python-pip ccache
```
We need python to run the scripts, kvm to be able to run the emulator and git to get the source.

## Obtaining repo

First we need to obtain the repo tool.
```sh
    mkdir $HOME/bin
    curl http://commondatastorage.googleapis.com/git-repo-downloads/repo > $HOME/bin/repo
    chmod 700 $HOME/bin/repo
```
Make sure to add `$HOME/bin` to your path, if it is not already there.

```sh
    export PATH=$PATH:$HOME/bin
```

Do not forget to add this to your `.bashrc` file.


 ### Initialize the repository:

You can initialize repo as follows:

```sh
    mkdir -p emu-master-dev && cd emu-master-dev
    repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev
```

***Note:*** You could use persistent-https to initialize the repo, this might give you slightly better performance if your have the persistent-https git plugin.

Sync the repo (and get some coffee, or a have a good nap.)

    repo sync -j 8

Congratulations! You have all the sources you need. Now run:

    cd external/qemu && android/rebuild.sh

If all goes well you should have a freshly build emulator in the `objs` directory.

You can pass the flag `--helpfull` to the rebuild script to get an idea of which options you can pass in. Look at the section under `aemu.cmake`
```sh
    android/rebuild.sh --helpfull

    aemu.cmake:
        --build: <config|check>: Target that should be build after configuration. The config target will only configure the build, no symbol processing or testing will take place.
            (default: 'check')
        --[no]clean: Clean the destination build directory before configuring. Setting this to false will attempt an incremental build. Note that this can introduce cmake caching issues.
            (default: 'true')
        --cmake_option: Options that should be passed on directly to cmake. These will be passed on directly to the underlying cmake project. For example: --cmake_option QEMU_UPSTREAM=FALSE;
            repeat this option to specify a list of values
            (default: '[]')
        --config: <debug|release>: Whether we are building a release or debug configuration.
            (default: 'release')
        --crash: <prod|staging|none>: Which crash server to use or none if you do not want crash uploads.enabling this will result in symbol processing and uploading during install.
            (default: 'none')
        --dist: Create distribution in directory
        --generator: <visualstudio|xcode|ninja|make>: CMake generator to use.
            (default: 'ninja')
        --out: Use specific output directory.
            (default: '/Users/jansene/src/emu-master-dev/external/qemu/objs')
        --[no]qtwebengine: Build with QtWebEngine support
            (default: 'false')
        --sanitizer: List of sanitizers ([address, thread]) to enable in the built binaries.
            (default: '')
            (a comma separated list)
        --sdk_build_number: The emulator sdk build number.
        --sdk_revision: ## DEPRECATED ##, it will automatically use the one defined in source.properties
        --target: <windows|linux|darwin|mingw>: Which platform to target. This will attempt to cross compile if the target does not match the current platform (linux)
            (default: 'linux')
        --test_jobs: Specifies  the number of tests to run simultaneously
            (default: '8')
            (an integer)
        --[no]tests: Run all the tests
            (default: 'true')
```
### Incremental builds

The rebuild script does a complete clean build. You can use ninja to partial builds:

    ninja -C objs

### Speeding up builds with 'ccache'

It is highly recommended to install the 'ccache' build tool on your development
machine(s). The Android emulator build scripts will probe for it and use it
if available, which can speed up incremental builds considerably.

    sudo apt-get install ccache

#### Optional ccache step

Configure ccache to use a different cache size with `ccache -M <max size>`. You can see a list of configuration options by calling ccache alone. * The default ccache directory is ~/.ccache. You might want to symlink it to another directory (for example, when using FileVault for your home directory). You should make this large if you are planning to cross compile to many targets (ming/windows).

### Cross compiling to Windows with clang-cl

***It is highly recommended to use a windows machine for windows development, vs cross compilation.***

It is possible to cross compile from linux to windows. This is mainly useful to quickly discover compilation issues, as you will not be able to actually run the code.

The windows target requires you to install the MSVC libraries. These libraries need manual intervention to be installed on your linux machine as you will need to:

#### Enable cross compiling from within Google:
run:

    $AOSP/external/qemu/android/scripts/activate-msvc.sh

This script will guide you through the steps, and configure your machine for cross compiling msvc. Once this is completed you should be able to replicate what is happening on the build bots. **The script will require sudo priveleges**

If all went well you can now target windows as follows:

    cd $AOSP/external/qemu/ && ./android/rebuild.sh --target windows

#### Enable cross compiling from outside Google:

  1. Strongly consider not doing this, and develop on a windows machine.
  2. You will need to have access to a Windows machine.
      - Install [Python](https://www.python.org/downloads/windows/)
      - Install [Chrome Depot Tools](https://dev.chromium.org/developers/how-tos/depottools)
      - Install [Visual Studio](https://visualstudio.microsoft.com/)

  3. Package your Windows SDK installation into a zip file by running the following on a Windows machine:

```sh
    cd path/to/depot_tools/win_toolchain
    # customize the Windows SDK version numbers
    python package_from_installed.py 2017 -w 10.0.17134.0
```

  4. Execute the following on your linux machine:

```sh
    export DISK_LOC=/mnt/jfs_msvc.img
    # Make sure we have JFS
    sudo apt-get install -y jfsutils
    sudo mkdir -p /mnt/msvc

    # Create a OS/2 case insensitive loopback fs.
    echo "Creating empty img in /tmp/jfs.img"
    dd if=/dev/zero of=/tmp/jfs.img bs=1M count=4096
    chmod a+rwx /tmp/jfs.img
    echo "Making filesystem"
    mkfs.jfs -q -O /tmp/jfs.img
    sudo mv /tmp/jfs.img ${DISK_LOC}

    sudo mount ${DISK_LOC} /mnt/msvc -t jfs -o loop
    sudo chmod a+rwx /mnt/msvc
    unzip msvc_zip -D /mnt/msvc
```

If all went well you can now target windows as follows:

    ./android/rebuild.sh --target windows

### Cross compiling to Darwin with clang-cl

***It is higly recommended to use a mac for mac development, vs cross compilation.***

It is possible to cross compile from linux to mac os. This is mainly useful to quickly discover compilation issues, as you will not be able to actually run the code.

The windows target requires you to install the a cross compilers. These libraries need manual intervention to be installed on your linux machine. You will need to
obtain a xcode.xip with MacOS-SDK (10.13-10.15) (Xcode 11.3 has been tested). You can obtain it from [go/xcode](http://go/xcode) if you're within Google or from
[Apple](https://developer.apple.com/download/more.).

Once you have obtained the .xip you can create the toolchain as follows:

```sh
  ./android/scripts/unix/activate-darwin-cross.sh --xcode=/data/.../Xcode_11.3.xip
```

After that succeeds you should be able to target darwin:

    ./android/rebuild.sh --target darwin


### Sending patches

Here you can find details on [submitting patches](
https://gerrit.googlesource.com/git-repo/+/refs/heads/master/SUBMITTING_PATCHES.md). No external configuation should be needed if you are within Google.
