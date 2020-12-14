// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/ProcessControl.h"

#include <gtest/gtest.h>

#include <memory>

#include "android/base/testing/TestTempDir.h"

namespace android {
namespace base {

class ProcessControlTest : public ::testing::Test {
protected:
    void SetUp() override {
        mTempDir.reset(new TestTempDir("processcontroltest"));
    }

    void TearDown() override { mTempDir.reset(); }

    std::unique_ptr<TestTempDir> mTempDir;
};

// Tests makeArgvStrings
TEST_F(ProcessControlTest, TestMakeArgvStrings) {
    const int argc = 5;
    const char** argv = new const char*[argc];

    const char* arg0 = "arg0";
    const char* arg1 = "arg1";
    const char* arg2 = "arg2";
    const char* arg3 = "arg3";
    const char* arg4 = "arg4";

    argv[0] = arg0;
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;
    argv[4] = arg4;

    std::vector<std::string> asVector = makeArgvStrings(argc, argv);

    EXPECT_EQ(argc, asVector.size());

    for (int i = 0; i < argc; i++) {
        EXPECT_EQ(argv[i], asVector[i]);
    }
}

// Tests that launch parameters are captured correctly.
TEST_F(ProcessControlTest, LaunchParametersCaptured) {
    std::string testParamsPath = mTempDir->makeSubPath("launchParams.txt");

    ProcessLaunchParameters test = {
            "WorkingDir",
            "ProgramName",
            {"arg0", "arg1", "arg2"},
    };

    saveLaunchParameters(test, testParamsPath);

    ProcessLaunchParameters loaded = loadLaunchParameters(testParamsPath);

    EXPECT_EQ(test, loaded);
}

TEST_F(ProcessControlTest, LaunchParametersEscapedProperly) {
    const int argc = 4;
    const char** argv = new const char*[argc];

    const char* arg0 = "foo";
    const char* arg1 = " zoo ";
    const char* arg2 = "A \"quote\"";
    const char* arg3 = R"#(\t\n")#";

    argv[0] = arg0;
    argv[1] = arg1;
    argv[2] = arg2;
    argv[3] = arg3;

    std::string expect_str = R"#("foo" " zoo " "A \"quote\"" "\\t\\n\"")#";
    EXPECT_EQ(expect_str, createEscapedLaunchString(argc, argv));
}

TEST_F(ProcessControlTest, ParseEscapedLaunchStringProperly) {
    std::vector<std::string> expect{"foo", " zoo ", "A \"quote\"",
                                    R"#(\t\n")#"};
    std::string parse_str = R"#("foo" " zoo " "A \"quote\"" "\\t\\n\"")#";
    EXPECT_EQ(expect, parseEscapedLaunchString(parse_str));
}

TEST_F(ProcessControlTest, ParseEscapedQuotedLaunchStringProperly) {
    std::vector<std::string> expect{"I'm", "doing", "great"};
    std::string parse_str = R"#("I'm" doing great)#";
    EXPECT_EQ(expect, parseEscapedLaunchString(parse_str));

    expect = {"I can't believe it's snowing", "again"};
    parse_str = R"#("I can't believe it's snowing" 'again')#";
    EXPECT_EQ(expect, parseEscapedLaunchString(parse_str));

    expect = {"\"This is fine", "it's true"};
    parse_str = R"#('"This is fine' "it's true")#";
    EXPECT_EQ(expect, parseEscapedLaunchString(parse_str));

    expect = {"\"Quotes ' are ' everwhere\" in this one block \" string"};
    parse_str = R"#('"Quotes \' are \' everwhere" in this one block \" string')#";
    EXPECT_EQ(expect, parseEscapedLaunchString(parse_str));

}


}  // namespace base
}  // namespace android
