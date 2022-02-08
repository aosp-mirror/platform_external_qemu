Emulator over gRPC
==================

The emulator can be configured with to host a set of gRPC services. The services are defined in a set
of protobuf files that are shipped with the emulator. The .proto definitions can be found in the
lib directory of the emulator. For example:

```sh
 $ ls $ANDROID_SDK_ROOT/emulator/lib/*proto
/Users/jansene/Library/Android/sdk/emulator/lib/emulator_controller.proto   /Users/jansene/Library/Android/sdk/emulator/lib/snapshot_service.proto      /Users/jansene/Library/Android/sdk/emulator/lib/waterfall.proto
/Users/jansene/Library/Android/sdk/emulator/lib/snapshot.proto              /Users/jansene/Library/Android/sdk/emulator/lib/ui_controller_service.proto
```

# gRPC security

The emulator has several mechanisms in place to secure the gRPC endpoints. The security of the endpoint is
configured by a series of flags. In general you can protect the emulator as follows:

- The transport layer: This involves securing the TCP connections.
- The individual paths: This involes securing who can access which endpoint. I.e. who can invoke a particular method.

## Securing the transport layer.

The emulator can run the gRPC service in the folllowing modes:

- Insecure on all ports: You can access the gRPC endpoint on every port without TLS. You can enable this by passing the `-grpc` flag.
- Insecure on loopback: You can access the gRPC endpoint on the loopback device. This is the default mode of the gRPC service.
- TLS on all ports: You can access the gRPC endpoint with tls enabled.  You must use the `-grpc-tls-key` and `-grpc-tls-cer` flag to provide the proper PEM and X.509 cert to the emulator.
- TLS with mutual authentication: Both sides will have to authenticate themselves. You must use the `-grpc-tls-key` and `-grpc-tls-cer` flags as well as the `-grpc-tls-ca` flag to provide a series of certificate authorities for client validation.


## Securing the endpoints.

Currently the emulator has support for the following modes:

- No securing of endpoints. Use the `-grpc` flag.

- *deprecated* Securing with `-grpc-use-token` flag.

  Require an authorization header with a valid token for every grpc call.

  Every grpc request expects the following header:
    authorization: Bearer <token>

- Securing with the `-help-grpc-use-jwt` flag. This flag will become the default unless `-grpc` flag is present.

 Every grpc request expects the following header:

    authorization: Bearer <token>

  Where token is a valid signed JSON Web Token (JWT)

  If an incorrect token is present the status UNAUTHORIZED will be returned.

  In order to make a succesful call a JWT token must be presented with the following fields:

  - aud: must contain a list of allowed methods.
  - iat: must have an issue in the past.
  - exp: must have an expiration date.

  In order to validate tokens the emulator will load all JSON Web Key (JWK) files from
  a directory that is only accessible by the user who launched the emulator.
  The location of this directory is defined in the emulator discovery file under the key grpc.jwks
  Placing a valid JWK file with the name ${grpc.jwks}/my_filename.jwk will result in the JWK
  being made available to the emulator. Note that `my_filename` can be an arbitrary file name.

  The emulator will merge all discovered keys and write them to the file in the JWK format. The file
  is defined in the emulator discovery file under the key grpc.jwk_active.

  Developers can use this to make sure that the key they generated has been picked up by the emulator.

  The location of the discovery file can be found in the directory is $HOME/Library/Caches/TemporaryItems/avd/running
  The file will be named `pid_%d_info.ini` where %d is the process id of the emulator.

  Notes:
   - Tokens must be signed with one of the following algorithms: ES256, ES384, ES512, RS256, RS384, RS512
   - Token based security can only be installed if you are using localhost or TLS.


# Examples

Most of the samples have moved over to the public container repository that you can find [here](https://github.com/google/android-emulator-container-scripts).

The samples are well documented and run against the publicly released emulator binaries.

## Controlling the emulator with Python

In the python directory you will find a few examples. The examples require Python3, as Python2 is now [deprecated](https://www.python.org/doc/sunset-python-2/). You can build the examples by running:

```sh
  $ make deps
```

**Note:** This installs the pillow package. If you see failures you might need
to install tkInter. (See details in the Screenshot section).

Next you need to launch an emulator with the gRPC port enabled on port 8554:

```sh
  $ $AOSP/external/qemu/objs/emulator @my_avd -grpc 8554 [..other options...]
```

Now you can run any of the samples under the sample directory as follows:

```sh
  $ python3 -m sample.name_of_sample
```

Where name of sample can be any of the python modules in the sample directory (`ls -1 sample/*.py`). For example:

```sh
  $ python3 -m sample.keystrokes
```

### Screenshot

The screenshot example requires tkInter. You can find details on installing tkInter [here](https://tkdocs.com/tutorial/install.html).

Or try:
- `sudo apt-get install python3-tk` on linux
- `brew install python3` on macos (has tkinter)
