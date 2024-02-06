# AOSP Toolchain Configuration for QEMU Build System

This document elaborates on the purpose and functionalities of the tool used to configure the QEMU build system to utilize the AOSP toolchains.

## Overview

This tool aims to simplify the process of building QEMU using the pre-existing toolchains within AOSP. It supports targeting various platforms, including Windows x86, Linux x86, and Darwin-Apple.

## Operational principle

The core of this tool lies in creating toolchain wrappers for various essential utilities, such as:

- nm
- cmake
- bazel
- pkg-config
- cc
- c++
- strip
- windres

These wrappers are responsible for configuring individual calls to the AOSP-based clang compiler with appropriate parameters.

Additionally, the tool generates an aosp-cl.ini file that Meson can utilize to configure the relevant compilers and other build settings.

## Managing QEMU dependencies

Since the QEMU Meson build system relies on pkg-config for dependency discovery (e.g., zlib, glib), this tool addresses this by leveraging the existing AOSP packages built with Bazel. It extracts necessary information from these packages and generates corresponding pkg-config (.pc) files. These files enable the pkg-config script to resolve dependencies properly, pointing to the appropriate AOSP-based Bazel packages.
