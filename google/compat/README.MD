# AOSP Toolchain Compatibility Layer

This directory provides header files and a compatibility library to ensure QEMU compilation using the AOSP clang toolchain.

The AOSP clang toolchain uses an older sysroot lacking essential definitions for successful compilation. These header files and libraries address this by defining missing constants.

The toolchain generator utilizes this layer to set appropriate -isystem paths and linker flags.

Note that most of this code originates from the "CRASHPAD" compatibility layer, available here:
<https://chromium.googlesource.com/crashpad/crashpad/+/refs/heads/main/compat/>

## Package details

Name: Crashpad
Short Name: crashpad
URL: <https://crashpad.chromium.org/>
Version: unknown
Revision: 9158eb7caa811b684c6bec7f9e9b5f52389d0ce0
License: Apache 2.0
License File: LICENSE

Description:
Crashpad is a crash-reporting system. It’s the successor to Breakpad.

Crashpad’s authoritative upstream repository is
<https://chromium.googlesource.com/crashpad/crashpad>.

This only contains a subset of the compat directory which contains a
set of compatibility layers.
