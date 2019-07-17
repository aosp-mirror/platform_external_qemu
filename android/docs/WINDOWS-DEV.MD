Android Emulator Windows Development
=====================================

In order to develop the android emulator @Google under windows you will need to have access to:
  - Have a bit9 exception http://go/bit9
  - Exception for credential guard http://go/winops-credential-guard (if you want to use haxm)
  - Have admin privileges on your machine.
  - Access to the repository of sources. All the sources are public, however the repo tools is not public as of yet.
  - A Compiler toolchain to compile the sources. Any edition of Visual Studio 2017 is known to work

# Software Requirements

Install the following:

- [Visual Studio 2017](https://visualstudio.microsoft.com/downloads/). We will need the compiler toolchain with the latest windows sdk, as we need the WHPX headers. Make sure to install the following set of tools:
    - Visual C++ build tools
    - Visual C++ ATL for x86 & x64
    - Visual C++ MFC for x86 & x64
    - Windows 10 SDK (10.0.177763.0)
    - [Python](https://www.python.org/downloads/windows/). You only need this if your version of visual studio does not come with python.
    - [Cmake](https://cmake.org/download/). You only need this if your version of visual studio does not come with cmake.
- [Android Studio](https://developer.android.com/studio). You will need this to run the emulator.
    - Make sure to create an avd so you can run the emulator.
    - Make sure you install a hypervisor (whpx/haxm).
- [Git](https://git-scm.com/downloads). Make sure that the install updates your path, so git will be on the path.
- [VScode](https://code.visualstudio.com/) Visual studio code works well for emulator development. Make sure to install the at least following extensions:
    - [ms-vscode.cpptool](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools). Needed for debugging.
    - [vector-of-bool.cmake-tools](https://marketplace.visualstudio.com/items?itemName=vector-of-bool.cmake-tools). Needed for compiling.
- Add %USERPROFILE%\bin to the [path](https://www.windows-commandline.com/set-path-command-line/).

If you are using [Chocolatey](https://chocolatey.org/), you can install all the dependencies with the following command:

```bat
C:\> @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"

C:> choco install -y python2 visualstudio2017community visualstudio2017-workload-vctools visualstudio2017-workload-nativedesktop cmake

```

You must run visual studio at least once to accept a license and do some initial configuration.

Make sure your account has **[SeCreateSymbolicLinkPrivilege](https://security.stackexchange.com/questions/10194/why-do-you-have-to-be-an-admin-to-create-a-symlink-in-windows)**. The easiest way to make sure you have this is to open this with administrator privileges.

Once you have installed everything you can validate that you have what you need on the path by executing:

  ```bat
  C:\> where git && where python
  C:\Program Files\Git\cmd\git.exe
  C:\python_27_amd64\files\python.exe
  ```

## Configure git

Git might not be configured properly to handle symlinks well on windows. Make sure you have the following settings set:

  ```bat
    C:\> git config --system core.symlink true
    C:\> git config --global core.symlink true
  ```


## Obtaining repo for windows

In order to work with the emulator code base you will need to have access to repo. Currently there is an unreleased internal version that works reasonably well with windows. You can obtain the official from the internal git repo, or you can download a zipped version from here:

  ```bat
  C:\>  gsutil cp gs://emu-dev-development/git-repo-windows.zip .
  ```

Contact the emu-dev team if you need access to the storage bucket.
*Note: we are looking into properly open sourcing these internal changes*

### Obtaining the version through git:

The first step is to obtain a copy of [git-remote-sso.exe](https://x20web.corp.google.com/teams/gerritcodereview/bin/git-remote-sso.exe), and place it in the git\cmd directory (or anywhere else on the PATH).

  ```bat
  C:> copy %USERPROFILE%\Downloads\git-remote-sso.exe "C:\Program Files\Git\cmd\"
  ```
Now we need to obtain a copy of repo for [windows](https://user.git.corp.google.com/rpaquay/git-repo-windows/+/windows-port-stable)

  ```bat
  C:> cd %USERPROFILE% && mkdir src && cd src
  C:> git clone -b windows-port-stable sso://user/rpaquay/git-repo-windows
  ```

### Add repo on the path

Make sure we have a repo launcher somewhere on the path.

  ```bat
  C:> echo @call python "%USERPROFILE%\src\git-repo-windows\repo" %* > "%USERPROFILE%\bin\repo.cmd"
  C:> repo help
  usage: repo COMMAND [ARGS]

  repo is not yet installed.  Use "repo init" to install it here.

  The most commonly used repo commands are:

    init      Install repo in the current working directory
    help      Display detailed help on a command

  For access to the full online help, install repo ("repo init").
  ```

    You now should have repo on the path, and should be able to initialize a repo. Which we will do next:

 ### Initialize the repository:

You can initialize repo as follows:

  ```bat
  C:> cd %USERPROFILE%\src && mkdir emu-master-dev && cd emu-master-dev
  C:> repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev
  ```

***Note:** we are not using persistent-https to initialize the repo, this means performance will not be as good as it could be.*


Sync the repo (and get some coffee, or a have a good nap.)

  ```bat
    C:>  cd %USERPROFILE%\src\emu-master-dev && repo sync -f --no-tags --optimized-fetch --prune
  ```

Congratulations! You have all the sources you need. Now run:

  ```bat
    C:>  cd %USERPROFILE%\src\emu-master-dev\external\qemu && android\rebuild
  ```


### Setup git cookies

Setup git cookies so you can repo upload by following this link: [https://www.googlesource.com/new-password](https://www.googlesource.com/new-password)

Make sure you have configured the proper user name and email address:

  ```bat
    C:\>git config --global user.name "I just copy pasted this without looking"
    C:\>git config --global user.email wrong-do-it-again@badrobot.com
  ```