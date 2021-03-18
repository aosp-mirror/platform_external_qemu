#!/bin/sh
codesign --deep -s - --entitlements ./entitlements.plist $1

