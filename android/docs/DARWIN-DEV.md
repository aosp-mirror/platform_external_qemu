Android Emulator MacOS Development
=====================================

This document describes how to get started with emulator development under MacOS. Throughout the emulator code base and documentation we ofter refer to MacOS as
[Darwin](https://www.howtogeek.com/295067/why-is-macos-software-sometimes-labeled-darwin/).

# Software Requirements

Make sure you have the latest version of [XCode](https://developer.apple.com/xcode/). You can obtain XCode here:

  - If you are within Google:
      -  Obtain [xcode](http://go/xcode).
      -  Make sure to get the proper [santa exceptions](http://go/santaexception)
  - From the [App store](https://apps.apple.com/us/app/xcode/id497799835?ls=1&mt=12)

Make sure to install the command line tools by executing:

    xcode-select --install

**NOTE:** *The recommended xcode version is [xcode 10.1](https://download.developer.apple.com/Developer_Tools/Xcode_10.1/Xcode_10.1.xip)*

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
Do not forget to add this to your `.bashrc` file or `.zshrc` if you use the z shell.


 ### Initialize the repository:

You can initialize repo as follows:

    mkdir -p $HOME/emu-master-dev && cd $HOME/emu-master-dev
    repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev

Sync the repo (and get some coffee, or a have a good nap.)

    repo sync -j 8

Congratulations! You have all the sources you need. Now run:

```sh
    cd external/qemu && android/rebuild.sh
```

If all goes well you should have a freshly build emulator in the `objs` directory:

```sh
 ./objs/emulator -list-avds
```

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
        --target: <windows|linux|darwin|mingw>: Which platform to target. This will attempt to cross compile if the target does not match the current platform (darwin)
            (default: 'darwin')
        --test_jobs: Specifies  the number of tests to run simultaneously
            (default: '8')
            (an integer)
        --[no]tests: Run all the tests
            (default: 'true')
```
### Incremental builds

The rebuild script does a complete clean build. You can use ninja to partial builds. The ninja build engine is part of our repository:

```sh
    export PATH=$PATH:$HOME/emu-master-dev/prebuilts/ninja/darwin-x86/
    ninja -C objs
```


### Speeding up builds with 'ccache'

It is highly recommended to install the 'ccache' build tool on your development
machine(s). The Android emulator build scripts will probe for it and use it
if available, which can speed up incremental builds considerably.

```sh
    brew install ccache
```
#### Optional step

Configure ccache to use a different cache size with `ccache -M <max size>`. You can see a list of configuration options by calling ccache alone. * The default ccache directory is ~/.ccache. You might want to symlink it to another directory (for example, when using FileVault for your home directory).

### Cross compiling to Windows with clang-cl

***It is highly recommended to use a windows machine for windows development, vs cross compilation.***

It is possible to cross compile from MacOs to windows. This is mainly useful to quickly discover compilation issues, as you will not be able to actually run the code.

The windows target requires you to install the MSVC libraries. These libraries need manual intervention to be installed on your linux machine as you will need to:

  1. Strongly consider not doing this, and develop on a windows machine.
  2. You will need a case insensitive filesystem (you likely have this). See this [post](https://apple.stackexchange.com/questions/71357/how-to-check-if-my-hd-is-case-sensitive-or-not) to verify if yours is.
  3. You will need the mingw toolchain (we use some program tools from mingw). The easiest way is to install it using [homebrew](https://brew.sh/):

        brew install mingw-w64

#### I'm within google

   - You will have to obtain the windows sdk

```sh
export PATH=$PATH:$PWD/android/third_party/chromium/depot_tools/
download_from_google_storage --config
python ./android/third_party/chromium/depot_tools/win_toolchain/get_toolchain_if_necessary.py --toolchain-dir=$AOSP_DIR/prebuilts/android-emulator-build/msvc --force --output-json=res.json 3bc0ec615cf20ee342f3bc29bc991b5ad66d8d2c
ln -sf $AOSP_DIR/prebuilts/android-emulator-build/msvc/vs_files/3bc0ec615cf20ee342f3bc29bc991b5ad66d8d2c $AOSP_DIR/prebuilts/android-emulator-build/msvc/win8sdk
```

next you will need to have a mingw compiler and realpath. The easiest way to obtain these is through homebrew:

```sh
brew install mingw-w64 coreutils
```

If all went well you can now target windows as follows:

    ./android/rebuild.sh --target windows

#### I'm outside Google

  1. You will need to have access to a Windows machine.
      - Install [Python](https://www.python.org/downloads/windows/)
      - Install [Chrome Depot Tools](https://dev.chromium.org/developers/how-tos/depottools)
      - Install [Visual Studio](https://visualstudio.microsoft.com/)

  2. Package your Windows SDK installation into a zip file by running the following on a Windows machine:

```sh
    cd path/to/depot_tools/win_toolchain
    # customize the Windows SDK version numbers
    python package_from_installed.py 2017 -w 10.0.17134.0
```

    copy the created zip file to your mac. Say win8sdk.zip

  3. Execute the following on your mac:

```sh
    mkdir -p /mnt/msvc/win8sdk
    unzip win8sdk.zip -D /mnt/msvc/win8sdk
```

If all went well you can now target windows as follows:

    ./android/rebuild.sh --target windows


### Sending patches

Here you can find details on [submitting patches](
https://gerrit.googlesource.com/git-repo/+/refs/heads/master/SUBMITTING_PATCHES.md). No external configuation should be needed if you are within Google.
