Android Emulator Windows Development
=====================================

This document describes how to get started with emulator development under windows.
If you want to use your own machine you can skip the sections on cloud an gWindows.

*Note: It is possible to cross compile to windows from both [Linux](LINUX-DEV.md) and [MacOs](DARWIN-DEV.md)

## Developing on gWindows.

This section is only relevant if you work at Google.

In order to develop the android emulator in gWindows you will need to meet the
following requirements.

  - Have a bit9 exception http://go/bit9
  - Exception for credential guard http://go/winops-credential-guard (if you
    want to use haxm)
  - Have admin privileges on your machine.
  - A Compiler toolchain to compile the sources. Any edition of Visual Studio
    2017 is known to work

*Note: The gWindows machines are locked down very well and you will experience
some performance penalties due to this. You are highly encouraged to develop in GCE*

## Developing in GCE

This section is only relevant if you want to setup a development environment in google
cloud.

First we need to create a Windows image with nested virtualization [enabled](https://cloud.google.com/compute/docs/instances/enable-nested-virtualization-vm-instances):

*Note: this is not officially supported, but we have not seen any issues.*

```sh
  $ WIN_VER=windows-2012-r2 && ZONE=us-west1-b &&
   gcloud compute disks create nested-$WIN_VER \
    --image-project=windows-cloud \
    --image-family $WIN_VER \
    --zone $ZONE \
    --size 500GB
    --licenses "https://compute.googleapis.com/compute/v1/projects/vm-options/global/licenses/enable-vmx" &&
   gcloud compute images create nested-$WIN_VER-image \
    --source-disk nested-$WIN_VER \
    --source-disk-zone $ZONE \
    --licenses "https://compute.googleapis.com/compute/v1/projects/vm-options/global/licenses/enable-vmx"
```

You can change the family to the latest windows version, and of course set the zone and disk size to your liking. Do not pick a core edition, as you will want to have a shell.

Next we create an instance and install all the necessary things

```sh
    $ gcloud compute instances create  \
      $USER-windows-development-01 \
      --zone $ZONE \
      --min-cpu-platform "Intel Haswell" \
      --image nested-$WIN_VER
```

You will need to setup the password, after which you should be able to login.

```sh
     gcloud beta compute reset-windows-password \
     $USER-windows-development-01  \
      --zone=$ZONE
      --user=$USER
```
# Software Requirements

Install the following:

- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/). We will
  need the compiler toolchain with the latest windows sdk, as we need the WHPX
  headers. Make sure to install the following set of tools:
    - Visual C++ build tools
    - Visual C++ ATL for x86 & x64
    - Visual C++ MFC for x86 & x64
    - Windows 10 SDK (10.0.177763.0)
- [Git](https://git-scm.com/downloads). Make sure that the install updates your
  path, so git will be on the path.
- [Python3](https://www.python.org/downloads/windows/). Needed for repo.
- [VScode](https://code.visualstudio.com/) Visual studio code works well for
  emulator development. Make sure to install the at least following extensions:
    - [ms-vscode.cpptool](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools).
      Needed for debugging.
    - [ms-vscode.cmake-tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools).
      Needed for compiling.
- Add %USERPROFILE%\bin to the
  [path](https://www.windows-commandline.com/set-path-command-line/).

If you are using [Chocolatey](https://chocolatey.org/), you can install all the
dependencies with the following command in an elevated terminal window (i.e.
type Win-R key then type "cmd"):

```bat
C:\>@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command " [System.Net.ServicePointManager]::SecurityProtocol = 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"

C:> choco install -y visualstudio2019community visualstudio2019buildtools visualstudio2019-workload-python visualstudio2019-workload-nativedesktop visualstudio2019-workload-vctools git vscode python3
```

You must run visual studio at least once to accept a license and do some initial configuration.

Make sure your account has
**[SeCreateSymbolicLinkPrivilege](https://security.stackexchange.com/questions/10194/why-do-you-have-to-be-an-admin-to-create-a-symlink-in-windows)**.
The easiest way to make sure you have this is to open every terminal window
with administrator privileges.

Once you have installed everything you can validate that you have what you need on the path by executing:

  ```bat
  C:\> where git && where python

  ```

## Configure git

You will need to setup your name and make sure you enable symlinks. Repo will
do its best to work without symlinks enabled, but we have not tested this. Make
sure you have the following settings set:

  ```bat
  C:\> git config --system core.symlinks true
  C:\> git config --global core.symlinks true
  C:\> git config --global user.name "I just copy pasted this without looking"
  C:\> git config --global user.email wrong-do-it-again@badrobot.com
  ```

## Obtaining repo for windows

*Note: The instructions in this section are for using the native Windows
version of the `repo` tool, where "native" is used to mean without "cygwin".*

There are 2 advantages running with Windows native tools:
  - Improved performance. For example, Git for Windows is usually dramatically
    faster the git for cygwin for many common operations
  - Compatibility with other native Windows tools: e.g. cmd.exe, TortoiseGit,
    IntelliJ, etc.

From an elevated command prompt execute:

  ```bat
  C:> mkdir %USERPROFILE%\bin && cd %USERPROFILE%\bin && curl -o repo http://storage.googleapis.com/git-repo-downloads/repo && echo @call python %~dp0repo %* > %USERPROFILE%/bin/repo.cmd
  ```

Make sure to add `%USERPROFILE%\bin` to the `PATH`, otherwise you will not
be able to use repo.

 ### Initialize the repository:

You can initialize repo as follows:

  ```bat
  C:> cd %USERPROFILE%\src && mkdir emu-master-dev && cd emu-master-dev
  C:> repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev --worktree
  ```

***Note:** we are not using persistent-https to initialize the repo, this means
performance will not be as good as it could be.*


Sync the repo (and get some coffee, or a have a good nap.)

  ```bat
    C:>  cd %USERPROFILE%\src\emu-master-dev && repo sync --no-tags --optimized-fetch --prune
  ```

Congratulations! You have all the sources you need. Now run:

  ```bat
    C:>  cd %USERPROFILE%\src\emu-master-dev\external\qemu && android\rebuild
  ```

You should end up with an emulator in the objs directory.

You can pass the flag `--help` to the rebuild script to get an idea of which options you can pass in.

### Setup git cookies

Setup git cookies so you can repo upload by following this link:
[https://www.googlesource.com/new-password](https://www.googlesource.com/new-password).

Make sure you have configured the proper user name and email address:

  ```bat
    C:\>git config --global user.name "I just copy pasted this without looking"
    C:\>git config --global user.email wrong-do-it-again@badrobot.com
  ```

Here you can find more details on [submitting patches](
https://gerrit.googlesource.com/git-repo/+/refs/heads/master/SUBMITTING_PATCHES.md).

# Known Issues

## I can't compile under visual studio code

Our cmake files rely on the visual studio environment variables to be set as
they are used to properly configure the compiler toolchain.
The easiest workaround is to start visual studio code from the command line
after loading the visual studio variables. Look for the line: `Accepted,
setting env with` in the output from `rebuild.cmd`.


## Where do I obtain avds etc?

The easiest way to get started is to install android studio:

```bat
  C:> choco install androidstudio
```

Next you can launch android studio and install a series of avds that you can use
