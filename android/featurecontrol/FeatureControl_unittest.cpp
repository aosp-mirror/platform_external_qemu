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
        mDefaultIniFilePath = mTempDir->makeSubPath("defaultSettings.ini").c_str();
        mUserIniFilePath = mTempDir->makeSubPath("userSettings.ini").c_str();

#define FEATURE_CONTROL_ITEM(item) \
        #item " = on\n"
        mAllOnIni =
#include "FeatureControlDef.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) \
        #item " = off\n"
        mAllOffIni =
#include "FeatureControlDef.h"
        ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) \
        #item " = default\n"
        mAllDefaultIni =
#include "FeatureControlDef.h"
        ;
#undef FEATURE_CONTROL_ITEM
    }
protected:
    void writeDefaultIni(android::base::StringView data) {
        std::ofstream outFile(mDefaultIniFilePath,
                              std::ios_base::out | std::ios_base::trunc);
        ASSERT_TRUE(outFile.good());
        outFile.write(data.data(), data.size());
    }
    void writeUserIni(android::base::StringView data) {
        std::ofstream outFile(mUserIniFilePath,
                              std::ios_base::out | std::ios_base::trunc);
        ASSERT_TRUE(outFile.good());
        outFile.write(data.data(), data.size());
    }
    std::unique_ptr<base::TestTempDir> mTempDir;
    std::string mDefaultIniFilePath;
    std::string mUserIniFilePath;
    std::string mAllOnIni;
    std::string mAllOffIni;
    std::string mAllDefaultIni;
};

}  // namespace

TEST_F(FeatureControlTest, overrideSetting) {
    using namespace featurecontrol;
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        setEnabledOverride(feature, true);
        EXPECT_EQ(true, isEnabled(feature));
        setEnabledOverride(feature, false);
        EXPECT_EQ(false, isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, resetToDefault) {
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
    writeUserIni(mAllDefaultIni);

    writeDefaultIni(mAllOnIni);
    FeatureControlImpl::get().init(mDefaultIniFilePath, mUserIniFilePath);
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_EQ(true, isEnabled(feature));
    }

    writeDefaultIni(mAllOffIni);
    FeatureControlImpl::get().init(mDefaultIniFilePath, mUserIniFilePath);
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_EQ(false, isEnabled(feature));
    }
}

TEST_F(FeatureControlTest, readUserSettings) {
    writeDefaultIni(mAllOffIni);

    writeUserIni(mAllOnIni);
    FeatureControlImpl::get().init(mDefaultIniFilePath, mUserIniFilePath);
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_EQ(true, isEnabled(feature));
    }

    writeUserIni(mAllOffIni);
    FeatureControlImpl::get().init(mDefaultIniFilePath, mUserIniFilePath);
    for (int i = 0; i < Feature_n_items; i++) {
        Feature feature = static_cast<Feature>(i);
        EXPECT_EQ(false, isEnabled(feature));
    }
}

} // featurecontrol
} // android