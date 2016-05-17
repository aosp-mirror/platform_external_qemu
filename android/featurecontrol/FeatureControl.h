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

namespace android{

    // FeatureControl is used to switch on/off advanced features
    // It loads sdk/tools/advancedFeatures.ini for default values and
    // .android/advancedFeatures.ini for user overriden values.
    // It is expected to be initialized at the beginning of the emulator.
    // For easier testing, one may also want to pass the override value through
    // command line and call setEnabledOverride.
    //
    // FeatureControl::isEnabled is thread safe, all other methods are not.
    //
    // To add new features, please add a new line to FeatureControlDef.h, in
    // the following format:
    // FEATURE_CONTROL_ITEM(YOUR_FEATURE_NAME)
    
class FeatureControl {
public:
    enum Feature {
#define FEATURE_CONTROL_ITEM(item) item,
#   include "FeatureControlDef.h"
#undef FEATURE_CONTROL_ITEM
        Feature_n_items
    }; 

    static bool isEnabled(Feature feature);
    static void setEnabledOverride(Feature feature, bool isEnabled);
    static void setEnabledToDefault(Feature feature);
    static void init();

    struct FeatureOption {
        Feature name;
        bool defaultVal;
        bool currentVal;
        bool isOverridden;
    };
private:
    FeatureOption mFeatures[Feature_n_items];
    FeatureControl();
};

}

