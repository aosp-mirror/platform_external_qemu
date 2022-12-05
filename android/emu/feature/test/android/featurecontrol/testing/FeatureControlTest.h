#include "host-common/FeatureControl.h"
#include "android/featurecontrol/FeatureControlImpl.h"

#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <string>
#include <string_view>

namespace android {
namespace featurecontrol {

class FeatureControlTest : public ::testing::Test {
public:

    void SetUp() override;

protected:
    void writeDefaultIniHost(std::string_view data);
    void writeDefaultIniGuest(std::string_view data);
    void writeUserIniHost(std::string_view data);
    void writeUserIniGuest(std::string_view data);
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
    void writeIni(std::string_view filename, std::string_view data);
};

} // namespace featurecontrol
} // namespace android
