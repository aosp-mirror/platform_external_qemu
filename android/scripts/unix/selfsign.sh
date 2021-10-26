#!/bin/bash
set -e


emudir=$PWD/emulator
entitlexml=$PWD/_codesign/entitlements.xml
codesign -s - --entitlements $entitlexml --deep --force $emudir/qemu/*/*
codesign -s - --entitlements $entitlexml --deep --force $emudir/emulator
xattr -dr com.apple.quarantine $emudir
