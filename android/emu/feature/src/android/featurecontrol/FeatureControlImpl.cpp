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

#include "android/featurecontrol/FeatureControlImpl.h"

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "host-common/hw-config.h"
#include "android/avd/info.h"
#include "aemu/base/Log.h"
#include "aemu/base/files/IniFile.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/emulation/ConfigDirs.h"
#include "android/metrics/StudioConfig.h"
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <memory>
#include <ostream>
#include <set>
#include <string_view>
#include <unordered_set>

using android::base::ScopedCPtr;


enum IniSetting { ON, OFF, DEFAULT, NULLVAL, ERR };
static constexpr char kIniSettingNull[] = "null";
static constexpr char kIniSettingDefault[] = "default";

static android::base::LazyInstance<android::featurecontrol::FeatureControlImpl>
        s_featureControl = LAZY_INSTANCE_INIT;

#define FEATURE_CONTROL_ITEM(item) #item,
static const std::unordered_set<std::string> kExpectedGuestFeatures = {
#include "host-common/FeatureControlDefGuest.h"
};
#undef FEATURE_CONTROL_ITEM

static IniSetting ParseIniStr(const std::string& str) {
    if (strcasecmp(str.c_str(), "on") == 0) {
        return ON;
    } else if (strcasecmp(str.c_str(), "off") == 0) {
        return OFF;
    } else if (strcasecmp(str.c_str(), kIniSettingDefault) == 0) {
        return DEFAULT;
    } else if (strcasecmp(str.c_str(), kIniSettingNull) == 0) {
        return NULLVAL;
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
    std::string strHost =
            defaultIniHost.getString(featureNameStr, kIniSettingNull);

    initEnabledDefault(featureName, false);
    switch (ParseIniStr(strHost)) {
        case ON:
            initEnabledDefault(featureName, true);
            break;
        case OFF:
        case NULLVAL:
            break;
        default:
            dwarning(
                    "Loading advanced feature host default setting: %s, expect "
                    "on/off, get: %s",
                    featureNameStr, strHost.c_str());
            break;
    }
}

void FeatureControlImpl::initGuestFeatureAndParseDefault(
        android::base::IniFile& defaultIniHost,
        android::base::IniFile& defaultIniGuest,
        android::featurecontrol::Feature featureName,
        const char* featureNameStr) {
    std::string strHost =
            defaultIniHost.getString(featureNameStr, kIniSettingNull);
    IniSetting valHost = ParseIniStr(strHost);
    IniSetting valGuest = ParseIniStr(
            defaultIniGuest.getString(featureNameStr, kIniSettingNull));
    if (valGuest == ON) {
        setGuestTriedEnable(featureName);
    }

    initEnabledDefault(featureName, false);
    switch (valHost) {
        case ON:
            if (valGuest == ON) {
                initEnabledDefault(featureName, true);
            }
            break;
        case OFF:
        case NULLVAL:
            break;
        default:
            dwarning(
                    "Loading advanced feature host default setting: %s, expect "
                    "on/off, get: %s",
                    featureNameStr, strHost.c_str());
            break;
    }
}

void FeatureControlImpl::loadUserOverrideFeature(
        android::base::IniFile& userIni,
        android::featurecontrol::Feature featureName,
        const char* featureNameStr) {
    std::string val = userIni.getString(featureNameStr, kIniSettingDefault);
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
        case NULLVAL:
            break;
        default:
            LOG(WARNING) << "Loading advanced feature user setting:"
                         << " " << featureNameStr
                         << ", expect on/off/default, get:"
                         << " " << val;
            break;
    }
}

