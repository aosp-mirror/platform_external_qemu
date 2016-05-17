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
#include "android/base/files/IniFile.h"

#include <string.h>

using namespace android;
using namespace featurecontrol;

namespace {
    class FeatureControlImpl {
    public:
        bool isEnabled(Feature feature);
        void setEnabledOverride(Feature feature, bool isEnabled);
        void setEnabledToDefault(Feature feature);
        static void init();

        struct FeatureOption {
            Feature name;
            bool defaultVal;
            bool currentVal;
            bool isOverridden;
        };
    private:
        FeatureOption mFeatures[Feature_n_items];
        FeatureControlImpl();
    };
}

static FeatureControlImpl* s_featureControl = nullptr;

static inline void initFeatureAndParseDefault(base::IniFile& defaultIni,
        Feature featureName,
        const char* featureNameStr,
        FeatureControlImpl::FeatureOption& featureOption) {
    featureOption.name = featureName;
    std::string val = defaultIni.getString(featureNameStr, "null");
    if (val == "on") {
        featureOption.defaultVal = true;
    } else {
        featureOption.defaultVal = false;
        if (val != "off") {
            LOG(WARNING) << "Loading advanced feature default setting: "
                         << featureNameStr << ", expect on/off, get: " << val
                         << "\n";
        }
    }
    featureOption.isOverridden = false;
    featureOption.currentVal = featureOption.defaultVal;
}

static inline void loadUserOverrideFeature(base::IniFile& userIni,
        Feature featureName,
        const char* featureNameStr) {
    std::string val = userIni.getString(featureNameStr, "auto");
    if (val == "on") {
        android::featurecontrol::setEnabledOverride(featureName, true);
    } else if (val == "off") {
        android::featurecontrol::setEnabledOverride(featureName, false);
    } else if (val == "default") {
        android::featurecontrol::setEnabledToDefault(featureName);
    } else {
        LOG(WARNING) << "Loading advanced feature user setting: "
                << featureNameStr << ", expect on/off/default, get: " << val
                << "\n";
    }
}

FeatureControlImpl::FeatureControlImpl() {}

void FeatureControlImpl::init() {

    s_featureControl = new FeatureControlImpl();

    std::string defaultIniName = ConfigDirs::getSdkRootDirectory()
                                 + "/tools/advancedFeatures.ini";
    base::IniFile defaultIni(defaultIniName);
    if (defaultIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
        initFeatureAndParseDefault(defaultIni, item, #item, s_featureControl->mFeatures[item]);
#   include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
    } else {
        LOG(WARNING) << "Failed to load advanced feature default setting:"
                << defaultIniName << "\n";
    }

    std::string userIniName = ConfigDirs::getUserDirectory()
                              + "/advancedFeatures.ini";
    base::IniFile userIni(userIniName);
    if (userIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
        loadUserOverrideFeature(userIni, item, #item);
#   include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
    }
}

bool FeatureControlImpl::isEnabled(Feature feature) {
    return mFeatures[feature].currentVal;
}

void FeatureControlImpl::setEnabledOverride(Feature feature, bool isEnabled) {
    FeatureOption& currFeature = mFeatures[feature];
    currFeature.currentVal = isEnabled;
    currFeature.isOverridden = true;
}

void FeatureControlImpl::setEnabledToDefault(Feature feature) {
    FeatureOption& currFeature = mFeatures[feature];
    currFeature.currentVal = currFeature.defaultVal;
    currFeature.isOverridden = false;
}

bool android::featurecontrol::isEnabled(Feature feature) {
    if (!s_featureControl) {
        LOG(WARNING) << "Feature control not initialized.\n";
        return false;
    }   
    return s_featureControl->isEnabled(feature);
}

void android::featurecontrol::setEnabledOverride(Feature feature, bool isEnabled) {
    s_featureControl->setEnabledOverride(feature, isEnabled);
}

void android::featurecontrol::setEnabledToDefault(Feature feature) {
    s_featureControl->setEnabledToDefault(feature);
}

void android::featurecontrol::initFeatureControl() {
    FeatureControlImpl::init();
}

extern "C" void featureControl_init() {
    initFeatureControl();
}