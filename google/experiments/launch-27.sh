#!/bin/bash
# Sample script that launches Android-27, this should be able to boot uop to the point of dropping into the adb shell
# It assumes that you have a 27.avd as well as a system image in $ANDROID_SDK_ROOT

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 DIRECTORY"
    exit 1
fi

directory="$1"

if [ ! -d "$directory" ]; then
    echo "Error: $directory is not a directory."
    exit 1
fi

qemu_path="$directory/qemu-system-x86_64"

if [ -e "$qemu_path" ]; then
    echo "$directory contains qemu-system-x86_64."
else
    echo "$directory does not contain qemu-system-x86_64."
    exit 1
fi


$qemu_path -serial stdio -enable-kvm -smp cores=4 -m 2048  -nodefaults \
-machine goldfish,vendor=/dev/block/pci/pci0000:00/0000:00:06.0/by-name/vendor,system=/dev/block/pci/pci0000:00/0000:00:02.0/by-name/system \
-object iothread,id=disk-iothread \
-kernel "$ANDROID_SDK_ROOT/system-images/android-27/default/x86_64/kernel-ranchu" \
-initrd  "$ANDROID_SDK_ROOT/system-images/android-27/default/x86_64/ramdisk.img" \
"-drive" \
 "if=none,index=0,id=system,if=none,file=$ANDROID_SDK_ROOT/system-images/android-27/default/x86_64/system.img,read-only" \
 "-device" \
 "virtio-blk-pci,drive=system,iothread=disk-iothread,modern-pio-notify" \
 "-drive" \
 "if=none,index=1,id=cache,if=none,file=$HOME/.android/avd/../avd/27.avd/cache.img.qcow2,overlap-check=none,cache=unsafe,l2-cache-size=1048576" \
 "-device" \
 "virtio-blk-pci,drive=cache,iothread=disk-iothread,modern-pio-notify" \
"-drive" \
 "if=none,index=2,id=userdata,if=none,file=$HOME/.android/avd/../avd/27.avd/userdata-qemu.img.qcow2,overlap-check=none,cache=unsafe,l2-cache-size=1048576" \
 "-device" \
 "virtio-blk-pci,drive=userdata,iothread=disk-iothread,modern-pio-notify" \
 "-drive" \
 "if=none,index=3,id=encrypt,if=none,file=$HOME/.android/avd/../avd/27.avd/encryptionkey.img.qcow2,overlap-check=none,cache=unsafe,l2-cache-size=1048576" \
 "-device" \
 "virtio-blk-pci,drive=encrypt,iothread=disk-iothread,modern-pio-notify" \
 "-drive" \
 "if=none,index=4,id=vendor,if=none,file=$ANDROID_SDK_ROOT/system-images/android-27/default/x86_64/vendor.img,read-only" \
 "-device" \
 "virtio-blk-pci,drive=vendor,iothread=disk-iothread,modern-pio-notify" \
 "-drive" \
 "if=none,index=5,id=sdcard,if=none,file=$HOME/.android/avd/../avd/27.avd/sdcard.img.qcow2,overlap-check=none,cache=unsafe,l2-cache-size=1048576" \
 "-device" \
 "virtio-blk-pci,drive=sdcard,iothread=disk-iothread,modern-pio-notify" \
-netdev user,id=mynet -device virtio-net-pci,netdev=mynet -device virtio-rng-pci -L $HOME/src/emu-master-dev/external/qemu/objs/lib/pc-bios -vga none -append 'no_timer_check clocksource=pit no-kvmclock console=ttyS0,38400 cma=296M@0-4G mac80211_hwsim.radios=2 mac80211_hwsim.channels=2 loop.max_part=7 ramoops.mem_address=0xff018000 ramoops.mem_size=0x10000 memmap=0x10000$0xff018000 printk.devkmsg=on qemu=1 androidboot.hardware=ranchu androidboot.serialno=EMULATOR34X1X12X0 qemu.gles=1 qemu.settings.system.screen_off_timeout=2147483647 qemu.encrypt=1 qemu.vsync=60 qemu.gltransport=pipe qemu.gltransport.drawFlushInterval=800 qemu.opengles.version=196608 qemu.uirenderer=skiagl qemu.dalvik.vm.heapsize=512m qemu.camera_hq_edge_processing=0 androidboot.android_dt_dir=/sys/bus/platform/devices/ANDR0001:00/properties/android/ qemu.wifi=1 android.qemud=1 qemu.avd_name=27 mac80211_hwsim.mac_prefix=5554' -nographic \
-monitor telnet::45454,server,nowait \
-audiodev none,id=hda,out.mixing-engine=off -device intel-hda -device hda-output,audiodev=hda \
-no-reboot
