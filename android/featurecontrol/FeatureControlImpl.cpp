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

#include "FeatureControlImpl.h"

#include "android/base/files/IniFile.h"
#include "android/base/Log.h"

using namespace android;
using namespace featurecontrol;

namespace {
enum IniSetting {ON, OFF, DEFAULT, ERR};
}

static FeatureControlImpl* s_featureControl = nullptr;

static IniSetting ParseIniStr(const std::string& str) {
    if (str == "on") {
        return ON;
    } else if (str == "off") {
        return OFF;
    } else if (str == "default") {
        return DEFAULT;
    } else {
        return ERR;
    }
}

static inline void initFeatureAndParseDefault(base::IniFile& defaultIni,
        Feature featureName,
        const char* featureNameStr,
        FeatureControlImpl::FeatureOption& featureOption) {
    featureOption.name = featureName;
    std::string val = defaultIni.getString(featureNameStr, "null");
    switch (ParseIniStr(val)) {
    case ON:
        featureOption.defaultVal = true;
        break;
    case OFF:
        featureOption.defaultVal = false;
        break;
    default:
        LOG(WARNING) << "Loading advanced feature default setting: "
                     << featureNameStr << ", expect on/off, get: " << val;
        break;
    }
    featureOption.isOverridden = false;
    featureOption.currentVal = featureOption.defaultVal;
}

static inline void loadUserOverrideFeature(base::IniFile& userIni,
        Feature featureName,
        const char* featureNameStr) {
    std::string val = userIni.getString(featureNameStr, "default");
    switch (ParseIniStr(val)) {
    case ON:
        s_featureControl->setEnabledOverride(featureName, true);
        break;
    case OFF:
        s_featureControl->setEnabledOverride(featureName, false);
        break;
    case DEFAULT:
        s_featureControl->setEnabledToDefault(featureName);
        break;
    default:
        LOG(WARNING) << "Loading advanced feature user setting: "
                << featureNameStr << ", expect on/off/default, get: " << val;
        break;
    }
}

FeatureControlImpl::FeatureControlImpl() {}

void FeatureControlImpl::init(android::base::StringView defaultIniPath,
                         android::base::StringView userIniPath) {

    s_featureControl = new FeatureControlImpl();

    base::IniFile defaultIni(defaultIniPath);
    if (defaultIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
        initFeatureAndParseDefault(defaultIni, item, #item, s_featureControl->mFeatures[item]);
#   include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
    } else {
        LOG(WARNING) << "Failed to load advanced feature default setting:"
                << defaultIniPath;
    }

    base::IniFile userIni(userIniPath);
    if (userIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
        loadUserOverrideFeature(userIni, item, #item);
#   include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
    }
}

FeatureControlImpl* FeatureControlImpl::get() {
    return s_featureControl;
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
