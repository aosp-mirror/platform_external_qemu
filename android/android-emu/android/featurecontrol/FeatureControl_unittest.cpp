// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/featurecontrol/testing/FeatureControlTest.h"

#include "android/base/memory/ScopedPtr.h"
#include "android/base/testing/TestSystem.h"
#include "android/cmdline-option.h"

#include <gtest/gtest.h>

namespace android {
namespace featurecontrol {

TEST_F(FeatureControlTest, overrideSetting) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    loadAllIni();
    using namespace featurecontrol;
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        setEnabledOverride(feature, true);
        EXPECT_TRUE(isEnabled(feature));
        setEnabledOverride(feature, false);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, resetToDefault) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    loadAllIni();
    using namespace featurecontrol;
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        bool defaultVal = isEnabled(feature);
        setEnabledOverride(feature, true);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, isEnabled(feature));
        setEnabledOverride(feature, false);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettings) {
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);

    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsWithNoUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsHostGuestDifferent) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOffIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");

#define FEATURE_CONTROL_ITEM(item) EXPECT_TRUE(isEnabled(item));
#include "FeatureControlDefHost.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) EXPECT_FALSE(isEnabled(item));
#include "FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOnIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);

    writeUserIniHost(mAllOnIni);
    writeUserIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
    }

    writeUserIniHost(mAllOffIni);
    writeUserIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, stringConversion) {
#define FEATURE_CONTROL_ITEM(item)           \
    EXPECT_EQ(item, stringToFeature(#item)); \
    EXPECT_STREQ(#item, FeatureControlImpl::toString(item).data());
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
#undef FEATURE_CONTROL_ITEM

    EXPECT_EQ(Feature_n_items,
              stringToFeature("somefeaturethatshouldneverexist"));
}

TEST_F(FeatureControlTest, setNonOverriden) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_TRUE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
    }

    Feature overriden = (Feature)0;
    setEnabledOverride(overriden, false);
    EXPECT_FALSE(isEnabled(overriden));

    setIfNotOverriden(overriden, true);
    EXPECT_FALSE(isEnabled(overriden));

    Feature nonOverriden = (Feature)1;
    EXPECT_TRUE(isEnabled(nonOverriden));
    EXPECT_FALSE(isOverridden(nonOverriden));
    setIfNotOverriden(nonOverriden, false);
    EXPECT_FALSE(isEnabled(nonOverriden));
}

TEST_F(FeatureControlTest, setNonOverridenGuestFeatureGuestOn) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
        setIfNotOverridenOrGuestDisabled(feature, true);
        EXPECT_TRUE(isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, setNonOverridenGuestFeatureGuestOff) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_FALSE(isEnabled(feature));
        EXPECT_FALSE(isOverridden(feature));
        setIfNotOverridenOrGuestDisabled(feature, true);
        if (isGuestFeature(feature)) {
            EXPECT_FALSE(isEnabled(feature));
        } else {
            EXPECT_TRUE(isEnabled(feature));
        }
    }
}

TEST_F(FeatureControlTest, parseCommandLine) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    ParamList feature1 = {
            (char*)FeatureControlImpl::toString(Feature::Wifi).data()};
    ParamList feature2 = {
            (char*)FeatureControlImpl::toString(Feature::EncryptUserData)
                    .data()};
    ParamList feature3 = {
            (char*)FeatureControlImpl::toString(Feature::GLPipeChecksum)
                    .data()};
    std::string feature4str =
            "-" + (std::string)FeatureControlImpl::toString(Feature::HYPERV);
    ParamList feature4 = {(char*)feature4str.data()};
    feature1.next = &feature2;
    feature2.next = &feature3;
    feature3.next = &feature4;
    AndroidOptions options = {};
    options.feature = &feature1;
    android_cmdLineOptions = &options;
    auto undoCommandLine = android::base::makeCustomScopedPtr(
            &android_cmdLineOptions,
            [](const AndroidOptions** opts) { *opts = nullptr; });
    loadAllIni();

    EXPECT_TRUE(isEnabled(Feature::Wifi));
    EXPECT_TRUE(isOverridden(Feature::Wifi));
    EXPECT_TRUE(isEnabled(Feature::EncryptUserData));
    EXPECT_TRUE(isOverridden(Feature::EncryptUserData));
    EXPECT_TRUE(isEnabled(Feature::GLPipeChecksum));
    EXPECT_TRUE(isOverridden(Feature::GLPipeChecksum));
    EXPECT_FALSE(isEnabled(Feature::HYPERV));
    EXPECT_TRUE(isOverridden(Feature::HYPERV));
}

TEST_F(FeatureControlTest, parseEnvironment) {
    android::base::TestSystem system("/usr", 64, "/");
    system.envSet(
            "ANDROID_EMULATOR_FEATURES",
            android::base::StringFormat(
                    "%s,%s,-%s", FeatureControlImpl::toString(Feature::Wifi),
                    FeatureControlImpl::toString(Feature::EncryptUserData),
                    FeatureControlImpl::toString(Feature::GLPipeChecksum)));
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    EXPECT_TRUE(isEnabled(Feature::Wifi));
    EXPECT_TRUE(isOverridden(Feature::Wifi));
    EXPECT_TRUE(isEnabled(Feature::EncryptUserData));
    EXPECT_TRUE(isOverridden(Feature::EncryptUserData));
    EXPECT_FALSE(isEnabled(Feature::GLPipeChecksum));
    EXPECT_TRUE(isOverridden(Feature::GLPipeChecksum));
}

}  // namespace featurecontrol
}  // namespace android
