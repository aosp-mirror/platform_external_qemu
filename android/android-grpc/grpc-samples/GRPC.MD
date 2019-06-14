Emulator over gRPC  Examples
============================


#### Caveats: Experimental feature!!

This is currently an experimental feature, and as such there are some things lacking that might be important if you want to use this in production:

- There is no authentication/authorization. The gRPC server does not do any authentication nor authorization. Anyone who has access to the gRPC control port can control your emulator
- No TLS support. Currently the gRPC service inside the emulato has not turned on TLS.
- Not all emulator control functions have been enabled. This likely
 means that the .proto file describing the interface will evolve.

In other words, make sure you keep this port private on your network and be ready to recompile your code when changes happen.

**DO NOT RUN THIS AS IS IN A PRODUCTION ENVIRONMENT**

### Introduction

This document describes how you can control the emulator over the gRPC
interface. To learn more about gRPC you can visit the
[website](https://grpc.io).

We describe several sample cases.

 - One where we will use the GO language to create a webserver
   that allows you to control the emulator over http.

 - An example demonstrates how you can use Javascript & React to control
   the emulator.

  - An example that uses the python bindings.


Note that this is still under active development, so more functionality might
be added in the future.

In general all the approaches work in a similar fashion:

1. Create the grpc bindings using protoc
2. Place the generated bindings in the right location.
3. Configure the EmulatorService, (i.e. set port, authentication etc.)
4. Make calls to the emulator.

# Enabling gRPC in the build

You must enable gRPC in the build, either by editing CMakeLists.txt and setting
GRPC to TRUE or by building as follows:

```sh
   ./android/rebuild --cmake_option GRPC=TRUE
```

This will compile in the gRPC feature.

# Controlling the emulator with http.

In this example we will construct an intermediate server that acts as
a webserver. The webserver will enable an API that is forwarded to the
emulator.

In order to run this sample you will need to have a working Go environment. You
can find a Go distribution and learn more about Go [here](https://golang.org)

*Note: The example requires you to have the ports 5556 & 8080 available*

## Installation
First you need to install ProtocolBuffers 3.0.0-beta-3 or later.  The installation depends on your OS.


### I'm using homebrew (Mac/Linux)

Your best bet is to use [homebrew](https://brew.sh/) and use the following

```sh
  $ brew install protobuf
```

### I'm using linux

You must make sure you have a recent version of protobuf available:

```sh
$ mkdir tmp && cd tmp
$ git clone https://github.com/google/protobuf && cd protobuf
$ ./autogen.sh && ./configure
$ make  check
$ sudo make install
```

### Launch the emulator and webserver


Before we can access the emulator we must launch it with the gRPC server
enabled.

```sh
  $ emulator @my_avd -grpc 5556 [..other options...]
```

Once the emulator is launched you can build and launch the web service as
follows:

```sh
  $ cd $AOSP/external/qemu/android/android-grpc/docs/grpc-samples/go && make deps && make run
```

Now you should be able to access the emulator over http on port 8080. For
example: [http://localhost:8080/v1/gps](http://localhost:8080/v1/gps) will show
you the current gps settings.

The following example will send a keycode to the emulator:

```sh
curl -H "Accept: application/json" -X POST -d '{"key": "30"}' http://localhost:8080/v1/key
```

For more details on the endpoints look at
$AOSP/android/android-grpc/android/emulation/control/api_config.yaml

### Troubleshooting/Known Issues:

- I see TypeError: ...

  Our Makefiles are very simplisitic and are not detecting changes to the .proto files. You might have to
  run ```make clean``` before trying again.

- I see the following json:

  ```json
    {"error":"all SubConns are in TransientFailure, latest connection error: connection error: desc = \"transport: Error while dialing dial tcp [::1]:5556: connect: connection refused\"","message":"all SubConns are in TransientFailure, latest connection error: connection error: desc = \"transport: Error while dialing dial tcp [::1]:5556: connect: connection refused\"","code":14}
  ```

  You likely restarted the emulator, restart the httpbridge.

- Why are screenshots slow in debug builds?

  The emulator enables a lot of checks (address sanitizers, code coverage) that
  take up a lot of resources. You might be better of using a release build.


# Controlling the emulator from JavaScript with React

The javascript example showcases how you can interact with the emulator by
displaying the current screeenshot, and sending mouse events to the emulator.
The sample consists of two react components:

- `Emulator` -> Responsible for sending mouse clicks, and hosting a view
- `EmulatorPngView` -> Responsible for displaying a view of the emulator. (Very slow)

In the future we hope to replace the `EmulatorPngView` with something like an
`EmulatorWebRTCView` that has significantly better performance characteristics.

In order to control the emulator from Javascript we will have to route the
calls through an intermediate proxy. We will use
[gRPC-web](https://github.com/grpc/grpc-web) to make it all possible.

## Requirements

In order to run the demo you will need Node, dep and a protoc plugin to
generate the javascript classes.

Your best bet is to use [homebrew](https://brew.sh/) and use the following

```sh
  $ brew install dep npm
```

or else follow the documentation here:
https://golang.github.io/dep/docs/installation.html

If you are planning to modify .proto files you willneed to acquire the
proper protoc plugin from [here](https://github.com/grpc/grpc-web/releases).
You can either build it yourself, or install the binary.


## Launch the emulator and website

In order to launch this demo you will need the following ports available:

- 5556 Emulator gRPC port
- 8080 gRPC JavaScript proxy
- 3000 npm developer port.

Before we can access the emulator we must launch it with the gRPC server enabled.

```sh
  $ emulator @my_avd -grpc 5556 [..other options...]
```

Once the emulator is launched you can build and launch the web application as follows:

```sh
  $ cd $AOSP/external/qemu/android/android-grpc/docs/grpc-samples/js  && make deps && make develop
```

*Note: Make deps is only needed once*

A browser should open and you should be able to interact with the emulator.

You can stop all the components by running:

```sh
  $ cd $AOSP/external/qemu/android/android-grpc/docs/grpc-samples/js  && make stop
```

*Note: this will just kill npm and the proxy*

## Launch the emulator in a release like environment with envoy and docker.

The recommended proxy for gRPC on the web is [Envoy](https://www.envoyproxy.io/). Envoy is an L7 proxy and communication bus designed for large modern service oriented architectures. It basically can handle a few things for us:

- Serve TLS.
- Add [External Authorization](https://www.envoyproxy.io/docs/envoy/latest/intro/arch_overview/ext_authz_filter)
- Add [JWT Authentication](https://www.envoyproxy.io/docs/envoy/latest/configuration/http_filters/jwt_authn_filter#config-http-filters-jwt-authn)
- Acts a s gRPC proxy
- Redirect http -> https
- Redirect serving of static pages to nginx.

This gets us closer to what is needed to deploy the emulator with gRPC into a production environment. A sample is added that uses docker to configure a simple production like environment with the following characteristics:

- An nginx container that will serve production build of the javascript example
- An envoy gRPC proxy
- A http -> https redirect
- A TLS endpoint using self signed certificates.

*Do not deploy this in a real production environment without adding some authorization filters, as anyone who can make an https connection will be able to control your emulator.*

# Controlling the emulator with Python

In the python directory you will find two examples:

- An example that aks the emulator about the vm configuration after which it
  will send a series of key events with a 2ms delay. After building you can run this with
  `python -m sample.keystrokes`

- An example that uses the streaming interface to retrieve the logcat. After
  building you can run this with `python -m sample.logcat`

Before we can access the emulator we must launch it with the gRPC server enabled.

```sh
  $ emulator @my_avd -grpc 5556 [..other options...]
```

Once the emulator is launched you can build the samples as follows:

```sh
  $ make deps
```

