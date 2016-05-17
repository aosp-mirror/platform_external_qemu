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

#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/featurecontrol/FeatureControlImpl.h"

#include <assert.h>
#include <string.h>

namespace android {
namespace featurecontrol {

bool isEnabled(Feature feature) {
    FeatureControlImpl* fcImpl = FeatureControlImpl::get();
    if (!fcImpl) {
        LOG(WARNING) << "Feature control not initialized.";
        return false;
    }
    return fcImpl->isEnabled(feature);
}

void setEnabledOverride(Feature feature, bool isEnabled) {
    FeatureControlImpl* fcImpl = FeatureControlImpl::get();
    assert(fcImpl);
    fcImpl->setEnabledOverride(feature, isEnabled);
}

void setEnabledToDefault(Feature feature) {
    FeatureControlImpl* fcImpl = FeatureControlImpl::get();
    assert(fcImpl);
    fcImpl->resetEnabledToDefault(feature);
}

void initFeatureControl() {
    std::string defaultIniName = base::PathUtils::join(
            base::System::get()->getLauncherDirectory(), "advancedFeatures.ini",
            base::PathUtils::HOST_TYPE);
    std::string userIniName = base::PathUtils::join(
            ConfigDirs::getUserDirectory(), "advancedFeatures.ini",
            base::PathUtils::HOST_TYPE);
    FeatureControlImpl::init(defaultIniName, userIniName);
}

}  // namespace featurecontrol
}  // namespace android

extern "C" void featureControl_init() {
    android::featurecontrol::initFeatureControl();
}
