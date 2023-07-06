#include "android/featurecontrol/testing/FeatureControlTest.h"

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>
#include <string_view>

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

#define FEATURE_CONTROL_ITEM(item, idx) \
    #item " = on\n"
    mAllOnIni =
#include "host-common/FeatureControlDefHost.h"
#include "host-common/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item, idx) #item " = on\n"
    mAllOnIniGuestOnly =
#include "host-common/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item, idx) #item " = off\n"
    mAllOffIni =
#include "host-common/FeatureControlDefHost.h"
#include "host-common/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item, idx) #item " = off\n"
    mAllOffIniGuestOnly =
#include "host-common/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item, idx) #item " = default\n"
    mAllDefaultIni =
#include "host-common/FeatureControlDefHost.h"
#include "host-common/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM

#define FEATURE_CONTROL_ITEM(item, idx) #item " = default\n"
    mAllDefaultIniGuestOnly =
#include "host-common/FeatureControlDefGuest.h"
    ;
#undef FEATURE_CONTROL_ITEM
}

void FeatureControlTest::writeDefaultIniHost(std::string_view data) {
    writeIni(mDefaultIniHostFilePath, data);
}

void FeatureControlTest::writeDefaultIniGuest(std::string_view data) {
    writeIni(mDefaultIniGuestFilePath, data);
}

void FeatureControlTest::writeUserIniHost(std::string_view data) {
    writeIni(mUserIniHostFilePath, data);
}

void FeatureControlTest::writeUserIniGuest(std::string_view data) {
    writeIni(mUserIniGuestFilePath, data);
}

void FeatureControlTest::loadAllIni() {
    FeatureControlImpl::get().init(
            mDefaultIniHostFilePath, mDefaultIniGuestFilePath,
            mUserIniHostFilePath, mUserIniGuestFilePath);
}

void FeatureControlTest::writeIni(std::string_view filename,
                                  std::string_view data) {
    std::ofstream outFile(std::string(filename),
                          std::ios_base::out | std::ios_base::trunc);
    ASSERT_TRUE(outFile.good());
    outFile.write(data.data(), data.size());
}

} // namespace featurecontrol
} // namespace android
