#!/bin/bash
EMU=LATEST
CROS=61-9765.28.0

function get_partition() {
  local input=$1
  local partition=$2
  local output=$3
  if test ! -f $output;then
    local info=$(parted $input unit B print|grep $partition)
    local offset=$(echo $info|awk '{print $2}'|tr -d B)
    local length=$(echo $info|awk '{print $4}'|tr -d B)
    local offsetb=$(( $offset / 65536 ))
    local lengthb=$(( $length / 65536 ))
    dd if=$input of=$output.tmp skip=$offsetb count=$lengthb bs=65536
    mv $output.tmp $output
  fi
}

function fetch_artifact() {
  local branch=$1
  local target=$2
  local bid=$3
  local file=$4
  if test ! -f $file; then
    $FETCH_ARTIFACT --branch "$branch" --target "$target" --bid "$bid" "$file" "$file.tmp"
    mv $file.tmp $file
  fi
}

function pack_for_os() {
  local platform=$1

  case $platform in
    macos)
      TARGET=sdk_tools_mac
      OS=darwin
      SH_SUFFIX=sh
      ;;
    linux)
      TARGET=sdk_tools_linux
      OS=linux
      SH_SUFFIX=sh
      ;;
    windows)
      TARGET=sdk_tools_linux
      OS=windows
      SH_SUFFIX=bat
      ;;
  esac

  local package_dir=$PACKAGE_ROOT_DIR/cros_sdk_for_${platform}_$D
  mkdir -p $package_dir
  cd $PACKAGE_ROOT_DIR
  local emulator=sdk-repo-$OS-emulator-$BID.zip
  fetch_artifact "$AB_BRANCH" "$TARGET" "$BID" $emulator
  cd $package_dir
  rm -fr emulator
  mkdir -p emulator
  unzip ../$emulator -d emulator
  mv emulator/emulator emulator/chromeos
  wget -qO - https://android.googlesource.com/platform/external/qemu/+/emu-2.4-arc/android/scripts/convert_avd_to_cros.$SH_SUFFIX?format=TEXT|base64 -d > convert_avd_to_cros.$SH_SUFFIX
  chmod 755 convert_avd_to_cros.$SH_SUFFIX
  mv convert_avd_to_cros.$SH_SUFFIX emulator/chromeos
  rm -fr system-images
  mkdir -p system-images/chromeos-m$CROS_B/google_apis
  ln $(realpath ../$CROS_V/x86) system-images/chromeos-m$CROS_B/google_apis -sf
  zip -r - emulator system-images >  ../cros_sdk_for_${platform}_$D.zip.tmp
  mv ../cros_sdk_for_${platform}_$D.zip.tmp ../cros_sdk_for_${platform}_$D.zip
}

set -e

BRANCH=emu-2.4-arc
AB_BRANCH=aosp-$BRANCH
ARTIFACT_HANDLER=/google/data/ro/projects/android/apiary_artifact_handler.par
FETCH_ARTIFACT=/google/data/ro/projects/android/fetch_artifact.par
D=$(date +%Y%m%d)
PACKAGE_ROOT_DIR=$HOME/chromeos_emulator
CROS_B=$(echo $CROS|cut -d- -f1)
CROS_V=$(echo $CROS|cut -d- -f2)
if [[ $EMU = LATEST ]]; then
  BID=$("$ARTIFACT_HANDLER" --branch "$AB_BRANCH" --target sdk_tools_mac --latest --action getbuildid)
else
  BID=$EMU
fi
crosimg=ChromeOS-R$CROS-novato.zip
mkdir -p $PACKAGE_ROOT_DIR/$CROS_V/x86
cd $PACKAGE_ROOT_DIR
test -f $crosimg || gsutil cp gs://chromeos-releases/dev-channel/novato/$CROS_V/$crosimg .
cd $PACKAGE_ROOT_DIR/$CROS_V/x86
if test ! -f system.img; then
  unzip ../../$crosimg chromiumos_qemu_image.bin
  mv chromiumos_qemu_image.bin system.img
fi
get_partition system.img ROOT-A ../root.img
ARC_VERSION=$(debugfs ../root.img -R "cat /etc/lsb-release"|grep CHROMEOS_ARC_VERSION|cut -d= -f2)
cd ..  && $FETCH_ARTIFACT --bid $ARC_VERSION --target sdk_google_cheets_x86-userdebug build.prop
cd x86 && mv ../build.prop .
get_partition system.img EFI-SYSTEM ../boot.img
mcopy -i ../boot.img -n ::/syslinux/vmlinuz.a kernel-ranchu.tmp
for fake in userdata.img ramdisk.img;do
    touch $fake
done
wget -qO - https://android.googlesource.com/platform/external/qemu/+/emu-2.4-arc/android/scripts/cros_files/package.xml?format=TEXT|base64 -d |sed -e "s/chromeos-m60/chromeos-m$CROS_B/g" > package.xml
wget -qO - https://android.googlesource.com/platform/external/qemu/+/emu-2.4-arc/android/scripts/cros_files/source.properties?format=TEXT|base64 -d > source.properties
for sys in macos linux windows;do
  pack_for_os $sys
done
