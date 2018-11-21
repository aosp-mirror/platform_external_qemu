Android Emulator Windows Development
=====================================

In order to develop the android emulator under windows you will need to have access to:
  - Have a bit9 exception http://go/bit9
  - Our repository of sources
      -  We will use the linux subsystem to get access.
  - A Compiler toolchain to compile the sources, in this case Visual Studio 2017

# Install Debian

We will use debian with git & repo to access our source repository.  Installing debian can be done from [PowerShell:](https://docs.microsoft.com/en-us/powershell/).

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

We will need to install some git extensions to connect to our repositories, which
we will do by:

- Compiling the tools themselves.
  ```
  $ sudo wget https://dl.google.com/go/go1.11.2.linux-amd64.tar.gz
  $ tar -C /usr/local -xzf https://dl.google.com/go/go1.11.2.linux-amd64.tar.gz
  $ sudo su
  $ export PATH=$PATH:/usr/local/go/bin
  $ cd /usr/share/doc/git/contrib/persistent-https
  $ make
  $ cp git-remote-persistent* /usr/local/bin
  ```

- Copy the ones I [created](https://drive.google.com/open?id=1jt5kIvUgEKnQJqp1OalkPHNpkWvDqiBp) and place them in /usr/local/bin


Next we will have to configure git:

```
git config
```

Now you can follow the steps to obtain the repository


