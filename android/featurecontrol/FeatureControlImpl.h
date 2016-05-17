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

#pragma once

#include "android/base/StringView.h"
#include "android/featurecontrol/Features.h"

namespace android {

namespace base { class IniFile; }

namespace featurecontrol {

// FeatureControlImpl is the implementation of feature control.
// Please access it through the API in android/featurecontrol/FeatureControl.h

class FeatureControlImpl {
public:
    bool isEnabled(AndroidFeatureControlFeature feature);
    void setEnabledOverride(AndroidFeatureControlFeature feature, bool isEnabled);
    void resetEnabledToDefault(AndroidFeatureControlFeature feature);
    void init(std::string defaultIniPath,
              std::string userIniPath);
    static FeatureControlImpl& get();
    FeatureControlImpl();
private:
    struct FeatureOption {
        AndroidFeatureControlFeature name = static_cast<AndroidFeatureControlFeature>(0);
        bool defaultVal = false;
        bool currentVal = false;
        bool isOverridden = false;
    };
    FeatureOption mFeatures[Feature_n_items] = {};
    void initEnabledDefault(AndroidFeatureControlFeature feature, bool isEnabled);

    void initFeatureAndParseDefault(
        android::base::IniFile& defaultIni,
        AndroidFeatureControlFeature featureName,
        const char* featureNameStr);
    void loadUserOverrideFeature(
        android::base::IniFile& defaultIni,
        AndroidFeatureControlFeature featureName,
        const char* featureNameStr);
};

}  // namespace featurecontrol
}  // namespace android
