Emulator over gRPC  Examples
============================

Most of the samples have moved over to the public container repository that you can find [here](https://github.com/google/android-emulator-container-scripts).

The samples are well documented and run against the publicly released emulator binaries.


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
  $ make deps && make protoc
```

Note that the samples are currently developed against Python2.