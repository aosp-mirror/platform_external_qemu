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
#include "android/crashreport/CrashReporter.h"
#include "android/emulation/ConfigDirs.h"
#include "android/globals.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/system.h"

#include <algorithm>
#include <memory>
#include <stdio.h>
#include <string.h>

enum IniSetting { ON, OFF, DEFAULT, ERR };

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

void FeatureControlImpl::initHostFeatureAndParseDefault(
        android::base::IniFile& defaultIniHost,
        android::featurecontrol::Feature featureName,
        const char* featureNameStr) {
    std::string strHost = defaultIniHost.getString(featureNameStr, "null");

    initEnabledDefault(featureName, false);
    switch (ParseIniStr(strHost)) {
        case ON:
            initEnabledDefault(featureName, true);
            break;
        case OFF:
            break;
        default:
            LOG(WARNING) << "Loading advanced feature host default setting: "
                         << featureNameStr
                         << ", expect on/off, get: " << strHost;
            break;
    }
}

void FeatureControlImpl::initGuestFeatureAndParseDefault(
        android::base::IniFile& defaultIniHost,
        android::base::IniFile& defaultIniGuest,
        android::featurecontrol::Feature featureName,
        const char* featureNameStr) {
    std::string strHost = defaultIniHost.getString(featureNameStr, "null");
    IniSetting valHost = ParseIniStr(strHost);
    IniSetting valGuest = ParseIniStr(
            defaultIniGuest.getString(featureNameStr, "null"));
    initEnabledDefault(featureName, false);
    switch (valHost) {
        case ON:
            if (valGuest == ON) {
                initEnabledDefault(featureName, true);
            }
            break;
        case OFF:
            break;
        default:
            LOG(WARNING) << "Loading advanced feature host default setting: "
                         << featureNameStr
                         << ", expect on/off, get: " << strHost;
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
            LOG(WARNING) << "Loading advanced feature user setting:"
                         << " " << featureNameStr
                         << ", expect on/off/default, get:"
                         << " " << val;
            break;
    }
}

void FeatureControlImpl::init(android::base::StringView defaultIniHostPath,
                              android::base::StringView defaultIniGuestPath,
                              android::base::StringView userIniHostPath,
                              android::base::StringView userIniGuestPath) {
    base::IniFile defaultIniHost(defaultIniHostPath);
    if (defaultIniHost.read()) {
        // Initialize host only features
#define FEATURE_CONTROL_ITEM(item) \
    initHostFeatureAndParseDefault(defaultIniHost, item, #item);
#include "FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

        // Initialize guest features
        // Guest ini read failure is OK and will disable all guest features
        if (base::System::get()->pathCanRead(defaultIniGuestPath)) {
            base::IniFile defaultIniGuest(defaultIniGuestPath);
            if (defaultIniGuest.read()) {
#define FEATURE_CONTROL_ITEM(item)                                         \
                initGuestFeatureAndParseDefault(defaultIniHost, defaultIniGuest, item, #item);
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
            }
        } else {
#define FEATURE_CONTROL_ITEM(item)                                         \
            initEnabledDefault(item, false);
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
        }
    } else {
#define FEATURE_CONTROL_ITEM(item)                                         \
        initEnabledDefault(item, false);
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
        LOG(WARNING) << "Failed to load advanced feature default setting:"
                     << defaultIniHostPath;
    }
    if (base::System::get()->pathCanRead(userIniHostPath)) {
        base::IniFile userIni(userIniHostPath);
        if (userIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
            loadUserOverrideFeature(userIni, item, #item);
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
        }
    }
    if (base::System::get()->pathCanRead(userIniGuestPath)) {
        base::IniFile userIni(userIniGuestPath);
        if (userIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
            loadUserOverrideFeature(userIni, item, #item);
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
        }
    }
}

FeatureControlImpl::FeatureControlImpl() {
    std::string defaultIniHostName =
            base::PathUtils::join(base::System::get()->getLauncherDirectory(),
                                  "lib", "advancedFeatures.ini");
    std::unique_ptr<char[]> defaultIniGuestName;
    if (android_avdInfo) {
        defaultIniGuestName.reset(
                avdInfo_getDefaultSystemFeatureControlPath(android_avdInfo));
    }
    std::string userIniHostName = base::PathUtils::join(
            ConfigDirs::getUserDirectory(), "advancedFeatures.ini");

    // We don't allow for user guest override until we find a use case for it
    std::string userIniGuestName;
    init(defaultIniHostName, defaultIniGuestName.get(), userIniHostName,
         userIniGuestName);

    using android::crashreport::CrashReporter;
    CrashReporter::get()->addCrashCallback([this]() {
        base::ScopedFd file = CrashReporter::get()->openDataAttachFile(
                "feature_control_state.txt");
        for (const FeatureOption& feature : mFeatures) {
            static constexpr char format[] =
                    "Feature: '%s' (%d), value: %d, default: %d, is overridden: %d\n";
            char buffer[sizeof(format) + 10 + 3 + 1 + 1 + 1] = {};
            int count = snprintf(buffer, sizeof(buffer), format,
                                 toString(feature.name).c_str(),
                                 (int)feature.name, feature.currentVal,
                                 feature.defaultVal, feature.isOverridden);
            HANDLE_EINTR(write(file.get(),
                               buffer, std::min<int>(count, sizeof(buffer))));
        }
    });
}

FeatureControlImpl& FeatureControlImpl::get() {
    return s_featureControl.get();
}

bool FeatureControlImpl::isEnabled(Feature feature) const {
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

bool FeatureControlImpl::isOverridden(Feature feature) const {
    const FeatureOption& currFeature = mFeatures[feature];
    return currFeature.isOverridden;
}

void FeatureControlImpl::setIfNotOverriden(Feature feature, bool isEnabled) {
    FeatureOption& currFeature = mFeatures[feature];
    if (currFeature.isOverridden) return;
    currFeature.currentVal = isEnabled;
}

Feature FeatureControlImpl::fromString(base::StringView str) {

#define FEATURE_CONTROL_ITEM(item) if (str == #item) return item;
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM

    return Feature::Feature_n_items;
}

base::StringView FeatureControlImpl::toString(Feature feature) {

#define FEATURE_CONTROL_ITEM(item) if (feature == Feature::item) return #item;
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM

    return "UnknownFeature";
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
