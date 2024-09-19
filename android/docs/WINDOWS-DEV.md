# Android Emulator Windows Development

This guide helps you set up the Android Emulator on Windows for development. You can use your own computer or Google Compute Engine (GCE).

## Software Requirements

- **Visual Studio 2022:** Install the Community edition with specific components (C++ build tools, ATL, MFC, Windows 10 SDK).
- **Git:** Install and ensure it's added to your PATH.
- **Chocolatey (Optional):** Install all dependencies using the provided command if you have Chocolatey.

    To install everything with chocolatey:

    ```bat
    @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command " [System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"

    choco install -y visualstudio2022community visualstudio2022buildtools visualstudio2022-workload-python visualstudio2022-workload-nativedesktop visualstudio2022-workload-vctools git curl
    ```

Run Visual Studio once to accept the license. Ensure your user account has the "SeCreateSymbolicLinkPrivilege." Open terminals with administrator privileges for easier setup.

## Configure git

You will need to setup your name and make sure you enable symlinks. Repo will do its best to work without symlinks enabled, but we have not tested this. Make sure you have the following settings set:

```bat
git config --system core.symlinks true
git config --global core.symlinks true
git config --global user.name "I just copy pasted this without looking"
git config --global user.email wrong-do-it-again@badrobot.com
```

## Getting the Source Code

1. **Install Repo:**

   From an elevated [visual studio command prompt](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=vs-2022) execute:

    ```bat
    mkdir %USERPROFILE%\bin && cd %USERPROFILE%\bin && curl -o repo http://storage.googleapis.com/git-repo-downloads/repo && echo @call python %~dp0repo %* > %USERPROFILE%\bin\repo.cmd
    ```

    Make sure to add `%USERPROFILE%\bin` to the `PATH`, otherwise you will not
    be able to use repo.

2. **Initialize Repo:**

    ```bat
    cd \ && mkdir emu-master-dev && cd emu-master-dev
    repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev
    ```

3. **Download Code:**

    ```bat
    repo sync
    ```

## Building the Emulator

1. **Build:**

    ```bat
    cd external\qemu && android\rebuild
    ```

2. **List Available Virtual Devices:**

    ```bat
    \objs\emulator -list-avds
    ```

## Additional Notes

- **Incremental Builds:**

    ```bat
    ninja -C objs
    ```

- **Configuring the build:**

    You can get additional information on how to configure the build by running:

    ```bat
    android\rebuild --help
    ```

    For example `android\rebuild --task CTest` will run all the unit tests

- **Running unit tests:**

    We use `ctest` to run unit tests. You can run them in the `objs` directory

    ```bat
    cd objs; ctest -j 10
    ```

### Downloading Pre-built Emulators

You can obtain the latest build of the emulator from our [build servers](https://ci.android.com/builds/branches/aosp-emu-master-dev/grid?legacy=1). Look for the `sdk-repo-{architecture}-emulator-{buildid}.zip` artifact.

### Sending patches

Here you can find details on [submitting patches](https://source.android.com/docs/setup/contribute/submit-patches).