Include What You Use
====================
The files here are some scripts and instructions to use Include What You Use [(iwyu)](https://github.com/include-what-you-use/include-what-you-use/blob/master/README.md)


# Overview

"Include what you use" means this: for every symbol (type, function, variable, or macro) that you use in `foo.cc` (or `foo.cpp`), either `foo.cc` or `foo.h` should include a .h file that exports the declaration of that symbol. (Similarly, for `foo_test.cc`, either `foo_test.cc` or `foo.h` should do the including.)  Obviously symbols defined in `foo.cc` itself are excluded from this requirement.

This puts us in a state where every file includes the headers it needs to declare the symbols that it uses.  When every file includes what it uses, then it is possible to edit any file and remove unused headers, without fear of accidentally breaking the upwards dependencies of that file.  It also becomes easy to automatically track and update dependencies in the source code.

# Get started on Linux

In this exciting process you will:

1. Build your own clang release that matches the emulator toolchain
2. Compile iwyu from scratch, based on your own clang compiler.
3. Learn how to install it and finally
4. Run the tool to optimize all your includes.

First you will have to make sure you have a local build of clang as described [here](https://android.googlesource.com/toolchain/llvm_android/+/master/README.md)

```sh
$ mkdir llvm-toolchain && cd llvm-toolchain
$ export LLVM_ROOT=$PWD/out/stage2-install
$ repo init -u https://android.googlesource.com/platform/manifest -b llvm-toolchain
$ repo sync -c
$ python toolchain/llvm_android/build.py --no-lto --no-build windows
```

Do not forget the **--no-lto flag**, or you will not be able to use your build

Get some coffee! The build will take some time (1 hour or so).

Next you will need a local copy if [iwyu](https://github.com/include-what-you-use/include-what-you-use/)
Make sure you check out the branch that corresponds to the clang version you compiled above. When this document was written it was version 9, which is oddly enough is the clang_8.0 branch,.

```sh
$ git clone https://github.com/include-what-you-use/include-what-you-use.git
  cd include-what-you-use  &&
  git checkout clang_8.0  &&
  mkdir build && cd build &&
  cmake -E env CXXFLAGS="-stdlib=libc++" CC=$LLVM_ROOT/bin/clang CXX=$LLVM_ROOT/bin/clang++
  cmake -G Ninja -DClang_DIR=$LLVM_ROOT/out/stage2-install/lib64/cmake/clang/ -DLLVM_DIR=$LLVM_ROOT/out/stage2-install/./lib64/cmake/llvm
  ninja

```

You should end up with an executable in `bin/include-what-you-use`. Place this somewhere on your path.

Next we must make sure that iwyu can find all the read headers. So let's setup the proper symlink:

```sh
  export LNDIR=$(dirname $(dirname $(include-what-you-use -print-resource-dir 2>/dev/null)))
  mkdir -p $LNDIR
  ln -sf $LLVM_ROOT/lib64/clang $LNDIR/clang
```

**Congratulations!** You've managed to create a working install of iwyu!

# Get started on Mac

The easiest way is to install the [include-what-you-use](https://formulae.brew.sh/formula/include-what-you-use)
 homebrew formulate.

```sh
brew install  include-what-you-use
```

# Using iwyu

First you will need to compile our source tree such that a compilation database is created. This can be done as follows:

```sh
$ android/rebuild.sh --cmake_option CMAKE_EXPORT_COMPILE_COMMANDS=ON
```


Include what you use will aggressively rewrite your code base. This basically happens in 2 phases:

- Analyze phase, where a yaml file with suggested fixes is generated:

  ```sh
  $ ./android/scripts/unix/iwyu.sh ".*qemu/android.*Optional_unittest.cpp" > sample.yaml
  ```

This will produce a file like this:

  ```yaml
warning: argument unused during compilation: '-fuse-ld=/usr/local/google/home/jansene/src/emu-master-dev/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.17-4.8/bin/x86_64-linux-ld'
warning: argument unused during compilation: '-fuse-ld=/usr/local/google/home/jansene/src/emu-master-dev/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.17-4.8/bin/x86_64-linux-ld'

android/android-emu/aemu/base/Optional.h should add these lines:
#include <new>                        // for operator new

android/android-emu/aemu/base/Optional.h should remove these lines:
- #include <cassert>  // lines 17-17
- #include <utility>  // lines 20-20

The full include-list for android/android-emu/aemu/base/Optional.h:
#include <cstddef>                    // for size_t
#include <initializer_list>           // for initializer_list
#include <new>                        // for operator new
#include <type_traits>                // for move, decay, forward, is_constr...
#include "aemu/base/Log.h"         // for CHECK, LogMessage
#include "aemu/base/TypeTraits.h"  // for enable_if_convertible, enable_if_c
namespace android { namespace base { template <class T> class Optional; } }  // lines 143-144
---

/usr/local/google/home/jansene/src/emu-master-dev/external/qemu/android/android-emu/aemu/base/Optional_unittest.cpp should add these lines:
#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult
#include <stdint.h>                 // for int64_t, int16_t, int32_t
#include "__functional_base"        // for reference_wrapper
#include "gtest/gtest_pred_impl.h"  // for AssertionResult, Test, EXPECT_EQ
#include "initializer_list"         // for initializer_list
#include "iosfwd"                   // for string
#include "new"                      // for operator new
#include "string"                   // for basic_string
#include "type_traits"              // for move, integral_constant<>::value

/usr/local/google/home/jansene/src/emu-master-dev/external/qemu/android/android-emu/aemu/base/Optional_unittest.cpp should remove these lines:
- #include <gtest/gtest.h>  // lines 15-15

The full include-list for /usr/local/google/home/jansene/src/emu-master-dev/external/qemu/android/android-emu/aemu/base/Optional_unittest.cpp:
#include "aemu/base/Optional.h"
#include <gtest/gtest-message.h>    // for Message
#include <gtest/gtest-test-part.h>  // for TestPartResult
#include <stdint.h>                 // for int64_t, int16_t, int32_t
#include <memory>                   // for unique_ptr
#include <vector>                   // for vector
#include "__functional_base"        // for reference_wrapper
#include "gtest/gtest_pred_impl.h"  // for AssertionResult, Test, EXPECT_EQ
#include "initializer_list"         // for initializer_list
#include "iosfwd"                   // for string
#include "new"                      // for operator new
#include "string"                   // for basic_string
#include "type_traits"              // for move, integral_constant<>::value
---
  ```


- Application phase, where the fixes can be automatically applied.

  ```sh
  $ ./android/scripts/unix/iwyu.sh < sample.yaml
  ```

  If you wish to have more control over how changes should be applied you can invoke the fix script directly:

  ```sh
  $ ./android/scripts/iwyu/fix_includes.py --help
  ```

# Beware of the code change!

There are some issues that you have to be well aware of when trying to do this in the emulator code base:

- ***NEVER EVER REWRITE DOWNSTREAM PROJECTS***

  Rewriting downstream projects means that merging down changes from upstream is going to be very, very challenging. The best way forward is to apply the changes upstream, and merge it back downstream.

- *It is not perfect*:

  There are several things that do not work very well:

  - It does not handle our platform specific code very well, for example: `#ifdef _WIN32` blocks can
    get rewritten with unexpected results. It is best to not automatically translate these files.

  - We use internal `#include` statement at places. Again, these get optimized away, breaking your build.
    It is best to exclude these files from automatic processing.

  - Make sure to update the .imp files which contain a set of rewrite rules that are specific to our use case.

  - Be careful with header rewrites! If you are planning to rewrite headers make sure you take the following steps:

    1. Optimize every `c/.cpp` first. It could very well be that your header might no longer bring in things that
       your files rely on.

    2. Optimize with --safe-headers first.

  - Make use of the `iwyu-apply.py` script. This script will try to find which files from a given commit can be included
    in the current revision. For example say you have applied fixes in commit foo you can run it as follows:

    ```sh
    $ pip install gitpython absl-py
    $ repo start clean-branch
    $ ./android/rebuild --target windows --out build-msvc
    $ ./android/rebuild --target mingw --out build-mingw
    $ ./android/rebuild
    $ python ./android/scripts/iwyu/iwyu_apply.py --rev foo
    ```

    The script will bisect against the revision and you will end up with a set of files that will not break the build.



