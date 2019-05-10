Emulator Video Bridge
=====================

The emulator video bridge is a separate executable that is capable of managing a set of WebRTC connections.

## Why is the Video Bridge a seperate executable?

The video bridge is a seperate executable that relies on the WebRTC libraries
that come from chrome. This library is build using GN, with its own toolchain
and dependencies. These dependencies (libyuv, ssl, etc) are incompatible with
the emulator and its build system.

To minimize the depedencies we build the bridge seperately and communciate
through sockets and shared memory:

    +---------------------------+               +-------------------------+
    |                           |    SOCKET     |                         |
    |       EMULATOR            +<------------->+       VIDEOBRIDGE       |
    |                           +-------------->+                         |
    +---------------------------+   Shared      +-------------------------+
                                    Memory


The shared memory region is used to transfer video, and the socket is used to
drive the [JSEP protocol](https://rtcweb-wg.github.io/jsep/).


## Configuring TURN

Traversal Uing Relays around NAT (TURN) is a protocol that assists in traversal
of network address translators (NAT) or firewalls for multimedia applications.
It may be used with the Transmission Control Protocol (TCP) and User Datagram
Protocol (UDP). It is most useful for clients on networks masqueraded by
symmetric NAT devices. TURN does not aid in running servers on well known ports
in the private network through a NAT; it supports the connection of a user
behind a NAT to only a single peer, as in telephony, for example.

Turn makes it possible to connect to the emulator over WebRTC, even if the
network is closed down and no direct peer connection can be established. It
does this by relaying the packets through a server that acts as a middle man.
Configuration of turn is done in the video bridge by invoking an external
program that provides the turn configuration.

For example in GCE you can enable the turn api (http://go/turnaas) and pass in
the following curl command to obtain a turn configuration:

```sh
$ curl -s -X POST https://networktraversal.googleapis.com/v1alpha/iceconfig?key=some_secret
```

This will produce a configuration that the bridge will distribute to the
clients to configure the peer connection. Depending on your network
configuration you might need to provide your own executable that provides this
information. Your executable should observe the following rules:

 - Produce a result on stdout.
 - Produces a result in under 1000 ms.
 - Produce a valid [JSON RTCConfiguration object](https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/RTCPeerConnection).
 - Contain at least an `"iceServers"` array.
 - The exit value should be 0 on success

An example result by using curl:

```json
{
  "lifetimeDuration": "86400s",
  "iceServers": [
    {
      "urls": [
        "stun:171.194.202.127:19302",
        "stun:[2607:f8b0:400f:c00::7f]:19302"
      ]
    },
    {
      "urls": [
        "turn:171.194.202.127:19305?transport=udp",
        "turn:[2607:f8b0:400f:c00::7f]:19305?transport=udp",
        "turn:171.194.202.127:19305?transport=tcp",
        "turn:[2607:f8b0:400f:c00::7f]:19305?transport=tcp"
      ],
      "username": "<removed>",
      "credential": "<removed>",
      "maxRateKbps": "8000"
    }
  ],
  "blockStatus": "NOT_BLOCKED",
  "iceTransportPolicy": "all"
}
```

The executable and its parameters can be directly provided to the emulator with
the `-turncfg` parameter.

For example:

```sh
emulator @P_64 -grpc 5556 -turncfg "curl -s -X POST https://networktraversal.googleapis.com/v1alpha/iceconfig?key=secret"
```

Would use the GCE Turn API.




