# Android Emulator MacOS Development

This guide will help you set up and build the Android Emulator on your Mac for development.

## What You Need

- **Xcode:** Version 10.1 or newer. (Recommended: Xcode 13.4 Build version 13F17a with SDK version 12.3)
- **MacOS SDK:** Version 10.15 or later.

## Getting Xcode

1. **Download:** Get Xcode from the App Store.
2. **Install:** Open the downloaded file, move Xcode to your "Applications" folder.
3. **Accept License & Install Tools:**

    ```bash
    sudo xcodebuild -license accept && sudo xcode-select --install
    ```

4. **Verify Installation:**

    ```bash
    xcodebuild -version
    ```

## Getting the Source Code

1. **Install Repo:**

    ```bash
    mkdir $HOME/bin
    curl http://commondatastorage.googleapis.com/git-repo-downloads/repo > $HOME/bin/repo
    chmod 700 $HOME/bin/repo
    export PATH=$PATH:$HOME/bin
    ```

    Add `export PATH=$PATH:$HOME/bin` to your `.bashrc` or `.zshrc` file.
2. **Initialize Repo:**

    ```bash
    mkdir -p $HOME/emu-master-dev && cd $HOME/emu-master-dev
    repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev
    ```

3. **Download Code:**

    ```bash
    repo sync -j 8
    ```

## Building the Emulator

1. **Build:**

    ```bash
    cd external/qemu && android/rebuild.sh
    ```

2. **List Available Virtual Devices:**

    ```bash
    ./objs/emulator -list-avds
    ```

## Additional Notes

- **Incremental Builds:**

    ```bash
    export PATH=$PATH:$HOME/emu-master-dev/prebuilts/ninja/darwin-x86/
    ninja -C objs
    ```

- **Configuring the build:**

    You can get additional information on how to configure the build by running:

    ```bash
    android/rebuild.sh --help
    ```

    For example `./android/rebuild.sh --task CTest` will run all the unit tests

- **Running unit tests:**

    We use `ctest` to run unit tests. You can run them in the `objs` directory

    ```bash
    cd objs; ctest -j 10
    ```

### Downloading Pre-built Emulators

You can obtain the latest build of the emulator from our [build servers](https://ci.android.com/builds/branches/aosp-emu-master-dev/grid?legacy=1). Look for the `sdk-repo-{architecture}-emulator-{buildid}.zip` artifact.

You might need to remove a quarantine flag before running them:

```bash
xattr -dr com.apple.quarantine /path/to/my/unzipped/emulator
```

### Sending patches

Here you can find details on [submitting patches](https://source.android.com/docs/setup/contribute/submit-patches).
