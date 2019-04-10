Emulator over gRPC  Examples
============================


#### Caveats: Experimental feature!!

This is currently an experimental feature, and as such there are some things lacking that might be important if you want to use this in production:

- There is no authentication/authorization. The gRPC server does not do any authentication nor authorization. Anyone who has access to the gRPC control port can control your emulator
- No TLS support. Currently the gRPC service inside the emulato has not turned on TLS.
- Not all emulator control functions have been enabled. This likely
 means that the .proto file describing the interface will evolve.

In other words, make sure you keep this port private on your network and be ready to recompile your code when changes happen.

### Introduction

This document describes how you can control the emulator over the gRPC interface. To learn more about gRPC you can visit the [website](https://grpc.io).

This document describes two sample cases. One where we will use the GO language to create a webserver that allows you to control the emulator over http.

The second example demonstrates how you can use python to control the emulator.

Note that this is still under active development, so more functionality might be added in the future.

# Enabling gRPC in the build

You must enable gRPC in the build, either by editing CMakeLists.txt and setting GRPC to TRUE or by building as follows:

```sh
   ./android/rebuild --cmake_option GRPC=TRUE
```

This will compile in the gRPC feature.

# Controlling the emulator with http.

In this example we will construct an intermediate server that acts as a webserver. The webserver will enable an API that is forwarded to the emulator.

In order to run this sample you will need to have a working Go environment. You can find a Go distribution and learn more about Go [here](https://golang.org)

*Note: The example requires you to have the ports 5556 & 8080 available*

## Installation
First you need to install ProtocolBuffers 3.0.0-beta-3 or later.  The installation depends on your OS.


### I'm using homebrew (Mac/Linux)

Your best bet is to use [homebrew](https://brew.sh/) and use the following

```sh
  $ brew install protobuf
```

### I'm using linux

If you have

```sh
$ mkdir tmp && cd tmp
$ git clone https://github.com/google/protobuf && cd protobuf
$ ./autogen.sh && ./configure
$ make  check
$ sudo make install
```

### Launch the emulator and webserver

*Note: Make sure you have a proper $GOPATH!*

Before we can access the emulator we must launch it with the gRPC server enabled.

```sh
  $ emulator @my_avd -grpc 5556 [..other options...]
```

Once the emulator is launched you can build and launch the web service as follows:

```sh
  $ cd $AOSP/external/qemu/android/android-grpc/docs/grpc-samples/go && make deps && make run
```

Now you should be able to access the emulator over http on port 8080. For example:
[http://localhost:8080/v1/gps](http://localhost:8080/v1/gps) will show you the current gps settings.

The following example will send a keycode to the emulator:

```sh
curl -H "Accept: application/json" -X POST -d '{"key": "30"}' http://localhost:8080/v1/key
```

For more details on the endpoints look at $AOSP/android/android-grpc/android/emulation/control/api_config.yaml

### Troubleshooting:

- I see the following json:

  ```json
    {"error":"all SubConns are in TransientFailure, latest connection error: connection error: desc = \"transport: Error while dialing dial tcp [::1]:5556: connect: connection refused\"","message":"all SubConns are in TransientFailure, latest connection error: connection error: desc = \"transport: Error while dialing dial tcp [::1]:5556: connect: connection refused\"","code":14}
  ```

  You likely restarted the emulator, restart the httpbridge.

- Why are screenshots slow in debug builds?

  The emulator enables a lot of checks (address sanitizers, code coverage) that take up a lot of resources. You might be better of using a release build.

 # Controlling the emulator from Python (TODO)

gRPC Python is supported for use with Python 2.7 or Python 3.4 or higher. First we will need some required packages to work with gRPC:

```sh
$ python -m pip install grpcio grpcio-tools
```

```
python -m grpc_tools.protoc -I../../protos --python_out=. --grpc_python_out=. ../../protos/helloworld.proto
```