// Copyright 2016 The Android Open Source Project
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

#include "android/emulation/ConsoleAuthToken.h"

#include "android/base/EintrWrapper.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/system/System.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <stdio.h>

namespace android {

using android::base::StringView;
using android::base::ScopedFd;
using android::base::System;
using android::base::TestSystem;
using android::base::TestTempDir;

using ScopedConsoleAuthToken = std::unique_ptr<ConsoleAuthToken>;

static void makeHomeDir(const TestSystem& sys) {
    sys.getTempRoot();  // Force creation of temporary root.
    ASSERT_TRUE(sys.pathFileMkDir("/home", 0700));
}

static void writeConsoleAuthTokenFile(const TestSystem& sys,
                                      StringView token,
                                      int mode = 0600) {
    std::string filePath = android::base::PathUtils::join(
            "/home", ConsoleAuthToken::kFileName);
    ScopedFd fd(sys.pathFileCreate(filePath, mode));
    ASSERT_TRUE(fd.get() >= 0);
    if (token.size() > 0) {
        ASSERT_EQ(token.size(),
                  HANDLE_EINTR(::write(fd.get(), token.c_str(), token.size())));
    }
}

TEST(ConsoleAuthToken, DefaultOpenModeCreateOnDemand) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    EXPECT_TRUE(token.get());
    EXPECT_TRUE(token->isValid());

    std::string token64 = token->toString();
    EXPECT_NE(0, token64.size());
    EXPECT_TRUE(token->checkAgainst(token64));
}

TEST(ConsoleAuthToken, OpenModeFailIfMissing) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    ScopedConsoleAuthToken token(
            new ConsoleAuthToken(ConsoleAuthToken::OpenMode::FailIfMissing));
    EXPECT_TRUE(token.get());
    EXPECT_FALSE(token->isValid());
}

TEST(ConsoleAuthToken, NoHomeDirectory) {
    TestSystem sys("/bin/launcher", 0);
    sys.getTempRoot();  // Force temp-root creation.
    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_FALSE(token->isValid());
    EXPECT_STREQ("User home directory does not exist",
                 token->getStatus().c_str());
}

#ifndef _WIN32
TEST(ConsoleAuthToken, TokenFileWithBadPermissions) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    // Create empty file with a+rwx permissions.
    writeConsoleAuthTokenFile(sys, "", 0777);

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_FALSE(token->isValid());
    EXPECT_STREQ("Invalid console auth token file mode: 100750 expected 100600",
                 token->getStatus().c_str());
}
#endif  // !_WIN32

TEST(ConsoleAuthToken, TokenFileTooShort) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    writeConsoleAuthTokenFile(sys, "x");

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_FALSE(token->isValid());
    EXPECT_STREQ("Console authentication file is too short to be valid",
                 token->getStatus().c_str());
    EXPECT_FALSE(token->checkAgainst(""));
    EXPECT_FALSE(token->checkAgainst("Hello World"));
}

TEST(ConsoleAuthToken, TokenFileIsMalformed) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    writeConsoleAuthTokenFile(sys, "0123456789012345678801**");

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_FALSE(token->isValid());
    EXPECT_STREQ("Console auth token content is not valid base64!",
                 token->getStatus().c_str());
}

TEST(ConsoleAuthToken, EmptyTokenFileIsPassThrough) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    writeConsoleAuthTokenFile(sys, "");

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_TRUE(token->isValid());
    EXPECT_TRUE(token->checkAgainst(""));
    EXPECT_TRUE(token->checkAgainst("Hello World"));
    EXPECT_TRUE(token->checkAgainst("00000000000000000000000000000000"));
}

TEST(ConsoleAuthToken, TokenFileIsValid) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    writeConsoleAuthTokenFile(sys, "U2V0ZWMgQXN0cm9ub215Lg==");

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_TRUE(token->isValid()) << token->getStatus();
    EXPECT_FALSE(token->checkAgainst(""));
    EXPECT_FALSE(token->checkAgainst("Hello World"));
    EXPECT_FALSE(token->checkAgainst("00000000000000000000000000000000"));
    EXPECT_FALSE(token->checkAgainst("0123456789abcdef01234567"));
    EXPECT_TRUE(token->checkAgainst("U2V0ZWMgQXN0cm9ub215Lg=="));
    EXPECT_FALSE(token->checkAgainst("U2V0ZWMgQXN0cm9ub215LZ=="));
}

TEST(ConsoleAuthToken, ToStringForInvalidToken) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    writeConsoleAuthTokenFile(sys, "0123456789012345678801**");

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_FALSE(token->isValid());
    EXPECT_STREQ("<invalid-auth-token>", token->toString().c_str());
}

TEST(ConsoleAuthToken, ToStringForPassThroughToken) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    writeConsoleAuthTokenFile(sys, "");

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_TRUE(token->isValid());
    EXPECT_STREQ("", token->toString().c_str());
}

TEST(ConsoleAuthToken, ToStringForValidToken) {
    TestSystem sys("/bin/launcher", 0);
    makeHomeDir(sys);
    writeConsoleAuthTokenFile(sys, "U2V0ZWMgQXN0cm9ub215Lg==");

    ScopedConsoleAuthToken token(new ConsoleAuthToken());
    ASSERT_TRUE(token.get());
    EXPECT_TRUE(token->isValid());
    EXPECT_STREQ("U2V0ZWMgQXN0cm9ub215Lg==", token->toString().c_str());
}

}  // namespace android
