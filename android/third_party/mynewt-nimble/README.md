## Overview

Apache NimBLE is an open-source Bluetooth 5.1 stack (both Host & Controller)
that completely replaces the proprietary SoftDevice on Nordic chipsets. It is
part of [Apache Mynewt project](https://github.com/apache/mynewt-core).


This is a port of NimBLE v 1.4.0 taken at  250cd70c.

It contains the following series of changes:

- It adds a CMake project file. The project can be build independently, or as part of the android emulator.
   - To build it independently:

   $

- It adds an OS abstraction so it can interact with root canal an emulated bluetooth chip.
- Only one transport layer is available, a socket that connects to port 6402 on the local machine. It currently
  expects root canal to be running on this port.
- A set of sample apps can be found in the apps directory, they will all compile as nimble_xx


This is currently under active development!

