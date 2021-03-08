Welcome to the Android Emulator
===============================

This document gives you some background on the emulator and outlines how you can start hacking and contributing to the emulator!

## Just get me started with development!

Make sure to install [Android Studio](https://developer.android.com/studio) and the associated SDKs. Do not forget to install the official emulator and create a few [android virtual devices](http://www.androiddocs.com/tools/devices/index.html). Next follow the instructions for the platform you would like to work on:

  - [On Linux](android/docs/LINUX-DEV.md)
  - [On Mac](android/docs/DARWIN-DEV.md)
  - [On Windows](android/docs/WINDOWS-DEV.md)

We use the [Repo](https://source.android.com/setup/develop/repo) tool to manage working accross multiple branches.

## About the Emulator
The Android Emulator simulates Android devices on your computer so that you can test your application on a variety of devices and Android API levels without needing to have each physical device.

The emulator provides almost all of the capabilities of a real Android device. You can simulate incoming phone calls and text messages, specify the location of the device, simulate different network speeds, simulate rotation and other hardware sensors, access the Google Play Store, and much more.

Testing your app on the emulator is in some ways faster and easier than doing so on a physical device. For example, you can transfer data faster to the emulator than to a device connected over USB.

The emulator comes with predefined configurations for various Android phone, tablet, Wear OS, and Android TV devices.

## Recommended Reading

The following is a list of concepts that are important. Please read these links and any other links you can find.  If you find a link that you think does a better job at explaining the concept, please add it here:

QEMU is an open source competitor to VMware Workstation, VirtualBox, HyperV.  It is focused on Linux server virtualization on Linux servers.  While QEMU does support booting other OS’s, we don’t use that functionality since Android is Linux.

The Android Emulator is downstream from the QEMU emulator.  It adds support for booting Android devices, emulates typical Android hardware (OpenGL, GPS, GSM, Sensors) and a GUI interface. The android emulator extends qemu in various ways.

For an overview of Qemu see:

- http://wiki.qemu.org/Main_Page
- https://en.wikibooks.org/wiki/QEMU
- https://en.wikibooks.org/wiki/QEMU/Images

The QEMU emulator leverages technologies like hardware Hypervisors KVM (Linux) and HAXM (Windows/Mac)

https://en.wikipedia.org/wiki/Hypervisor
https://en.wikipedia.org/wiki/Kernel-based_Virtual_Machine

The QEMU emulator supports both guest images that require full virtualization and guest images that require paravirtualization technologies like virtio

The emulator passes a [device tree](https://en.wikipedia.org/wiki/Device_tree) blob to a guest kernel to describe the guest hardware

When running a non-x86 image on an x86 host, QEMU will JIT the non-x86 code into x86 code. See this lectutre for [more](http://www.cs.cmu.edu/~412/lectures/L05_QEMU_BT.pdf). The MMU (page table hardware) is emulated in software, which is unfortunately slow.

You will need to build Android kernels and system images that the emulator will run. The easiest way to obtain these is to make use of the publicly released images. You can find more details [here](https://developer.android.com/studio/run/managing-avds).

## Building the Emulator

The emulator uses [Cmake](https://cmake.org/) as a meta build generator, and all the necessary compilers and toolchains are included in the repository. Make sure you have read the section [above](#Just-get-me-started-with-development) as the requirements to succesfully build vary slightly from platform to platform.

In general you can run the following script:

    ./android/rebuild.sh

For incremental builds you can use `ninja`. For example

    ninja -C objs

## Contributing code

The emulator uses a [coding style](android/docs/ANDROID-CODING-STYLE.md) derived from the [Chromium style](https://chromium.googlesource.com/chromium/src/+/master/styleguide/c++/c++.md). We use the repo tool to [submit pathces](https://gerrit.googlesource.com/git-repo/+/refs/heads/master/SUBMITTING_PATCHES.md). The usual workflow is roughly as follows:

```sh
    repy sync # Pulls in all the changes accross all branches.
    repo start my_awesome_feature
```

This will create a git branch called `my_awesome_feature`. You can now work on your patch.

Once you have written a patch you can send it out for code review. We use [gerrit](https://www.gerritcodereview.com/) for code reviews.

Use the repo tool to upload or update a CL:

```sh
    repo upload --cbr --re=”blah@google.com,foo@google.com,bar@google.com”
```

  - “--cbr” means “upload the current branch”,
  - “--re” supplies the initial reviewers list

Add “-t” switch to use the current git branch as a review topic (topic groups CLs together and only allows them to be submitted when all got +2/+verified)

The repo tool will provide you with a url where you can find your change.


### Code Reviews

Make sure to check the CL against our coding style: [coding style](android/docs/ANDROID-CODING-STYLE.md). Coding style isn’t frozen: just edit it in our repository and create a code review to propose a change.

Some good articles on code reviewing, especially when it comes to google:

- The [CL Author’s Guide](https://google.github.io/eng-practices/review/reviewer/), which gives detailed guidance to developers whose CLs are undergoing review.
- [How to Do a Code Review](https://google.github.io/eng-practices/review/developer/), which gives detailed guidance for code reviewers.

Below are some short notes relevant to the emulator.

**Keep in mind that all of the code reviews are open source and visible to everyone!**. In other words, be nice and and provide actionable constructive feedback.

- C++ over C for all new code. Always.

- After uploading a CL for review, author should “+1” it when they think it’s ready for reviewing. A CL without author’s “+1” is a “work in progress” and other reviewers may ignore it..

- Prefer not to send a WIP CL to reviewers and only add them when it’s in a reviewable state. If you realized that you added reviewers too soon, just remove them - “x” button in the browser UI for each reviewer actually works.

- Avoid large CLs. There are always exceptions, use your best judgement to improve code clarity and to help other maintainers in the future.

   Split the changes into smaller isolated chunks and submit those as a single topic
   If you touch multiple components, that’s a good way to split the CL.

    **Changes into any QEMU files must be in their own CL** - otherwise rebasing into the new version becomes 10x more painful.

-  “-2” is sticky, it remains there until the very same reviewer removes it. If you “-2”-ed someone, it’s always a good thing to communicate to them about the follow-up

- Abandon the CLs you don’t need anymore
- Tests: when reviewing the code, make sure there’s a test or a really good reason for its absence

## Merging downstream Qemu

Merging changes from the qemu branch should be done on the `emu-master-qemu` branch.
You will need to this on a linux machine as qemu development happens in a linux environment.
Once you have obtained this branch you can add the remote qemu repository as follows:

    cd emu-master-qemu/external/qemu
    git remote add qemu https://git.qemu.org/git/qemu.git
    git fetch qemu

Now you can start merging in changes:

    git merge master

**Be smart, merge only a few commits at a time**

Next you should try to build qemu standalone:

```sh
    ./android/scripts/unix/build-qemu-android.sh  --verbose --verbose --build-dir=$HOME/qemu-build  &&
    cd $HOME/qemu-build/build-linux-x86_64 &&
    source env.sh &&
    cd qemu-android &&
    make
```

Now you are ready for building, testing, and merging the next set. You could have a look at [this script](https://android-review.googlesource.com/c/platform/external/qemu/+/692577/2/android/scripts/git-merge-helper.sh) to automate this slightly.

