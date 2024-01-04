# QEMU with Android Extensions

This repository contains a modified version of QEMU, the open-source machine emulator, specifically optimized for Android emulation. It includes:

- Base QEMU: The core QEMU functionality.
- Android specific drivers: Drivers for various hardware components needed to run Android applications, such as graphics, sound, and networking.

- Platform-specific modifications: Changes to the code to ensure successful compilation on supported platforms:
  - Linux x64
  - Mac M1
  - Windows x64

This repository allows you to build QEMU using the compiler toolchain available in the Android Open Source Project (AOSP).

## Getting started

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

### Launching Android emulation

- The built QEMU binaries are located in the build directory.
- You need to launch QEMU with specific command-line arguments to configure the emulated hardware.
- Sample launch scripts are available in the [google/experiments](google/experiments)  directory. You will need to modify them based on your needs.

## Making changes to the code base

### Upstream first

When making changes that are not specific to Google's implementation, consider upstreaming them to the official QEMU repository. This involves a two-step process:

- First, submit your changes to the internal mimik branch (go/qemu). This is the branch Google uses to prepare patches for upstreaming.
- Once integrated into the mimik branch, the changes will automatically flow back to dependent repositories within Google. Additionally, follow up with the mimik team to make sure that  attempts will be made to upstream the changes to the official QEMU repository.

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

### Android changes should live in google

All Android-specific code resides in the [google](google) subdirectory. This includes goldfish drivers, Android-specific CPU boards, scripts, compatibility layers, and toolchain configurators.

By following these guidelines, wec can ensure clean and consistent contributions to the codebase, facilitating efficient integration and future maintenance.
