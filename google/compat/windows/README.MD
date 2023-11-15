# AOSP Toolchain Compatibility Layer for Windows

This directory provides header files and a compatibility library to ensure QEMU compilation using the AOSP clang-cl toolchain.

Though clang-cl accepts clang like compiler flags, it does not provide a set of posix compliant headers (like mingw does). In this directory you will find header files with constant definitions and functions that are used by QEMU. For some functions an implementations is provided.

Note that the current functionality is not (yet) utf-8 compliant so you will likely encounter issues when using unicode paths.


## Package Details

Some of the code used here is lifted from other packages:

- Dirent: This implementation is lifted from mingw runtime, the source has been taking from the AOSP version of [glib](https://android.googlesource.com/platform/external/bluetooth/glib/+/refs/heads/emu-dev/glib/dirent/).
- Getopt: The implementation we use was taken from netbsd, with the following license:

```
 * Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
```

- We have a series of missing compiler directives that we have brought in from llvm, which are release under the Apache-2.0 license with an llvm exception. The exact license can be obtained [here](https://llvm.org/LICENSE.txt)
