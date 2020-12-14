#!/bin/bash
# Copyright 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This is a script for the emulator to detect whether it's been translated in
# Rosetta, and, if so, delete itself and replace it with a native Apple Silicon
# build.  To be superseded with arch-specific downloads in SDK Manager.

cd "${0%/*}"

# Fail on error
set -e

# Download the aarch64 emulator

EMULATOR_DOWNLOAD_BASE_NAME="emulator-darwin-aarch64-0.2"
rm -rf ${EMULATOR_DOWNLOAD_BASE_NAME}
echo "curl -L https://github.com/google/android-emulator-m1-preview/releases/download/0.2/${EMULATOR_DOWNLOAD_BASE_NAME}-engine-only.zip -o ${EMULATOR_DOWNLOAD_BASE_NAME}.zip"
curl -L https://github.com/google/android-emulator-m1-preview/releases/download/0.2/${EMULATOR_DOWNLOAD_BASE_NAME}-engine-only.zip -o ${EMULATOR_DOWNLOAD_BASE_NAME}.zip && \
echo "unzip ${EMULATOR_DOWNLOAD_BASE_NAME}.zip" && \
unzip ${EMULATOR_DOWNLOAD_BASE_NAME}.zip && \

# Delete all emulator files

rm ./qsn
rm ./qemu-img
rm ./lib64/libemugl_common.dylib
rm ./lib64/qt/plugins/platforms/libqcocoa.dylib
rm ./lib64/qt/plugins/styles/libqmacstyle.dylib
rm ./lib64/qt/plugins/bearer/libqgenericbearer.dylib
rm ./lib64/qt/plugins/iconengines/libqsvgicon.dylib
rm ./lib64/qt/plugins/imageformats/libqgif.dylib
rm ./lib64/qt/plugins/imageformats/libqwbmp.dylib
rm ./lib64/qt/plugins/imageformats/libqwebp.dylib
rm ./lib64/qt/plugins/imageformats/libqico.dylib
rm ./lib64/qt/plugins/imageformats/libqmacheif.dylib
rm ./lib64/qt/plugins/imageformats/libqjpeg.dylib
rm ./lib64/qt/plugins/imageformats/libqtiff.dylib
rm ./lib64/qt/plugins/imageformats/libqsvg.dylib
rm ./lib64/qt/plugins/imageformats/libqicns.dylib
rm ./lib64/qt/plugins/imageformats/libqtga.dylib
rm ./lib64/qt/plugins/imageformats/libqmacjp2.dylib
rm ./lib64/qt/libexec/qtwebengine_resources.pak
rm ./lib64/qt/libexec/QtWebEngineProcess
rm ./lib64/qt/libexec/icudtl.dat
rm ./lib64/qt/libexec/qtwebengine_locales/ar.pak
rm ./lib64/qt/libexec/qtwebengine_locales/en-US.pak
rm ./lib64/qt/libexec/qtwebengine_locales/lt.pak
rm ./lib64/qt/libexec/qtwebengine_locales/tr.pak
rm ./lib64/qt/libexec/qtwebengine_locales/te.pak
rm ./lib64/qt/libexec/qtwebengine_locales/lv.pak
rm ./lib64/qt/libexec/qtwebengine_locales/mr.pak
rm ./lib64/qt/libexec/qtwebengine_locales/zh-TW.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ms.pak
rm ./lib64/qt/libexec/qtwebengine_locales/nl.pak
rm ./lib64/qt/libexec/qtwebengine_locales/bn.pak
rm ./lib64/qt/libexec/qtwebengine_locales/vi.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ta.pak
rm ./lib64/qt/libexec/qtwebengine_locales/zh-CN.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ja.pak
rm ./lib64/qt/libexec/qtwebengine_locales/hi.pak
rm ./lib64/qt/libexec/qtwebengine_locales/en-GB.pak
rm ./lib64/qt/libexec/qtwebengine_locales/pl.pak
rm ./lib64/qt/libexec/qtwebengine_locales/sw.pak
rm ./lib64/qt/libexec/qtwebengine_locales/fa.pak
rm ./lib64/qt/libexec/qtwebengine_locales/el.pak
rm ./lib64/qt/libexec/qtwebengine_locales/sv.pak
rm ./lib64/qt/libexec/qtwebengine_locales/sr.pak
rm ./lib64/qt/libexec/qtwebengine_locales/es-419.pak
rm ./lib64/qt/libexec/qtwebengine_locales/fr.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ru.pak
rm ./lib64/qt/libexec/qtwebengine_locales/gu.pak
rm ./lib64/qt/libexec/qtwebengine_locales/id.pak
rm ./lib64/qt/libexec/qtwebengine_locales/fil.pak
rm ./lib64/qt/libexec/qtwebengine_locales/sk.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ro.pak
rm ./lib64/qt/libexec/qtwebengine_locales/es.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ko.pak
rm ./lib64/qt/libexec/qtwebengine_locales/hu.pak
rm ./lib64/qt/libexec/qtwebengine_locales/kn.pak
rm ./lib64/qt/libexec/qtwebengine_locales/fi.pak
rm ./lib64/qt/libexec/qtwebengine_locales/da.pak
rm ./lib64/qt/libexec/qtwebengine_locales/sl.pak
rm ./lib64/qt/libexec/qtwebengine_locales/de.pak
rm ./lib64/qt/libexec/qtwebengine_locales/it.pak
rm ./lib64/qt/libexec/qtwebengine_locales/hr.pak
rm ./lib64/qt/libexec/qtwebengine_locales/he.pak
rm ./lib64/qt/libexec/qtwebengine_locales/pt-PT.pak
rm ./lib64/qt/libexec/qtwebengine_locales/et.pak
rm ./lib64/qt/libexec/qtwebengine_locales/cs.pak
rm ./lib64/qt/libexec/qtwebengine_locales/nb.pak
rm ./lib64/qt/libexec/qtwebengine_locales/am.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ml.pak
rm ./lib64/qt/libexec/qtwebengine_locales/uk.pak
rm ./lib64/qt/libexec/qtwebengine_locales/bg.pak
rm ./lib64/qt/libexec/qtwebengine_locales/th.pak
rm ./lib64/qt/libexec/qtwebengine_locales/ca.pak
rm ./lib64/qt/libexec/qtwebengine_locales/pt-BR.pak
rm ./lib64/qt/libexec/qtwebengine_resources_200p.pak
rm ./lib64/qt/libexec/qtwebengine_devtools_resources.pak
rm ./lib64/qt/libexec/qtwebengine_resources_100p.pak
rm ./lib64/qt/resources/qtwebengine_resources.pak
rm ./lib64/qt/resources/icudtl.dat
rm ./lib64/qt/resources/qtwebengine_resources_200p.pak
rm ./lib64/qt/resources/qtwebengine_devtools_resources.pak
rm ./lib64/qt/resources/qtwebengine_resources_100p.pak
rm ./lib64/qt/lib/libQt5WebEngineCoreAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5DBusAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5GuiAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5QuickWidgetsAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5WebChannelAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5SvgAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5PrintSupportAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5QmlAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5WidgetsAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5NetworkAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5WebEngineWidgetsAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5QuickAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5CoreAndroidEmu.5.12.1.dylib
rm ./lib64/qt/lib/libQt5WebSocketsAndroidEmu.5.12.1.dylib
rm ./lib64/qt/translations/qtwebengine_locales/ar.pak
rm ./lib64/qt/translations/qtwebengine_locales/en-US.pak
rm ./lib64/qt/translations/qtwebengine_locales/lt.pak
rm ./lib64/qt/translations/qtwebengine_locales/tr.pak
rm ./lib64/qt/translations/qtwebengine_locales/te.pak
rm ./lib64/qt/translations/qtwebengine_locales/lv.pak
rm ./lib64/qt/translations/qtwebengine_locales/mr.pak
rm ./lib64/qt/translations/qtwebengine_locales/zh-TW.pak
rm ./lib64/qt/translations/qtwebengine_locales/ms.pak
rm ./lib64/qt/translations/qtwebengine_locales/nl.pak
rm ./lib64/qt/translations/qtwebengine_locales/bn.pak
rm ./lib64/qt/translations/qtwebengine_locales/vi.pak
rm ./lib64/qt/translations/qtwebengine_locales/ta.pak
rm ./lib64/qt/translations/qtwebengine_locales/zh-CN.pak
rm ./lib64/qt/translations/qtwebengine_locales/ja.pak
rm ./lib64/qt/translations/qtwebengine_locales/hi.pak
rm ./lib64/qt/translations/qtwebengine_locales/en-GB.pak
rm ./lib64/qt/translations/qtwebengine_locales/pl.pak
rm ./lib64/qt/translations/qtwebengine_locales/sw.pak
rm ./lib64/qt/translations/qtwebengine_locales/fa.pak
rm ./lib64/qt/translations/qtwebengine_locales/el.pak
rm ./lib64/qt/translations/qtwebengine_locales/sv.pak
rm ./lib64/qt/translations/qtwebengine_locales/sr.pak
rm ./lib64/qt/translations/qtwebengine_locales/es-419.pak
rm ./lib64/qt/translations/qtwebengine_locales/fr.pak
rm ./lib64/qt/translations/qtwebengine_locales/ru.pak
rm ./lib64/qt/translations/qtwebengine_locales/gu.pak
rm ./lib64/qt/translations/qtwebengine_locales/id.pak
rm ./lib64/qt/translations/qtwebengine_locales/fil.pak
rm ./lib64/qt/translations/qtwebengine_locales/sk.pak
rm ./lib64/qt/translations/qtwebengine_locales/ro.pak
rm ./lib64/qt/translations/qtwebengine_locales/es.pak
rm ./lib64/qt/translations/qtwebengine_locales/ko.pak
rm ./lib64/qt/translations/qtwebengine_locales/hu.pak
rm ./lib64/qt/translations/qtwebengine_locales/kn.pak
rm ./lib64/qt/translations/qtwebengine_locales/fi.pak
rm ./lib64/qt/translations/qtwebengine_locales/da.pak
rm ./lib64/qt/translations/qtwebengine_locales/sl.pak
rm ./lib64/qt/translations/qtwebengine_locales/de.pak
rm ./lib64/qt/translations/qtwebengine_locales/it.pak
rm ./lib64/qt/translations/qtwebengine_locales/hr.pak
rm ./lib64/qt/translations/qtwebengine_locales/he.pak
rm ./lib64/qt/translations/qtwebengine_locales/pt-PT.pak
rm ./lib64/qt/translations/qtwebengine_locales/et.pak
rm ./lib64/qt/translations/qtwebengine_locales/cs.pak
rm ./lib64/qt/translations/qtwebengine_locales/nb.pak
rm ./lib64/qt/translations/qtwebengine_locales/am.pak
rm ./lib64/qt/translations/qtwebengine_locales/ml.pak
rm ./lib64/qt/translations/qtwebengine_locales/uk.pak
rm ./lib64/qt/translations/qtwebengine_locales/bg.pak
rm ./lib64/qt/translations/qtwebengine_locales/th.pak
rm ./lib64/qt/translations/qtwebengine_locales/ca.pak
rm ./lib64/qt/translations/qtwebengine_locales/pt-BR.pak
rm ./lib64/gles_swiftshader/libEGL.dylib
rm ./lib64/gles_swiftshader/libGLES_CM.dylib
rm ./lib64/gles_swiftshader/libGLESv2.dylib
rm ./lib64/vulkan/vk_swiftshader_icd.json
rm ./lib64/vulkan/portability-macos-debug.json
rm ./lib64/vulkan/libvulkan.dylib
rm ./lib64/vulkan/portability-macos.json
rm ./lib64/vulkan/libMoltenVK.dylib
rm ./lib64/vulkan/MoltenVK_icd.json
rm ./lib64/vulkan/libvk_swiftshader.dylib
rm ./lib64/vulkan/libportability_icd-debug.dylib
rm ./lib64/vulkan/libportability_icd.dylib
rm ./lib64/vulkan/glslangValidator
rm ./lib64/libshadertranslator.dylib
rm ./lib64/libOpenglRender.dylib
rm ./LICENSE
rm ./resources/macroPreviews/Reset_position.mp4
rm ./resources/macroPreviews/Walk_to_image_room.mp4
rm ./resources/macroPreviews/Track_vertical_plane.mp4
rm ./resources/macroPreviews/Track_horizontal_plane.mp4
rm ./resources/Toren1BD_Decor.png
rm ./resources/poster.png
rm ./resources/Toren1BD.mtl
rm ./resources/Toren1BD_Main.png
rm ./resources/Toren1BD.posters
rm ./resources/Toren1BD.obj
rm ./resources/macros/Track_vertical_plane
rm ./resources/macros/Reset_position
rm ./resources/macros/Track_horizontal_plane
rm ./resources/macros/Walk_to_image_room
rm ./bin64/fsck.ext4
rm ./bin64/mkfs.ext4
rm ./bin64/tune2fs
rm ./bin64/resize2fs
rm ./bin64/e2fsck
rm ./emulator-check
rm ./perfetto-protozero-protoc-plugin
rm ./qemu/darwin-x86_64/qemu-system*
rm ./emulator64-crash-service
rm ./lib/waterfall.proto
rm ./lib/emulator_controller.proto
rm ./lib/advancedFeaturesCanary.ini
rm ./lib/hardware-properties.ini
rm ./lib/advancedFeatures.ini
rm ./lib/hostapd.conf
rm ./lib/pc-bios/efi-e1000.rom
rm ./lib/pc-bios/kvmvapic.bin
rm ./lib/pc-bios/keymaps/sl
rm ./lib/pc-bios/keymaps/pl
rm ./lib/pc-bios/keymaps/modifiers
rm ./lib/pc-bios/keymaps/sv
rm ./lib/pc-bios/keymaps/da
rm ./lib/pc-bios/keymaps/no
rm ./lib/pc-bios/keymaps/Makefile
rm ./lib/pc-bios/keymaps/ja
rm ./lib/pc-bios/keymaps/lv
rm ./lib/pc-bios/keymaps/it
rm ./lib/pc-bios/keymaps/is
rm ./lib/pc-bios/keymaps/cz
rm ./lib/pc-bios/keymaps/ru
rm ./lib/pc-bios/keymaps/en-gb
rm ./lib/pc-bios/keymaps/bepo
rm ./lib/pc-bios/keymaps/common
rm ./lib/pc-bios/keymaps/pt
rm ./lib/pc-bios/keymaps/fr-ch
rm ./lib/pc-bios/keymaps/fr-ca
rm ./lib/pc-bios/keymaps/mk
rm ./lib/pc-bios/keymaps/ar
rm ./lib/pc-bios/keymaps/hr
rm ./lib/pc-bios/keymaps/pt-br
rm ./lib/pc-bios/keymaps/hu
rm ./lib/pc-bios/keymaps/nl
rm ./lib/pc-bios/keymaps/de
rm ./lib/pc-bios/keymaps/fi
rm ./lib/pc-bios/keymaps/fr
rm ./lib/pc-bios/keymaps/es
rm ./lib/pc-bios/keymaps/et
rm ./lib/pc-bios/keymaps/lt
rm ./lib/pc-bios/keymaps/fo
rm ./lib/pc-bios/keymaps/en-us
rm ./lib/pc-bios/keymaps/de-ch
rm ./lib/pc-bios/keymaps/th
rm ./lib/pc-bios/keymaps/nl-be
rm ./lib/pc-bios/keymaps/fr-be
rm ./lib/pc-bios/keymaps/tr
rm ./lib/pc-bios/multiboot.bin
rm ./lib/pc-bios/vgabios-stdvga.bin
rm ./lib/pc-bios/bios.bin
rm ./lib/pc-bios/vgabios-virtio.bin
rm ./lib/pc-bios/bios-256k.bin
rm ./lib/pc-bios/linuxboot_dma.bin
rm ./lib/pc-bios/linuxboot.bin
rm ./lib/pc-bios/efi-virtio.rom
rm ./lib/pc-bios/efi-e1000e.rom
rm ./lib/pc-bios/vgabios-cirrus.bin
rm ./lib/snapshot_service.proto
rm ./lib/emu-original-feature-flags.protobuf
rm ./lib/snapshot.proto
rm ./lib/ca-bundle.pem
rm ./lib/ui_controller_service.proto
rm ./emulator

# Copy the emulator over and delete the download
echo "cp -r ${EMULATOR_DOWNLOAD_BASE_NAME}/* ." && \
cp -r ${EMULATOR_DOWNLOAD_BASE_NAME}/* .  && \
rm -rf ${EMULATOR_DOWNLOAD_BASE_NAME} && \
rm -rf ${EMULATOR_DOWNLOAD_BASE_NAME}*.zip
