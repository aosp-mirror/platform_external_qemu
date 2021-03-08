Emulator over gRPC  Examples
============================

Most of the samples have moved over to the public container repository that you can find [here](https://github.com/google/android-emulator-container-scripts).

The samples are well documented and run against the publicly released emulator binaries.

# Controlling the emulator with Python

In the python directory you will find a few examples. The examples require Python3, as Python2 is now [deprecated](https://www.python.org/doc/sunset-python-2/). You can build the examples by running:

```sh
  $ make deps
```

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
