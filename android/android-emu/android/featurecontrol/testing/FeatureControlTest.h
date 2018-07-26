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

class FeatureControlTest : public ::testing::Test {
public:

    void SetUp() override;

protected:

    void writeDefaultIniHost(android::base::StringView data);
    void writeDefaultIniGuest(android::base::StringView data);
    void writeUserIniHost(android::base::StringView data);
    void writeUserIniGuest(android::base::StringView data);
    void loadAllIni();
    std::unique_ptr<android::base::TestTempDir> mTempDir;
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
    void writeIni(android::base::StringView filename,
                  android::base::StringView data);
};

} // namespace featurecontrol
} // namespace android
