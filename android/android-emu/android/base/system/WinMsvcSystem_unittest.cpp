
#include <gtest/gtest.h>
#include "android/base/system/System.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/testing/TestSystem.h"

using android::base::TestSystem;
using android::base::Win32UnicodeString;
bool callback = false;

static void msvcInvalidParameterHandler(const wchar_t* expression,
                                        const wchar_t* function,
                                        const wchar_t* file,
                                        unsigned int line,
                                        uintptr_t pReserved) {
    LOG(WARNING) << "Ignoring invalid parameter detected in function: "
                 << function << " file: " << file << ", line: " << line
                 << ", expression: " << expression;
    callback = true;
}

class WinTestSystem : TestSystem {
public:
    WinTestSystem() : TestSystem("Foo", 64) {}
    void configureHost() const override {
        _set_invalid_parameter_handler(msvcInvalidParameterHandler);
    }
};

TEST(System, bad_params_no_crash_windows) {
    WinTestSystem testSys;
    testSys.configureHost();

    // Obviously bad news:
    char* formatString = NULL;
    printf(formatString);

    EXPECT_EQ(true, callback);
}
