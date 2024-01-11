# Tool to Update Netsim Prebuilts for Android Emulator

## Overview

This is an internal tool that can be used to obtain a netsim build, unpack it and make it available as a prebuilt for the android emulator.

### Audience

This document is intended for developers and testers working with Netsim and the Android emulator.

### Prerequisites

* oauth2l: Ensure oauthl2 (or ~/go/bin/oauth2l) is installed. If not, obtain it from [go/oauth2l](http://go/oauth2l).

## Quick start

1. Run the tool:

          fetch-netsim.sh --bid <build_id>

     This will create a git commit and send it out for review to gerrit.

2. Dry run (optional):

          fetch-netsim.sh --bid <build_id> --dry-run

     This will execute all the steps, apart from any git (or repo) operations.

## Detailed usage

### Install

You can permanently install fetch-netsim by running `pip install`. This will install the fetch-netsim command. You can get usage instructions
by running:

     fetch-netsim -h

### Key steps performed by the tool

* Start a new branch for development (netsim-%bid%) in the `--dest`` directory (should default to proper place when run in $AOSP)
* Git remove all the older netsim objects
* For all the desired targets, obtain the zip file from go/ab and store it in a temporary directory
* Unzip the directory to the desired location, as set in the `--dest` flag. This is only needed if you are running the command outside of an emulator AOSP repository
* Create a git commit with all the changes
* Upload the commit, with the reviewers set.

### Testing

To get an idea of what the tool will do you can run the following:

     fetch-netsim.sh --dry-run --dest /tmp/netsim

This will obtain the latest netsim release, and unzip it in the /tmp/netsim directory.

## Known Issues

* Your token has expired. Simply run `~/go/bin/oauth2l reset`
* Make sure to use a build id (`--bid xxx`) for which all targets are green.
* I see failures when omitting the `--bid` parameter. The tool tries to fetch the latest build, it could be that it is not yet completed or has failures.
