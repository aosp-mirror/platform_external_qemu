// Copyright (C) 2019 The Android Open Source Project
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

#include "android/emulation/control/logcat/LogcatParser.h"

#include <gtest/gtest.h>  // for Test, Message, TestP...
#include <iostream>

namespace android {
namespace emulation {
namespace control {

TEST(LogcatParser, parsesOneEntry) {
    // Note the \n at end!
    std::string tst =
            "10-11 22:27:43.043  2233  2414 W ErrorReporter: reportError "
            "[type: 211, code: 524300]: Error reading from input stream\n";
    auto res = LogcatParser::parseLines(tst);
    auto entries = res.second;
    EXPECT_EQ(entries.size(), 1);
}


TEST(LogcatParser, parsesTime) {
    // Note the \n at end!
    std::string tst =
            "10-11 22:27:43.043  2233  2414 W ErrorReporter: reportError "
            "[type: 211, code: 524300]: Error reading from input stream\n";
    auto res = LogcatParser::parseLines(tst);
    auto entry  = res.second[0];
    time_t time_in_s = entry.timestamp() / 1000;
    std::tm* tm = std::localtime(&time_in_s);
    // Year after 1900..
    EXPECT_TRUE(tm->tm_year >= 119);
    EXPECT_EQ(tm->tm_mon, 10);
    EXPECT_EQ(tm->tm_mday, 11);
    EXPECT_EQ(tm->tm_hour, 22);
    EXPECT_EQ(tm->tm_min, 27);
    EXPECT_EQ(tm->tm_sec, 43);
    EXPECT_EQ(entry.timestamp() % 1000, 43);
}

TEST(LogcatParser, consumedALine) {
    // Note the \n at end!
    std::string tst =
            "10-11 22:27:43.043  2233  2414 W ErrorReporter: reportError "
            "[type: 211, code: 524300]: Error reading from input stream\n";
    auto res = LogcatParser::parseLines(tst);
    EXPECT_EQ(res.first, tst.size());
}

TEST(LogcatParser, ingnoresBadLine) {
    // Note the \n at end!
    std::string tst =
            "xsxsxs\n10-11 22:27:43.043  2233  2414 W ErrorReporter: "
            "reportError [type: 211, code: 524300]: Error reading from input "
            "stream\n";
    auto res = LogcatParser::parseLines(tst);
    EXPECT_EQ(res.second.size(), 1);
    EXPECT_EQ(res.first, tst.size());
}

TEST(LogcatParser, skipUntil_newline) {
    // Note the \n at end!
    std::string tst =
            "10-11 22:27:43.043  2233  2414 W ErrorReporter: reportError "
            "[type: 211, code: 524300]: Error reading from input stream\n";
    std::string snd = "10-11 22:27:44.001  2233  2414 I Test: secondLine\n";
    for (int i = 0; i < snd.size() - 1; i++) {
        auto res = LogcatParser::parseLines(tst + snd.substr(0, i));
        EXPECT_EQ(res.second.size(), 1);
        EXPECT_EQ(res.first, tst.find('\n') + 1);
    }
}


TEST(LogcatParser, entryLooksAsExpected) {
    // Note the \n at end!
    std::string tst =
            "10-11 22:27:43.043  2233  2414 W ErrorReporter: reportError "
            "[type: 211, code: 524300]: Error reading from input stream\n";
    auto res = LogcatParser::parseLines(tst);
    auto entry = res.second[0];
    EXPECT_EQ(entry.pid(), 2233);
    EXPECT_EQ(entry.tid(), 2414);
    EXPECT_EQ(entry.level(), LogcatEntry_LogLevel_WARN);
    EXPECT_STREQ(entry.tag().c_str(), "ErrorReporter");
    EXPECT_STREQ(entry.msg().c_str(),
                 "reportError [type: 211, code: 524300]: Error reading from "
                 "input stream");
    EXPECT_EQ(entry.timestamp() % 1000, 43);
}

}  // namespace control
}  // namespace emulation
}  // namespace android