void FeatureControlImpl::init(const std::string& defaultIniHostPath,
                              const std::string& defaultIniGuestPath,
                              const std::string& userIniHostPath,
                              const std::string& userIniGuestPath) {
    memset(mGuestTriedEnabledFeatures, 0,
           sizeof(FeatureOption) * Feature_n_items);
    base::IniFile defaultIniHost(defaultIniHostPath);
    if (defaultIniHost.read()) {
        // Initialize host only features
#define FEATURE_CONTROL_ITEM(item) \
    initHostFeatureAndParseDefault(defaultIniHost, item, #item);
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

        // Initialize guest features
        // Guest ini read failure is OK and will disable all guest features
        if (base::System::get()->pathCanRead(defaultIniGuestPath)) {
            base::IniFile defaultIniGuest(defaultIniGuestPath);
            if (defaultIniGuest.read()) {
                std::set<std::string> unexpectedGuestFeatures = {};
                for (const auto& guestFeature : defaultIniGuest) {
                    if (!kExpectedGuestFeatures.count(guestFeature)) {
                        unexpectedGuestFeatures.insert(guestFeature);
                    }
                }
                if (!unexpectedGuestFeatures.empty()) {
                    std::string missing =
                            absl::StrJoin(unexpectedGuestFeatures, ", ");
                    dwarning(
                            "Please update the emulator to one that supports "
                            "the feature(s): %s",
                            missing.c_str());
                }
#define FEATURE_CONTROL_ITEM(item)                                         \
    initGuestFeatureAndParseDefault(defaultIniHost, defaultIniGuest, item, \
                                    #item);
#include "host-common/FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
            }
        } else {
#define FEATURE_CONTROL_ITEM(item) initEnabledDefault(item, false);
#include "host-common/FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
        }
    } else {
#define FEATURE_CONTROL_ITEM(item) initEnabledDefault(item, false);
#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM
        LOG(WARNING) << "Failed to load advanced feature default setting:"
                     << defaultIniHostPath;
    }
    if (base::System::get()->pathCanRead(userIniHostPath)) {
        base::IniFile userIni(userIniHostPath);
        if (userIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
    loadUserOverrideFeature(userIni, item, #item);
#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM
        }
    }
    if (base::System::get()->pathCanRead(userIniGuestPath)) {
        base::IniFile userIni(userIniGuestPath);
        if (userIni.read()) {
#define FEATURE_CONTROL_ITEM(item) \
    loadUserOverrideFeature(userIni, item, #item);
#include "host-common/FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
        }
    }

    // Apply overrides from the hw config.
    if (getConsoleAgents()->settings->hw()->hw_featureflags) {
        parseAndApplyOverrides(
                getConsoleAgents()->settings->hw()->hw_featureflags);
    }

    // Enumerate the command line and environment variables to add overrides.
    const auto envVar =
            android::base::System::get()->envGet("ANDROID_EMULATOR_FEATURES");
    if (!envVar.empty()) {
        parseAndApplyOverrides(envVar);
    }
    if (getConsoleAgents()->settings->has_cmdLineOptions()) {
        if (const ParamList* feature =
                    getConsoleAgents()
                            ->settings->android_cmdLineOptions()
                            ->feature) {
            do {
                parseAndApplyOverrides(feature->param);
                feature = feature->next;
            } while (feature);
        }
    }
}

void FeatureControlImpl::initNoFiles() {
    memset(mGuestTriedEnabledFeatures, 0,
           sizeof(FeatureOption) * Feature_n_items);

    // Apply overrides from the hw config.
    if (getConsoleAgents()->settings->hw()->hw_featureflags) {
        parseAndApplyOverrides(
                getConsoleAgents()->settings->hw()->hw_featureflags);
    }

    // Enumerate the command line and environment variables to add overrides.
    const auto envVar =
            android::base::System::get()->envGet("ANDROID_EMULATOR_FEATURES");
    if (!envVar.empty()) {
        parseAndApplyOverrides(envVar);
    }

    if (getConsoleAgents()->settings->has_cmdLineOptions()) {
        if (const ParamList* feature =
                    getConsoleAgents()
                            ->settings->android_cmdLineOptions()
                            ->feature) {
            do {
                parseAndApplyOverrides(feature->param);
                feature = feature->next;
            } while (feature);
        }
    }
}

void FeatureControlImpl::create() {
    if (s_featureControl.hasInstance()) {
        derror("Feature control already exists in create() call");
    }
    (void)s_featureControl.get();
}

FeatureControlImpl::FeatureControlImpl() {
    if (base::System::getEnvironmentVariable("ANDROID_EMU_SANDBOX") == "1") {
        initNoFiles();
        return;
    }

    const auto updateChannel = studio::updateChannel();

    std::string defaultIniHostName;

    if (updateChannel == studio::UpdateChannel::Stable) {
        defaultIniHostName = base::PathUtils::join(
                base::System::get()->getLauncherDirectory(), "lib",
                "advancedFeatures.ini");
    } else {
        // TODO(jansene): If we ever use beta/dev channels, disambiguate them
        // with separate files here.
        defaultIniHostName = base::PathUtils::join(
                base::System::get()->getLauncherDirectory(), "lib",
                "advancedFeaturesCanary.ini");
    }

    std::string defaultIniGuestName;
    if (getConsoleAgents()->settings->avdInfo()) {
        const char* gname = avdInfo_getDefaultSystemFeatureControlPath(
                getConsoleAgents()->settings->avdInfo());
        if (gname) {
            defaultIniGuestName = gname;
            free((void*)gname);
        }
    }
    std::string userIniHostName = base::PathUtils::join(
            ConfigDirs::getUserDirectory(), "advancedFeatures.ini");

    // We don't allow for user guest override until we find a use case for it
    std::string userIniGuestName;
    init(defaultIniHostName, defaultIniGuestName, userIniHostName,
         userIniGuestName);
}

std::ostream& operator<<(std::ostream& os,
                         const FeatureControlImpl::FeatureOption& feature) {
    os << absl::StrFormat(
            "Feature: '%s' (%d), value: %d, default: %d, is "
            "overridden: %d",
            FeatureControlImpl::toString(feature.name),
            static_cast<int>(feature.name), feature.currentVal,
            feature.defaultVal, feature.isOverridden);
    return os;
}

void FeatureControlImpl::writeFeaturesToStream(std::ostream& os) const {
    for (const FeatureOption& opt : mFeatures) {
        os << opt << std::endl;
    }
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

bool FeatureControlImpl::isGuestFeature(Feature feature) const {
#define FEATURE_CONTROL_ITEM(item) \
    if (feature == Feature::item)  \
        return true;
#include "host-common/FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
    return false;
}

bool FeatureControlImpl::isEnabledByGuest(Feature feature) const {
    return mGuestTriedEnabledFeatures[feature].currentVal;
}

void FeatureControlImpl::setIfNotOverriden(Feature feature, bool isEnabled) {
    FeatureOption& currFeature = mFeatures[feature];
    if (currFeature.isOverridden) {
        return;
    }
    currFeature.currentVal = isEnabled;
}

void FeatureControlImpl::setIfNotOverridenOrGuestDisabled(Feature feature,
                                                          bool isEnabled) {
    FeatureOption& currFeature = mFeatures[feature];
    if (currFeature.isOverridden) {
        return;
    }
    if (isGuestFeature(feature) && !isEnabledByGuest(feature)) {
        return;
    }

    currFeature.currentVal = isEnabled;
}

Feature FeatureControlImpl::fromString(std::string_view str) {
#define FEATURE_CONTROL_ITEM(item) \
    if (str == #item)              \
        return item;
#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

    return Feature::Feature_n_items;
}

std::string_view FeatureControlImpl::toString(Feature feature) {
#define FEATURE_CONTROL_ITEM(item) \
    if (feature == Feature::item)  \
        return #item;
#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
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

void FeatureControlImpl::setGuestTriedEnable(Feature feature) {
    FeatureOption& opt = mGuestTriedEnabledFeatures[feature];
    opt.name = feature;
    opt.defaultVal = true;
    opt.currentVal = true;
    opt.isOverridden = false;
}

void FeatureControlImpl::parseAndApplyOverrides(std::string_view overrides) {
    for (auto it = overrides.begin(); it < overrides.end();) {
        bool enable = true;
        if (*it == '-') {
            enable = false;
            ++it;
        }
        auto itEnd = std::find(it, overrides.end(), ',');
        if (it != itEnd) {
            auto feature = fromString(std::string_view(&*it, (itEnd - it)));
            if (feature == Feature::Feature_n_items) {
                dwarning("[FeatureControl] Bad feature name: '%s'",
                         std::string(it, itEnd).c_str());
            } else {
                setEnabledOverride(feature, enable);
                VERBOSE_PRINT(
                        init,
                        "[FeatureControl] Feature '%s' (%d) state set to %s",
                        std::string(it, itEnd).c_str(), (int)feature,
                        (enable ? "enabled" : "disabled"));
            }
        }
        it = itEnd + 1;
    }
}

std::vector<Feature> FeatureControlImpl::getEnabledNonOverride() const {
    std::vector<Feature> res;

#define FEATURE_CONTROL_ITEM(feature)  \
    if (mFeatures[feature].defaultVal) \
        res.push_back(feature);

#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

    return res;
}

std::vector<Feature> FeatureControlImpl::getEnabledOverride() const {
    std::vector<Feature> res;

#define FEATURE_CONTROL_ITEM(feature)                                     \
    if (mFeatures[feature].isOverridden && mFeatures[feature].currentVal) \
        res.push_back(feature);

#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

    return res;
}

std::vector<Feature> FeatureControlImpl::getDisabledOverride() const {
    std::vector<Feature> res;

#define FEATURE_CONTROL_ITEM(feature)                                      \
    if (mFeatures[feature].isOverridden && !mFeatures[feature].currentVal) \
        res.push_back(feature);

#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

    return res;
}

std::vector<Feature> FeatureControlImpl::getEnabled() const {
    std::vector<Feature> res;

#define FEATURE_CONTROL_ITEM(feature)  \
    if (mFeatures[feature].currentVal) \
        res.push_back(feature);

#include "host-common/FeatureControlDefGuest.h"
#include "host-common/FeatureControlDefHost.h"
#undef FEATURE_CONTROL_ITEM

    return res;
}
}  // namespace featurecontrol
}  // namespace android
