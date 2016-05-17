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

#include "android/base/Log.h"
#include "android/emulation/ConfigDirs.h"
#include "android/featurecontrol/FeatureControlImpl.h"

#include <string.h>

using namespace android;
using namespace featurecontrol;

bool android::featurecontrol::isEnabled(Feature feature) {
    FeatureControlImpl* fcImpl = FeatureControlImpl::get();
    if (!fcImpl) {
        LOG(WARNING) << "Feature control not initialized.";
        return false;
    }
    return fcImpl->isEnabled(feature);
}

void android::featurecontrol::setEnabledOverride(Feature feature, bool isEnabled) {
    FeatureControlImpl::get()->setEnabledOverride(feature, isEnabled);
}

void android::featurecontrol::setEnabledToDefault(Feature feature) {
    FeatureControlImpl::get()->setEnabledToDefault(feature);
}

void android::featurecontrol::initFeatureControl() {
    std::string defaultIniName = ConfigDirs::getSdkRootDirectory()
                                 + "/tools/advancedFeatures.ini";
    std::string userIniName = ConfigDirs::getUserDirectory()
                              + "/advancedFeatures.ini";
    FeatureControlImpl::init(defaultIniName, userIniName);
}

extern "C" void featureControl_init() {
    initFeatureControl();
}