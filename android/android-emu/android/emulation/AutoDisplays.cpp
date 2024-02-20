/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AutoDisplays.h"
#include "android/avd/info.h"
#include "aemu/base/Log.h"

namespace android {
namespace automotive
{
int getDefaultFlagsForDisplay(int displayId) {
    return displayId == 1 ? DEFAULT_FLAGS_AUTO_CLUSTER : DEFAULT_FLAGS_AUTO;
}
bool isMultiDisplaySupported(const AvdInfo* info) {
    return avdInfo_getBuildPropertyBool(info, kMultiDisplayProp, false);
}
bool isDistantDisplaySupported(const AvdInfo* info) {
    return avdInfo_getBuildPropertyBool(info, kDistantDisplayProp, false);
}
} // namespace automotive
} // namespace android