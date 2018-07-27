#include "android/featurecontrol/testing/FeatureControlTest.h"

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>

namespace android {
namespace featurecontrol {

void FeatureControlTest::SetUp() {
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
#include "android/featurecontrol/FeatureControlDefHost.h"
#include "android/featurecontrol/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = on\n"
    mAllOnIniGuestOnly =
#include "android/featurecontrol/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = off\n"
    mAllOffIni =
#include "android/featurecontrol/FeatureControlDefHost.h"
#include "android/featurecontrol/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = off\n"
    mAllOffIniGuestOnly =
#include "android/featurecontrol/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = default\n"
    mAllDefaultIni =
#include "android/featurecontrol/FeatureControlDefHost.h"
#include "android/featurecontrol/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item) #item " = default\n"
    mAllDefaultIniGuestOnly =
#include "android/featurecontrol/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM
}

void FeatureControlTest::writeDefaultIniHost(android::base::StringView data) {
    writeIni(mDefaultIniHostFilePath, data);
}

void FeatureControlTest::writeDefaultIniGuest(android::base::StringView data) {
    writeIni(mDefaultIniGuestFilePath, data);
}

void FeatureControlTest::writeUserIniHost(android::base::StringView data) {
    writeIni(mUserIniHostFilePath, data);
}

void FeatureControlTest::writeUserIniGuest(android::base::StringView data) {
    writeIni(mUserIniGuestFilePath, data);
}

void FeatureControlTest::loadAllIni() {
    FeatureControlImpl::get().init(
            mDefaultIniHostFilePath, mDefaultIniGuestFilePath,
            mUserIniHostFilePath, mUserIniGuestFilePath);
}

void FeatureControlTest::writeIni(android::base::StringView filename,
              android::base::StringView data) {
    std::ofstream outFile(filename,
                          std::ios_base::out | std::ios_base::trunc);
    ASSERT_TRUE(outFile.good());
    outFile.write(data.data(), data.size());
}

} // namespace featurecontrol
} // namespace android
