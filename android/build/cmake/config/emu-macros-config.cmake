# Copyright 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

get_filename_component(PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common" ABSOLUTE)
set(EMULATOR_MACROS_DEPENDENCIES
    ${PREBUILT_ROOT}/automation/macros/Move_to_image_room>resources/macros/Move_to_image_room;
    ${PREBUILT_ROOT}/automation/macros/Reset_position>resources/macros/Reset_position;
    ${PREBUILT_ROOT}/automation/macros/Track_horizontal_plane>resources/macros/Track_horizontal_plane;
    ${PREBUILT_ROOT}/automation/macros/Track_vertical_plane>resources/macros/Track_vertical_plane;
    ${PREBUILT_ROOT}/automation/macroPreviews/Move_to_image_room.gif>resources/macroPreviews/Move_to_image_room.gif;
    ${PREBUILT_ROOT}/automation/macroPreviews/Reset_position.gif>resources/macroPreviews/Reset_position.gif;
    ${PREBUILT_ROOT}/automation/macroPreviews/Track_horizontal_plane.gif>resources/macroPreviews/Track_horizontal_plane.gif;
    ${PREBUILT_ROOT}/automation/macroPreviews/Track_vertical_plane.gif>resources/macroPreviews/Track_vertical_plane.gif;
)
set(PACKAGE_EXPORT "EMULATOR_MACROS_DEPENDENCIES")
