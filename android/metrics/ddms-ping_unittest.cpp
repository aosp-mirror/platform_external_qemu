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

#include "android/metrics/ddms-ping.h"

#include "android/base/containers/StringVector.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

using namespace android::base;

namespace {

// Helper class used to collect a single command-line as a string vector.
class SilentShellCollector {
public:
    SilentShellCollector(TestSystem* system) : mCommand() {
        system->setSilentCommandShell(myShell, this);
    }

    const StringVector& command() const { return mCommand; }

private:
    static bool myShell(void* opaque, const StringVector& command) {
        static_cast<SilentShellCollector*>(opaque)->mCommand = command;
        return true;
    }

    StringVector mCommand;
};

}  // namespace

TEST(DdmsPing, WithoutDdmsBinary) {
    const char kEmulatorVersion[] = "Good Guy Greg 1.3";
    const char kGlVendor[] = "Nuida";
    const char kGlRenderer[] = "Fake it till you make it";
    const char kGlVersion[] = "Bugs galore 3.14";

    TestSystem testSys("/foo", 32);

    SilentShellCollector silent(&testSys);

    android_ddms_ping(kEmulatorVersion,
                      kGlVendor,
                      kGlRenderer,
                      kGlVersion);

    EXPECT_TRUE(silent.command().empty());
}

TEST(DdmsPing, WithDdmsBinary) {
    const char kEmulatorVersion[] = "Good Guy Greg 1.3";
    const char kGlVendor[] = "Nuida";
    const char kGlRenderer[] = "Fake it till you make it";
    const char kGlVersion[] = "Bugs galore 3.14";

#ifdef _WIN32
    const char kDdms[] = "/foo/ddms.bat";
#else
    const char kDdms[] = "/foo/ddms";
#endif

    TestSystem testSys("/foo", 32);
    TestTempDir* tempDir = testSys.getTempRoot();
    tempDir->makeSubDir("foo");
    tempDir->makeSubFile(kDdms);

    SilentShellCollector silent(&testSys);

    android_ddms_ping(kEmulatorVersion,
                      kGlVendor,
                      kGlRenderer,
                      kGlVersion);

    const StringVector& command = silent.command();

    EXPECT_EQ(7U, command.size());
    if (command.size() == 7U) {
        EXPECT_STREQ(kDdms, command[0].c_str());
        EXPECT_STREQ("ping", command[1].c_str());
        EXPECT_STREQ("emulator", command[2].c_str());
        EXPECT_STREQ(kEmulatorVersion, command[3].c_str());
        EXPECT_STREQ(kGlVendor, command[4].c_str());
        EXPECT_STREQ(kGlRenderer, command[5].c_str());
        EXPECT_STREQ(kGlVersion, command[6].c_str());
    }
}
