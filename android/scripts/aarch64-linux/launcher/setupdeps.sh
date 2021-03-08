#!/bin/bash

if [ "$#" -ne 1 ]; then
echo "Usage: $0 rundir"
exit 1
fi

RUNROOT=$1

set -e

# setup gcc lib 2.9, as many aarch64 hosts do not have it
# it comes from the aarch64 gcc tool chain that is used to build
# emulator
# it also has a copy of tcmalloc.so

GLIBC="glibc-2.29.tgz"

if [ ! -f $GLIBC ]; then
echo "please ensure $GLIBC exists in current directory and try again."
exit 1
fi

# need this to modify the interpreter path and rpath
PATCHELF="patchelf.tgz"

if [ ! -f $PATCHELF ]; then
echo "please ensure $PATCHELF exists in current directory and try again."
exit 2
fi

# provide a stub xlib that is still needed even for headless
STUBXLIB="libStubXlib.so.tgz"
if [ ! -f $STUBXLIB ]; then
echo "please ensure $STUBXLIB exists in current directory and try again."
exit 3
fi

# adb binary
ADB="platform-tools.tgz"
if [ ! -f $ADB ]; then
echo "please ensure $ADB exists in current directory and try again."
exit 3
fi


mkdir -p $RUNROOT/lib
tar xzvf $GLIBC -C $RUNROOT/lib

mkdir -p $RUNROOT/bin
tar xzvf $PATCHELF -C $RUNROOT/bin

mkdir -p $RUNROOT/lib/stubxlib
tar xzvf $STUBXLIB -C $RUNROOT/lib/stubxlib

mkdir -p $RUNROOT/sdkroot/platform-tools
tar xzvf $ADB -C $RUNROOT/sdkroot/platform-tools

# now run patchelf on emulator and qemu-system-aarch64-headless
gcclibpath=$PWD/$RUNROOT/lib/lib
$RUNROOT/bin/patchelf --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $RUNROOT/sdkroot/emulator/emulator

$RUNROOT/bin/patchelf --set-interpreter $gcclibpath/ld-linux-aarch64.so.1 --set-rpath $gcclibpath:\$ORIGIN/lib64:\$ORIGIN $RUNROOT/sdkroot/emulator/qemu/linux-aarch64/qemu-system-aarch64-headless

cat <<EOT > runemu.sh
#!/bin/bash

export LD_PRELOAD=$PWD/$RUNROOT/lib/stubxlib/libStubXlib.so
export QT_NO_GLIB=1
export ANDROID_EMUGL_FIXED_BACKEND_LIST=1
export ANDROID_EMUGL_VERBOSE=1

emulator -gpu swiftshader_indirect -avd a1 -verbose -show-kernel -feature -RefCountPipe -no-snapshot -no-window "\$@"
EOT

chmod a+x runemu.sh
