#!/bin/bash
## Example usage:
##   HOST=windows EMU=4286764 CROS=60-9592.63.0 ./package-cros-sdk.sh
EMU="${EMU:-LATEST}"
CROS="${CROS:-61-9765.31.0}"
CHANNEL="${CHANNEL:-beta}"
HOST="${HOST:-linux windows macos}"

# validate some string against regex and show warning
function validate_str() {
  local string="$1"
  local regex="$2"
  local warn="$3"

  if [[ "${string}" =~ ${regex} ]]; then
    return
  fi
  echo "${warn}" && exit 1
}

# extract one partition from whole disk image.
function get_partition() {
  local input="$1"
  local partition="$2"
  local output="$3"
  if [[ ! -f "${output}" ]];then
    local info="$(parted "${input}" unit B print|grep "${partition}")"
    local offset="$(echo "${info}"|awk '{print $2}'|tr -d B)"
    local length="$(echo "${info}"|awk '{print $4}'|tr -d B)"
    local offsetb="$(( offset / 65536 ))"
    local lengthb="$(( length / 65536 ))"
    dd if="${input}" of="${output}".tmp skip="${offsetb}" count="${lengthb}" \
        bs=65536
    mv "${output}".tmp "${output}"
  fi
}

function fetch_artifact() {
  local branch="$1"
  local target="$2"
  local bid="$3"
  local file="$4"
  if [[ ! -f "${file}" ]]; then
    "${FETCH_ARTIFACT}" --branch "${branch}" --target "${target}" \
        --bid "${bid}" "${file}" "${file}".tmp
    mv "${file}".tmp "${file}"
  else
    echo "Reusing local copy of ${file}"
  fi
}

function pack_for_os() {
  local platform="$1"

  case "${platform}" in
    macos)
      TARGET=sdk_tools_mac
      OS=darwin
      ;;
    linux)
      TARGET=sdk_tools_linux
      OS=linux
      ;;
    windows)
      TARGET=sdk_tools_linux
      OS=windows
      ;;
  esac

  local package_dir="${TMP_SDK_DIR}/${OS}"
  mkdir "${package_dir}"
  cd "${PACKAGE_ROOT_DIR}"
  local emulator="sdk-repo-${OS}-emulator-${EMU}.zip"
  fetch_artifact "${AB_BRANCH}" "${TARGET}" "${EMU}" "${emulator}"
  cd "${package_dir}"
  unzip -q "${PACKAGE_ROOT_DIR}/${emulator}" -d emulator
  mv emulator/emulator emulator/chromeos
  local img_dir=system-images/chromeos-m"${CROS_MILESTONE}"/chromeos
  mkdir -p "${img_dir}"
  ln "${TMP_SDK_DIR}/x86" "${img_dir}" -sf
  ln "${TMP_SDK_DIR}/extras" . -sf
  local zipf="${PACKAGE_ROOT_DIR}/cros_sdk_for_${platform}_${VERSION}.zip"
  # By default, zip dereferences symlink.
  zip -qr - emulator system-images extras > "${zipf}.tmp"
  mv "${zipf}.tmp" "${zipf}"
  echo "${zipf} generated."
}

set -e

validate_str "$1" "^$" "$(grep "^##" $0)"
validate_str "${EMU}" "^([0-9]*|LATEST)$" \
    "${EMU} doesn't look like a valid emulator build id"
validate_str "${CROS}" "^[0-9.\-]*$" \
    "${CROS} doesn't look like a valid Chrome OS version"
validate_str "${CHANNEL}" "^beta|dev$" "${CHANNEL} isn't a valid channel"
for sys in ${HOST}; do
  validate_str "${sys}" "^windows|linux|macos$" "${sys} isn't a valid host"
done

