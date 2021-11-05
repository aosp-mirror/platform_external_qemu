#!/bin/bash

emudir=$PWD/emulator
entitlexml=$PWD/_codesign/entitlements.xml
codesign -s - --entitlements $entitlexml $emudir/qemu/*/*
codesign -s - --entitlements $entitlexml $emudir/lib64/*.dylib
xattr -dr com.apple.quarantine $emudir
