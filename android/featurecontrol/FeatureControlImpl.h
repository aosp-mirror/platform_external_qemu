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
    bool isEnabled(Feature feature);
    void setEnabledOverride(Feature feature, bool isEnabled);
    void resetEnabledToDefault(Feature feature);
    void init(android::base::StringView defaultIniPath,
              android::base::StringView userIniPath);
    static FeatureControlImpl& get();
    FeatureControlImpl();
private:
    struct FeatureOption {
        Feature name = static_cast<Feature>(0);
        bool defaultVal = false;
        bool currentVal = false;
        bool isOverridden = false;
    };
    FeatureOption mFeatures[Feature_n_items] = {};
    void initEnabledDefault(Feature feature, bool isEnabled);

    void initFeatureAndParseDefault(
        android::base::IniFile& defaultIni,
        Feature featureName,
        const char* featureNameStr);
    void loadUserOverrideFeature(
        android::base::IniFile& defaultIni,
        Feature featureName,
        const char* featureNameStr);
};

}  // namespace featurecontrol
}  // namespace android
