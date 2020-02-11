Android Emulator Windows Development
=====================================

This document describes how to get started with emulator development under windows.

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

# Software Requirements

Install the following:

- [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/). We will
  need the compiler toolchain with the latest windows sdk, as we need the WHPX
  headers. Make sure to install the following set of tools:
    - Visual C++ build tools
    - Visual C++ ATL for x86 & x64
    - Visual C++ MFC for x86 & x64
    - Windows 10 SDK (10.0.177763.0)
    - Make sure you have at least 16.3.1 or later installed! Older versions
      have some STL issues that break the build.  See
      [b/141381410](http://b/141381410) for details.
    - [Python](https://www.python.org/downloads/windows/). You only need this
      if your version of visual studio does not come with python.
    - [Cmake](https://cmake.org/download/). You only need this if your version
      of visual studio does not come with cmake.
- [Android Studio](https://developer.android.com/studio). You will need this to
  run the emulator.
    - Make sure to create an avd so you can run the emulator.
    - Make sure you install a hypervisor (whpx/haxm).
- [Git](https://git-scm.com/downloads). Make sure that the install updates your
  path, so git will be on the path.
- [VScode](https://code.visualstudio.com/) Visual studio code works well for
  emulator development. Make sure to install the at least following extensions:
    - [ms-vscode.cpptool](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools).
      Needed for debugging.
    - [vector-of-bool.cmake-tools](https://marketplace.visualstudio.com/items?itemName=vector-of-bool.cmake-tools).
      Needed for compiling.
- Add %USERPROFILE%\bin to the
  [path](https://www.windows-commandline.com/set-path-command-line/).

If you are using [Chocolatey](https://chocolatey.org/), you can install all the
dependencies with the following command in an elevated terminal window (i.e.
type Win-R key then type "cmd"):

```bat
C:\> @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"

C:> choco install -y python2 visualstudio2017community visualstudio2017-workload-vctools visualstudio2017-workload-nativedesktop cmake

```

You must run visual studio at least once to accept a license and do some initial configuration.

Make sure your account has
**[SeCreateSymbolicLinkPrivilege](https://security.stackexchange.com/questions/10194/why-do-you-have-to-be-an-admin-to-create-a-symlink-in-windows)**.
The easiest way to make sure you have this is to open every terminal window
with administrator privileges.

Once you have installed everything you can validate that you have what you need on the path by executing:

  ```bat
  C:\> where git && where python
  C:\Program Files\Git\cmd\git.exe
  C:\python_27_amd64\files\python.exe
  ```

## Configure git

Git might not be configured properly to handle symlinks well on windows. Make
sure you have the following settings set:

  ```bat
  C:\> git config --system core.symlinks true
  C:\> git config --global core.symlinks true
  ```

## Obtaining repo for windows

*Note: The instructions in this section are for using the native Windows
version of the `repo` tool, where "native" is used to mean without "cygwin".*

There are 2 advantages running with Windows native tools:
  - Improved performance. For example, Git for Windows is usually dramatically
    faster the git for cygwin for many common operations
  - Compatibility with other native Windows tools: e.g. cmd.exe, TortoiseGit,
    IntelliJ, etc.

From a PowerShell window (i.e. type Win-R key then type "Powershell"):

  ```PS
  PS C:\Users\hacker> md "$env:USERPROFILE\bin\"
  PS C:\Users\hacker> $wc = New-Object Net.WebClient
  PS C:\Users\hacker> $wc.DownloadFile('http://storage.googleapis.com/git-repo-downloads/repo', "$env:USERPROFILE\bin\repo")
  PS C:\Users\hacker> '@call python %~dp0repo %*' | Out-File -FilePath "$env:USERPROFILE\bin\repo.cmd"
  ```

Make sure to add `"$env:USERPROFILE\bin"` to the `PATH`, otherwise you will not
be able to use repo. For the example above this would be:
`C:\Users\hacker\bin"`.

*Note: If the output of “repo” commands produce escaped ANSI codes (e.g.
“ESC[m”), try the following:*

  ```bat
  C:> set LESS=-r
  ```

 ### Initialize the repository:

You can initialize repo as follows:

  ```bat
  C:> cd %USERPROFILE%\src && mkdir emu-master-dev && cd emu-master-dev
  C:> repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev
  ```

***Note:** we are not using persistent-https to initialize the repo, this means
performance will not be as good as it could be.*


Sync the repo (and get some coffee, or a have a good nap.)

  ```bat
    C:>  cd %USERPROFILE%\src\emu-master-dev && repo sync -f --no-tags --optimized-fetch --prune
  ```

Congratulations! You have all the sources you need. Now run:

  ```bat
    C:>  cd %USERPROFILE%\src\emu-master-dev\external\qemu && android\rebuild
  ```


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
