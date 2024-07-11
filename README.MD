# QEMU with Android Extensions

This repository contains a modified version of QEMU, tailored for Android emulation, along with instructions for building it using the [Android Open Source Project (AOSP) toolchain].

## Contents

- **Base QEMU:** The core QEMU functionality.
- **Low-level QEMU Android drivers:** Components such as ACPI tables and motherboard configurations that are initialized early in the QEMU configuration process.
- **Android-specific drivers:** Drivers for various hardware components (graphics, sound, networking) needed to run Android applications. These drivers are located in `//hardware/generic/goldfish/...` as plugins loaded during QEMU driver initialization.
- **Platform-specific modifications:** Changes to the codebase to ensure successful compilation on supported platforms:
  - Linux x64
  - Mac M1
  - Windows x64

## Building QEMU

This repository enables you to build QEMU using the AOSP compiler toolchain.

The Android emulator is a series of plugins that can be loaded into QEMU. Run the Android emulator by providing the correct parameters to QEMU.

While stock QEMU is built using `meson`, the Android emulator is built using `bazel`. Bazel build files are generated using an automated script that must be run on every platform of interest. After that, a merge script can be run to generate the final `bazel` build files. You should never manually edit any of the `BUILD.bazel` files.

## Adding Generic Drivers

To add generic drivers for the wider QEMU community, make the modifications to the `meson` build scripts and ensure they compile on all platforms of interest. Please try to upstream these drivers, if possible, to make them available to the wider community and reduce maintenance costs.

Once your modifications are successful, you need to generate the `bazel` build files. See the [README.md](google/scripts/fetch_bazel/README.md) document for details on how to create the `bazel` build files from the `meson` build files.

## Building stock QEMU

To get started you will need to configure the toolchain this can be done by running the following:

1. Create a python virtual environment
2. pip install google/toolchain
3. Run the `amc` command to configure the toolchain.

For Mac/Linux:

```sh
python -m venv .venv
source .venv/bin/activate
pip install google/toolchain
amc setup build
amc compile build
```

For Windows:

```sh
python -m venv .venv
call .venv\Scripts\activate.bat
pip install google/toolchain
amc setup build
amc compile build
```

Once this is finished you should be able to run the qemu binaries in the build directory.

## Making changes to the QEMU code base

### Upstream first

When making changes that are not specific to Google's implementation, consider upstreaming them to the official QEMU repository. This involves a two-step process:

- First, submit your changes to the internal mimik branch (go/qemu). This is the branch Google uses to prepare patches for upstreaming.
- Once integrated into the mimik branch, the changes will automatically flow back to dependent repositories within Google. Additionally, follow up with the mimik team to make sure that attempts will be made to upstream the changes to the official QEMU repository.

### Minimize impact on base QEMU changes

This repository received regulary updates from the `remotes/aosp/mirror-qemu-internal-staging-v8.1.0` branch. To minimize conflicts during merging updates, we adhere to the following principles:

- *Build System:* We adopt the QEMU meson build system for consistency.
- *Dependencies:* Do not introduce new high-level dependencies not already used by upstream QEMU, such as protobuf, gRPC, or WebRTC.
- *Minimal changes:* Minimize modifications to the original QEMU source code. Ideally, changes should be upstreamed first.
- *Android-specific code*: All code specific to Android and not intended for upstreaming should reside within the google directory.
- *Android-specific changes:* Use `#ifdef CONFIG_ANDROID` ... `#endif` blocks to isolate code specific to Android, making merges easier and enabling builds of vanilla QEMU by disabling the CONFIG_ANDROID feature. For example:

```c
#ifdef CONFIG_ANDROID
// My changes
int x = 0;
x = x + 1;
#endif
```

Note: Try to make drivers to live as plugins in //hardware/generic/goldfish

### Android changes that cannot be upstreamed should live in google

All Android-specific code resides in the [google](google) subdirectory. This includes goldfish drivers, Android-specific CPU boards, scripts, compatibility layers, and toolchain configurators.

By following these guidelines, wec can ensure clean and consistent contributions to the codebase, facilitating efficient integration and future maintenance.

## Building the Android Emulator release

The android emulator consists of a series of plugins that can be loaded into stock qemu. The android emulator itself is built using bazel. You can create a release by running

```sh
bazel build //hardware/generic/goldfish/emulator:release
```

This will produce a zip file for your current architecture. You can unzip this file and launch the emulator. For example on darwin it will produce the following:

```sh
bazel build //hardware/generic/goldfish/emulator:release
INFO: Analyzed target //hardware/generic/goldfish/emulator:release (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //hardware/generic/goldfish/emulator:release up-to-date:
  bazel-bin/hardware/generic/goldfish/emulator/sdk-repo-darwin_aarch64-emulator-developer.zip
INFO: Elapsed time: 0.258s, Critical Path: 0.00s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
```

You can run the qemu binaries directly, but you will have

### Running a development build

You can also run the emulator directly from within bazel:

```sh
bazel run //hardware/generic/goldfish/emulator/launcher:goldfish -- --avd V --vnc :8
```

This will launch the goldfish emulator with avd *V* and creates a vnc connection on port 5908. You can now attach your favorite debugger to debug any issues.

Note that on linux you will have to set the following source mapping:

```sh
$ lldb
(lldb) settings set target.source-map /proc/self/cwd /my/bazel/workspace/root
```

Where `/my/bazel/workspace/root` points to your bazel workspace root.

## Adding emulator functionality

All the code related to emulator can be found in //hardware/generic/goldfish. See the [//hardware/generic/goldfish/README.MD](../../hardware/generic/goldfish//README.MD) for more information.