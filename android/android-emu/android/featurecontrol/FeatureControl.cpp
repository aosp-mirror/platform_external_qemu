// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "FeatureControl.h"

#include "android/featurecontrol/FeatureControlImpl.h"

namespace android {
namespace featurecontrol {

bool isEnabled(Feature feature) {
    return FeatureControlImpl::get().isEnabled(feature);
}

void setEnabledOverride(Feature feature, bool isEnabled) {
    FeatureControlImpl::get().setEnabledOverride(feature, isEnabled);
}

void resetEnabledToDefault(Feature feature) {
    FeatureControlImpl::get().resetEnabledToDefault(feature);
}

Feature stringToFeature(const std::string& str) {
    return FeatureControlImpl::get().fromString(str);
}

}  // namespace featurecontrol
}  // namespace android
