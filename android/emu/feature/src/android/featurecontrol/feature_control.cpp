// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "host-common/feature_control.h"
#include "host-common/FeatureControl.h"

void feature_initialize() {
    android::featurecontrol::initialize();
}

bool feature_is_enabled(Feature feature) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    return android::featurecontrol::isEnabled(value);
}

bool feature_is_enabled_by_guest(Feature feature) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    return android::featurecontrol::isEnabledByGuest(value);
}

void feature_set_enabled_override(Feature feature, bool isEnabled) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    android::featurecontrol::setEnabledOverride(value, isEnabled);
}

void feature_set_if_not_overridden(Feature feature, bool enable) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    android::featurecontrol::setIfNotOverriden(value, enable);
}

void feature_set_if_not_overridden_or_guest_disabled(Feature feature,
                                                     bool enable) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    android::featurecontrol::setIfNotOverridenOrGuestDisabled(value, enable);
}

void feature_reset_enabled_to_default(Feature feature) {
    auto value = static_cast<android::featurecontrol::Feature>(feature);
    return android::featurecontrol::resetEnabledToDefault(value);
}

void feature_update_from_server() {
    android::featurecontrol::applyCachedServerFeaturePatterns();
    android::featurecontrol::asyncUpdateServerFeaturePatterns();
}

const char* feature_name(Feature feature) {
    static const std::vector<std::string>* const sFeatureNames = [] {
        return new std::vector<std::string>{
#define FEATURE_CONTROL_ITEM(item) #item,
#include "host-common/FeatureControlDefHost.h"
#include "host-common/FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM
        };
    }();

    if (feature >= sFeatureNames->size()) {
        return "InvalidFeature";
    }

    return (*sFeatureNames)[feature].c_str();
}
