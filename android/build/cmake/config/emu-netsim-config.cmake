# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

if(INCLUDE_NETSIM_CFG_CMAKE)
  return()
endif()
set(INCLUDE_NETSIM_CFG_CMAKE 1)

get_filename_component(
  PREBUILT_ROOT
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/netsim/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

# Add netsim-ui into objs
set(NETSIM_UI_DEPENDENCIES
    ${PREBUILT_ROOT}/netsim-ui/index.html>netsim-ui/index.html;
    ${PREBUILT_ROOT}/netsim-ui/dev.html>netsim-ui/dev.html;
    ${PREBUILT_ROOT}/netsim-ui/assets/grid-background.svg>netsim-ui/assets/grid-background.svg;
    ${PREBUILT_ROOT}/netsim-ui/assets/hexagonal-background.png>netsim-ui/assets/hexagonal-background.png;
    ${PREBUILT_ROOT}/netsim-ui/assets/netsim-logo-b.svg>netsim-ui/assets/netsim-logo-b.svg;
    ${PREBUILT_ROOT}/netsim-ui/assets/netsim-logo.svg>netsim-ui/assets/netsim-logo.svg;
    ${PREBUILT_ROOT}/netsim-ui/assets/polar-background.svg>netsim-ui/assets/polar-background.svg;
    ${PREBUILT_ROOT}/netsim-ui/js/cube-sprite.js>netsim-ui/js/cube-sprite.js;
    ${PREBUILT_ROOT}/netsim-ui/js/customize-map-button.js>netsim-ui/js/customize-map-button.js;
    ${PREBUILT_ROOT}/netsim-ui/js/device-dragzone.js>netsim-ui/js/device-dragzone.js;
    ${PREBUILT_ROOT}/netsim-ui/js/device-dropzone.js>netsim-ui/js/device-dropzone.js;
    ${PREBUILT_ROOT}/netsim-ui/js/device-info.js>netsim-ui/js/device-info.js;
    ${PREBUILT_ROOT}/netsim-ui/js/device-list.js>netsim-ui/js/device-list.js;
    ${PREBUILT_ROOT}/netsim-ui/js/device-map.js>netsim-ui/js/device-map.js;
    ${PREBUILT_ROOT}/netsim-ui/js/device-observer.js>netsim-ui/js/device-observer.js;
    ${PREBUILT_ROOT}/netsim-ui/js/license-info.js>netsim-ui/js/license-info.js;
    ${PREBUILT_ROOT}/netsim-ui/js/navigation-bar.js>netsim-ui/js/navigation-bar.js;
    ${PREBUILT_ROOT}/netsim-ui/js/netsim-app.js>netsim-ui/js/netsim-app.js;
    ${PREBUILT_ROOT}/netsim-ui/js/packet-info.js>netsim-ui/js/packet-info.js;
    ${PREBUILT_ROOT}/netsim-ui/js/pyramid-sprite.js>netsim-ui/js/pyramid-sprite.js;
    ${PREBUILT_ROOT}/netsim-ui/node_modules/tslib/tslib.es6.js>netsim-ui/node_modules/tslib/tslib.es6.js;
)

if(WINDOWS_MSVC_X86_64)
  set(NETSIM_DEPENDENCIES
      ${PREBUILT_ROOT}/netsim.exe>netsim.exe;${PREBUILT_ROOT}/netsimd.exe>netsimd.exe;
  )

install(PROGRAMS ${PREBUILT_ROOT}/netsim.exe ${PREBUILT_ROOT}/netsimd.exe DESTINATION .)
else()
  set(NETSIM_DEPENDENCIES
      ${PREBUILT_ROOT}/netsim>netsim;${PREBUILT_ROOT}/netsimd>netsimd;)

install(PROGRAMS ${PREBUILT_ROOT}/netsim ${PREBUILT_ROOT}/netsimd DESTINATION .)
endif()

set(PACKAGE_EXPORT NETSIM_DEPENDENCIES;NETSIM_UI_DEPENDENCIES)
android_license(
  TARGET "NETSIM_DEPENDENCIES"
  LIBNAME "Netsim"
  SPDX "Apache-2.0"
  LICENSE
    "https://android.googlesource.com/platform/tools/netsim/+/refs/heads/main/LICENSE"
  LOCAL ${ANDROID_QEMU2_TOP_DIR}/../../tools/netsim/LICENSE)

android_license(
  TARGET "NETSIM_UI_DEPENDENCIES"
  LIBNAME "Netsim Web UI"
  SPDX "Apache-2.0"
  LICENSE
    "https://android.googlesource.com/platform/tools/netsim/+/refs/heads/main/LICENSE"
  LOCAL ${ANDROID_QEMU2_TOP_DIR}/../../tools/netsim/LICENSE)
