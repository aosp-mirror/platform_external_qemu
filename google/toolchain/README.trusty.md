# Trusty Toolchain Configuration for QEMU Build System

This document elaborates on the purpose and functionalities of the tool used to configure the QEMU build system to produce static QEMU binaries used to test the Trusty TEE.

## Overview

This tool simplifies the process of building QEMU statically with all
Trusty-specific patches applied so the produced distributables can be used
to run and test the Trusty TEE in CI.

## Usage

```
$ ./build-qemu-trusty path/to/out path/to/dist.zip
```

This command will build QEMU in the `path/to/out` directory and produce a
distributable `path/to/dist.zip` file containing all the files needed to run
QEMU.

## Contact

For questions and comments, please contact trusty-team@google.com.
