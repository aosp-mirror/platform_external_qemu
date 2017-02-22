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

#include "android/featurecontrol/Features.h"
#include "android/featurecontrol/HWMatching.h"

namespace android {
namespace featurecontrol {

// featurecontrol is used to switch on/off advanced features
// It loads sdk/tools/advancedFeatures.ini for default values and
// .android/advancedFeatures.ini for user overriden values.
// It is expected to be initialized at the beginning of the emulator.
// For easier testing, one may also want to pass the override value through
// command line and call setEnabledOverride. (Command line override not
// implemented yet)
//
// featurecontrol::isEnabled is thread safe, all other methods are not.
//
// To add new features, please (1) add it to android/data/advancedFeatures.ini
// (2) add a new line to FeatureControlDef.h, in the following format:
// FEATURE_CONTROL_ITEM(YOUR_FEATURE_NAME)
bool isEnabled(Feature feature);
void setEnabledOverride(Feature feature, bool isEnabled);
void resetEnabledToDefault(Feature feature);

// For server-based blacklists and feature control:
// The single function to call: queries blacklist of HW specs
// deemed special enough to disable (or enable!) certain
// advancedFeatures, and then actually enables/disables features
// if the user has not override them in ~/.android/advancedFeatures.ini.
// Downloads a server-specific protobuf file containing the patterns,
// asynchronously; if there is no such protobuf file or the file
// is not done being written yet, then there are no extra actions taken.
void downloadAndApplyFeaturePatterns();

} // namespace android
} // namespace featurecontrol