BRANCH=emu-2.4-arc
AB_BRANCH="aosp-${BRANCH}"
ARTIFACT_HANDLER=/google/data/ro/projects/android/apiary_artifact_handler.par
FETCH_ARTIFACT=/google/data/ro/projects/android/fetch_artifact.par
PACKAGE_ROOT_DIR="${HOME}/chromeos_emulator"
CROS_MILESTONE="$(echo "${CROS}"|cut -d- -f1)"
CROS_VERSION="$(echo "${CROS}"|cut -d- -f2)"
if [[ "${EMU}" == LATEST ]]; then
  EMU="$("${ARTIFACT_HANDLER}" --branch "${AB_BRANCH}" \
	     --target sdk_tools_mac --latest --action getbuildid)"
  echo "Detected latest build id is ${EMU} on ab/${BRANCH}"
fi
VERSION="${EMU}-${CROS}"

echo "Packaging emulator version ${EMU} with Chrome OS version ${CROS} "
echo "for targets ${HOST}"
echo "Is that correct? [y/N]"
read answer
if [[ "${answer}" != Y && "${answer}" != y ]]; then
    exit 1
fi

echo "Get system.img from build server."
crosimg=ChromeOS-R"${CROS}"-novato.zip
mkdir -p "${PACKAGE_ROOT_DIR}/${CROS_VERSION}/x86"
cd "${PACKAGE_ROOT_DIR}"
if [[ -f "${crosimg}" ]]; then
   echo "Reusing local copy of ${crosimg}"
else
   gsutil cp "gs://chromeos-releases/${CHANNEL}-channel/novato/${CROS_VERSION}/${crosimg}" .
fi
cd "${PACKAGE_ROOT_DIR}/${CROS_VERSION}/x86"
if [[ ! -f system.img || -z "${CACHE_SYSTEM_IMG}" ]]; then
  unzip ../../"${crosimg}" chromiumos_qemu_image.bin
  mv chromiumos_qemu_image.bin system.img
else
  echo "Reusing local copy of system.img in ${CROS_VERSION}/x86"
fi

echo "Building directory structure of system image"
TMP_SDK_DIR="$(mktemp -d /tmp/cros_sdk.XXXXXXX)"
trap "{ rm -fr ${TMP_SDK_DIR}; }" EXIT
mkdir "${TMP_SDK_DIR}/x86"
cd "${TMP_SDK_DIR}/x86"
ln "${PACKAGE_ROOT_DIR}/${CROS_VERSION}/x86/system.img" . -sf

echo "Extract arc version from system.img and then get build.prop from go/ab"
get_partition system.img ROOT-A ../root.img
ARC_VERSION="$(debugfs ../root.img -R "cat /etc/lsb-release" | \
	           grep CHROMEOS_ARC_VERSION|cut -d= -f2)"
# Fetch file outside x86 dir so there's no garbage file in it if fetch fails.
cd ..  && "${FETCH_ARTIFACT}" --bid "${ARC_VERSION}" \
     --target sdk_google_cheets_x86-userdebug build.prop
cd x86 && mv ../build.prop .

echo "Extract kernel from boot partition of system.img"
get_partition system.img EFI-SYSTEM ../boot.img
mcopy -i ../boot.img -n ::/syslinux/vmlinuz.a kernel-ranchu

echo "Generate fake userdata.img"
touch userdata.img

echo "Add package.xml, devices.xml and source.properties"
URLD=https://android.googlesource.com/platform/external/qemu/+/emu-2.4-arc/android/scripts/cros_files
wget -qO - "${URLD}"/package.xml?format=TEXT|base64 -d | \
	sed -e "s/chromeos-m60/chromeos-m${CROS_MILESTONE}/g" > package.xml
wget -qO - "${URLD}"/source.properties?format=TEXT|base64 -d > source.properties

echo "Put devices.xml in extras"
DEVD="${TMP_SDK_DIR}/extras/chromeos/DeviceProfiles"
mkdir -p "${DEVD}"
cd "${DEVD}"
wget -qO - "${URLD}"/devices.xml?format=TEXT|base64 -d > devices.xml
echo Extra.Path=DeviceProfiles > source.properties

echo "Now package for all systems"
for sys in ${HOST}; do
  echo "Packaging for ${sys}..."
  pack_for_os "${sys}"
done
echo Success
