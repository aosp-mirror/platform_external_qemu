Emulator Video Bridge
=====================

The emulator video bridge is a separate executable that is capbale of managing a set of WebRTC connections.

## Why is the Video Bridge a seperate executable?

The video bridge is a seperate executable that relies on the WebRTC libraries that come from chrome. This library is build using GN, with its own toolchain and dependencies. These dependencies (libyuv, ssl, etc) are incompatible with the emulator and its build system.

To minimize the depedencies we build the bridge seperately and communciate through sockets and shared memory:

    +---------------------------+               +-------------------------+
    |                           |    SOCKET     |                         |
    |       EMULATOR            +<------------->+       VIDEOBRIDGE       |
    |                           +-------------->+                         |
    +---------------------------+   Shared      +-------------------------+
                                    Memory


The shared memory region is used to transfer video, and the socket is used to drive the [JSEP protocol](https://rtcweb-wg.github.io/jsep/).





