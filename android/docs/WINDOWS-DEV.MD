Android Emulator Windows Development
=====================================

In order to develop the android emulator under windows you will need to have access to:
  - Have a bit9 exception http://go/bit9
  - Have admin privileges on your machine.
  - Our repository of sources:
      1. Use a windows version of repo to access the sources (RECOMMENDED)
      2. Use the linux subsystem to get access:
          - Currently not working due to issues with symlinking between WSL and NTFS
          - Abysmal performance due to file system translation
  - A Compiler toolchain to compile the sources, in this case Visual Studio 2017

# Using repo for windows

Install the following:

- [Visual Studio 2017](https://visualstudio.microsoft.com/downloads/). We will need the compiler toolchain with the latest windows sdk, as we need the WHPX headers.
    - [Python](https://www.python.org/downloads/windows/). You only need this if your version of visual studio does not come with python.
    - [Cmake](https://cmake.org/download/). You only need this if your version of visual studio does not come with cmake.
- [Git](https://git-scm.com/downloads). Make sure that the install updates your path, so git will be on the path.
- Add %USERPROFILE%\bin to the [path](https://www.windows-commandline.com/set-path-command-line/). 

Once you have installed these open the "Developer Command Prompt for VS 2017". Make sure your account has **[SeCreateSymbolicLinkPrivilege](https://security.stackexchange.com/questions/10194/why-do-you-have-to-be-an-admin-to-create-a-symlink-in-windows)**. The easiest way to make sure you have this is to open this with administrator privileges. 

1. First check that all dependencies are on the path:

    ```
    C:\> where git && where python && where cmake
    C:\Program Files\Git\cmd\git.exe
    C:\python_27_amd64\files\python.exe
    C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
    ``` 

2. The next step is to obtain a copy of [git-remote-sso.exe](https://x20web.corp.google.com/teams/gerritcodereview/bin/git-remote-sso.exe), and place it in the git\cmd directory (or anywhere else on the PATH).  **Optional, you can live without**

    ```
    C:> copy %USERPROFILE%\Downloads\git-remote-sso.exe "C:\Program Files\Git\cmd\"
    ```
3. Now we need to obtain a copy of repo for [windows](https://user.git.corp.google.com/rpaquay/git-repo-windows/+/windows-port-stable)

    ```
    C:> cd %USERPROFILE% && mkdir src && cd src
    C:> git clone -b windows-port-stable sso://user/rpaquay/git-repo-windows
    ```

4. Make sure we have a repo launcher somewhere on the path.

    ```
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

  5. Initialize the repository:

      ``` 
      C:> cd %USERPROFILE%\src && mkdir emu-master-dev && cd emu-master-dev
      C:> repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev          
      ```  

      ***Note:** we are not using persistent-https to initialize the repo*

  6. Sync the repo (and get some coffee, or a have a good nap.)

     ```
        C:>  cd %USERPROFILE%\src\emu-master-dev && repo sync -f --no-tags --optimized-fetch --prune
     ```

   7. Congratulations! You have all the sources you need. Now run:

     ```
        C:>  cd %USERPROFILE%\src\emu-master-dev\external\qemu && android\rebuild
     ```


   8. Setup git cookies so you can repo upload by following this link: [https://www.googlesource.com/new-password](https://www.googlesource.com/new-password)

## Using Debian. 

**NOTE: Currently does not work due symlinks not being visible in NTFS**

We will use debian with git & repo to access our source repository. You will have to use this approach if you do not have access to a windows version of repo. Installing debian can be done from [PowerShell:](https://docs.microsoft.com/en-us/powershell/).

**Make sure you have a bit9 exception before you start!**

First we need to enable the windows subsystem feature:

```
PS C:> Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Windows-Subsystem-Linux
```

Next we need to obtain the debian subsystem. You can obtain it from microsoft as follows

```
 PS C:\> Invoke-WebRequest -Uri https://aka.ms/wsl-debian-gnulinux -OutFile ~/Debian.zip -UseBasicParsing
```

You will now have a zipfile that contains the debian subsystem.  Simply unzip it:

```
PS C:\> Expand-Archive ~/Debian.zip -DestinationPath ~
```

And install/launch

```
PS C:> ~/Debian.exe
```

You now should have a debian shell. First thing we have to do is install a set of dependencies:

```
$ sudo apt-get update && sudo apt-get install python git curl vim wget byobu
```

In theory you should now be able to use the standard repo tools to obtain a copy of all the sources.