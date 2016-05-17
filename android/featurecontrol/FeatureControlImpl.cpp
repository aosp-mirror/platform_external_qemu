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
#include "android/base/files/PathUtils.h"
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"
#include "android/emulation/ConfigDirs.h"
#include "android/utils/system.h"

#include <string.h>

namespace {
enum IniSetting { ON, OFF, DEFAULT, ERR };
}

static android::base::LazyInstance<android::featurecontrol::FeatureControlImpl>
        s_featureControl = LAZY_INSTANCE_INIT;

static IniSetting ParseIniStr(const std::string& str) {
    if (strcasecmp(str.c_str(), "on") == 0) {
        return ON;
    } else if (strcasecmp(str.c_str(), "off") == 0) {
        return OFF;
    } else if (strcasecmp(str.c_str(), "default") == 0) {
        return DEFAULT;
    } else {
        return ERR;
    }
}

namespace android {
namespace featurecontrol {

void FeatureControlImpl::initFeatureAndParseDefault(
        android::base::IniFile& defaultIni,
        android::featurecontrol::Feature featureName,
        const char* featureNameStr) {
    std::string val = defaultIni.getString(featureNameStr, "null");
    switch (ParseIniStr(val)) {
        case ON:
            initEnabledDefault(featureName, true);
            break;
        case OFF:
            initEnabledDefault(featureName, false);
            break;
        default:
            LOG(WARNING) << "Loading advanced feature default setting: "
                         << featureNameStr << ", expect on/off, get: " << val;
            initEnabledDefault(featureName, false);
            break;
    }
}

void FeatureControlImpl::loadUserOverrideFeature(
        android::base::IniFile& userIni,
        android::featurecontrol::Feature featureName,
        const char* featureNameStr) {
    std::string val = userIni.getString(featureNameStr, "default");
    switch (ParseIniStr(val)) {
        case ON:
            setEnabledOverride(featureName, true);
            break;
        case OFF:
            setEnabledOverride(featureName, false);
            break;
        case DEFAULT:
            resetEnabledToDefault(featureName);
            break;
        default:
            LOG(WARNING) << "Loading advanced feature user setting: "
                         << featureNameStr
                         << ", expect on/off/default, get: " << val;
            break;
    }
}

void FeatureControlImpl::init(android::base::StringView defaultIniPath,
                              android::base::StringView userIniPath) {
    base::IniFile defaultIni(defaultIniPath);
    if (defaultIni.read()) {
#define FEATURE_CONTROL_ITEM(item)                                         \
        initFeatureAndParseDefault(defaultIni, item, #item);
#include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
    } else {
#define FEATURE_CONTROL_ITEM(item)                                         \
        initEnabledDefault(item, false);
#include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
        LOG(WARNING) << "Failed to load advanced feature default setting:"
                     << defaultIniPath;
    }

    base::IniFile userIni(userIniPath);
    if (userIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
        loadUserOverrideFeature(userIni, item, #item);
#include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
    }
}

FeatureControlImpl::FeatureControlImpl() {
    std::string defaultIniName = base::PathUtils::join(
            base::System::get()->getLauncherDirectory(), "lib",
            "advancedFeatures.ini", base::PathUtils::HOST_TYPE);
    std::string userIniName = base::PathUtils::join(
            ConfigDirs::getUserDirectory(), "advancedFeatures.ini",
            base::PathUtils::HOST_TYPE);
    init(defaultIniName, userIniName);
}

FeatureControlImpl& FeatureControlImpl::get() {
    return s_featureControl.get();
}

bool FeatureControlImpl::isEnabled(Feature feature) {
    return mFeatures[feature].currentVal;
}

void FeatureControlImpl::setEnabledOverride(Feature feature, bool isEnabled) {
    FeatureOption& currFeature = mFeatures[feature];
    currFeature.currentVal = isEnabled;
    currFeature.isOverridden = true;
}

void FeatureControlImpl::resetEnabledToDefault(Feature feature) {
    FeatureOption& currFeature = mFeatures[feature];
    currFeature.currentVal = currFeature.defaultVal;
    currFeature.isOverridden = false;
}

void FeatureControlImpl::initEnabledDefault(Feature feature, bool isEnabled) {
    FeatureOption& currFeature = mFeatures[feature];
    currFeature.name = feature;
    currFeature.defaultVal = isEnabled;
    currFeature.currentVal = isEnabled;
    currFeature.isOverridden = false;
}

}  // namespace featurecontrol
}  // namespace android
