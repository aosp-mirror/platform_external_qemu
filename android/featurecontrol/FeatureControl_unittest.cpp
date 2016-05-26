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

#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/FeatureControlImpl.h"

#include "android/base/StringView.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

namespace android {
namespace featurecontrol {

namespace {

class FeatureControlTest : public ::testing::Test {
public:
    void SetUp() override {
        mTempDir.reset(new base::TestTempDir("featurecontroltest"));
        mDefaultIniHostFilePath =
                mTempDir->makeSubPath("defaultSettingsHost.ini").c_str();
        mDefaultIniGuestFilePath =
                mTempDir->makeSubPath("defaultSettingsGuest.ini").c_str();
        mUserIniHostFilePath =
                mTempDir->makeSubPath("userSettingsHost.ini").c_str();
        mUserIniGuestFilePath =
                mTempDir->makeSubPath("userSettingsGuest.ini").c_str();

#define FEATURE_CONTROL_ITEM(item) \
        #item " = on\n"
        mAllOnIni =
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = on\n"
        mAllOnIniGuestOnly =
#include "FeatureControlDefGuest.h"
                ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = off\n"
        mAllOffIni =
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = off\n"
        mAllOffIniGuestOnly =
#include "FeatureControlDefGuest.h"
                ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = default\n"
        mAllDefaultIni =
#include "FeatureControlDefHost.h"
#include "FeatureControlDefGuest.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = default\n"
        mAllDefaultIniGuestOnly =
#include "FeatureControlDefGuest.h"
                ;
#undef FEATURE_CONTROL_ITEM
    }
protected:
    void writeDefaultIniHost(const std::string& data) {
        writeIni(mDefaultIniHostFilePath, data);
    }
    void writeDefaultIniGuest(const std::string& data) {
        writeIni(mDefaultIniGuestFilePath, data);
    }
    void writeUserIniHost(const std::string& data) {
        writeIni(mUserIniHostFilePath, data);
    }
    void writeUserIniGuest(const std::string& data) {
        writeIni(mUserIniGuestFilePath, data);
    }
    void loadAllIni() {
        FeatureControlImpl::get();

        FeatureControlImpl::get().init(
                mDefaultIniHostFilePath, mDefaultIniGuestFilePath,
                mUserIniHostFilePath, mUserIniGuestFilePath);
    }
    std::unique_ptr<base::TestTempDir> mTempDir;
    std::string mDefaultIniHostFilePath;
    std::string mDefaultIniGuestFilePath;
    std::string mUserIniHostFilePath;
    std::string mUserIniGuestFilePath;
    std::string mAllOnIni;
    std::string mAllOnIniGuestOnly;
    std::string mAllOffIni;
    std::string mAllOffIniGuestOnly;
    std::string mAllDefaultIni;
    std::string mAllDefaultIniGuestOnly;

private:
    void writeIni(const std::string& filename,
                  const std::string& data) {
        std::ofstream outFile(filename,
                              std::ios_base::out | std::ios_base::trunc);
        ASSERT_TRUE(outFile.good());
        outFile.write(data.data(), data.size());
    }
};

}  // namespace

TEST_F(FeatureControlTest, overrideSetting) {
    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    loadAllIni();
    using namespace featurecontrol;
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        setEnabledOverride(feature, true);
        EXPECT_TRUE(featureIsEnabled(feature));
        setEnabledOverride(feature, false);
        EXPECT_FALSE(featureIsEnabled(feature));
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
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        bool defaultVal = featureIsEnabled(feature);
        setEnabledOverride(feature, true);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, featureIsEnabled(feature));
        setEnabledOverride(feature, false);
        resetEnabledToDefault(feature);
        EXPECT_EQ(defaultVal, featureIsEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettings) {
    writeUserIniHost(mAllDefaultIni);
    writeUserIniGuest(mAllDefaultIniGuestOnly);
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        EXPECT_TRUE(featureIsEnabled(feature));
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        EXPECT_FALSE(featureIsEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsWithNoUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        EXPECT_TRUE(featureIsEnabled(feature));
    }

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOffIniGuestOnly);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        EXPECT_FALSE(featureIsEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readDefaultSettingsHostGuestDifferent) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOffIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");

#define FEATURE_CONTROL_ITEM(item) EXPECT_TRUE(featureIsEnabled(item));
#include "FeatureControlDefHost.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) EXPECT_FALSE(featureIsEnabled(item));
#include "FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

    writeDefaultIniHost(mAllOffIni);
    writeDefaultIniGuest(mAllOnIni);
    FeatureControlImpl::get().init(mDefaultIniHostFilePath,
                                   mDefaultIniGuestFilePath, "", "");
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        EXPECT_FALSE(featureIsEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readUserSettings) {
    writeDefaultIniHost(mAllOnIni);
    writeDefaultIniGuest(mAllOnIniGuestOnly);

    writeUserIniHost(mAllOnIni);
    writeUserIniGuest(mAllOnIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        EXPECT_TRUE(featureIsEnabled(feature));
    }

    writeUserIniHost(mAllOffIni);
    writeUserIniGuest(mAllOffIniGuestOnly);
    loadAllIni();
    for (int i = 0; i < Feature_n_items; i++) {
        AndroidFeatureControlFeature feature =
            static_cast<AndroidFeatureControlFeature>(i);
        EXPECT_FALSE(featureIsEnabled(feature));
    }
}

} // featurecontrol
} // android
