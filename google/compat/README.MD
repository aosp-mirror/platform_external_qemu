# Compatibility Layers

In this directory, you'll discover a collection of compatibility layers essential for compiling QEMU on both Linux and Windows, leveraging the clang toolchain provided by AOSP.

The core concept revolves around configuring the toolchains to prioritize the headers defined here. During the linking step, the resulting executable should link against the compatibility library.

It's crucial to note that these layers only implement the necessary components required for compiling QEMU on the respective platforms.

## Linux

For Linux, this entails the introduction of a set of headers and libraries. This ensures that QEMU can successfully compile against an older sysroot available in AOSP. Details on the linux compat layer can be found in [linux/README.MD](linux/README.MD)

## Windows

On Windows, the compatibility layer introduces a set of POSIX headers utilized by QEMU, accompanied by an implementation. The implementation of `dirent` is sourced from the glib distribution, while `getopt` is borrowed from the Android emulator.
