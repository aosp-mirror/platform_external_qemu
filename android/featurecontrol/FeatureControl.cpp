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
#include "android/utils/ini.h"

#include <string.h>

static android::FeatureControl* s_featureControl = nullptr;

static inline void initFeatureAndParseDefault(CIniFile* defaultIni,
        android::FeatureControl::Feature featureName,
        const char* featureNameStr,
        android::FeatureControl::FeatureOption& featureOption) {
    featureOption.name = featureName;
    const char* val = iniFile_getString(defaultIni, featureNameStr, "null");
    if (!strcmp("on", val)) {
        featureOption.defaultVal = true;
    } else {
        featureOption.defaultVal = false;
        if (strcmp("off", val)) {
            LOG(WARNING) << "Loading advanced feature default setting: "
                         << featureNameStr << ", expect on/off, get: " << val
                         << "\n";
        }
    }
    featureOption.isOverridden = false;
    featureOption.currentVal = featureOption.defaultVal;
}

static inline void loadUserOverrideFeature(CIniFile* userIni,
        android::FeatureControl::Feature featureName,
        const char* featureNameStr) {
    const char* val = iniFile_getString(userIni, featureNameStr, "auto");
    if (!strcmp("on", val)) {
        android::FeatureControl::setEnabledOverride(featureName, true);
    } else if (!strcmp("off", val)) {
        android::FeatureControl::setEnabledOverride(featureName, false);
    } else if (!strcmp("default", val)) {
        android::FeatureControl::setEnabledToDefault(featureName);
    } else {
        LOG(WARNING) << "Loading advanced feature user setting: "
                << featureNameStr << ", expect on/off/default, get: " << val
                << "\n";
    }
}

namespace android {

FeatureControl::FeatureControl() {}

void FeatureControl::init() {

    s_featureControl = new FeatureControl();

    std::string defaultIniName = ConfigDirs::getSdkRootDirectory()
                                 + "/tools/advancedFeatures.ini";
    CIniFile* defaultIni = iniFile_newFromFile(defaultIniName.c_str());

#define FEATURE_CONTROL_ITEM(item) \
        initFeatureAndParseDefault(defaultIni, item, #item, s_featureControl->mFeatures[item]);
#   include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM

    iniFile_free(defaultIni);

    std::string userIniName = ConfigDirs::getUserDirectory()
                              + "/advancedFeatures.ini";
    FILE* fid = fopen(userIniName.c_str(), "r");
    if (fid) {
        fclose(fid);
        CIniFile* userIni = iniFile_newFromFile(userIniName.c_str());

#define FEATURE_CONTROL_ITEM(item) \
        loadUserOverrideFeature(userIni, item, #item);
#   include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM

        iniFile_free(userIni);
    }
}

bool FeatureControl::isEnabled(Feature feature) {
    if (!s_featureControl) {
        LOG(WARNING) << "Feature control not initialized.\n";
        return false;
    }
    return s_featureControl->mFeatures[feature].currentVal;
}

void FeatureControl::setEnabledOverride(Feature feature, bool isEnabled) {
    FeatureOption& featureoption = s_featureControl->mFeatures[feature];
    featureoption.currentVal = isEnabled;
    featureoption.isOverridden = true;
}

void FeatureControl::setEnabledToDefault(Feature feature) {
    FeatureOption& featureoption = s_featureControl->mFeatures[feature];
    featureoption.currentVal = featureoption.defaultVal;
    featureoption.isOverridden = false;
}

}

extern "C" void featureControl_init() {
    android::FeatureControl::init();
}