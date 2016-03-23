// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/system/Process.h"

#include "android/base/files/PathUtils.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/base/Uuid.h"

#include <gtest/gtest.h>

#include <fstream>

namespace android {
namespace base {

TEST(Process, runCommandTrue) {
#ifndef _WIN32
    std::vector<std::string> cmd = {"ls"};
#else
    // 'dir' is a builtin command, so all you need is a sane cmd.exe
    std::vector<std::string> cmd = {"cmd.exe", "/C", "dir"};
#endif

    EXPECT_TRUE(Process::runCommand(cmd));
    EXPECT_TRUE(Process::runCommand(cmd, Process::Options::Default,
                                    Process::RunOptions::TerminateOnTimeout, "",
                                    1000));

    Process::Pid pid = 0;
    Process::ExitCode exitCode = 666;
    EXPECT_TRUE(Process::runCommand(cmd, Process::Options::Default,
                                    Process::RunOptions::Default, "",
                                    Process::kInfinite, &exitCode, &pid));
    EXPECT_EQ(0, exitCode);
    EXPECT_GT(pid, 0);
}

TEST(Process, runCommandTimeout) {
#ifndef _WIN32
    std::vector<std::string> cmd = {"sleep", "0.5"};
#else
    // 2 Attempts give us a delay of 1 second.
    // 'ping' is not listed as an internal cmd.exe command, but seems to be that
    // way on recent windows boxes and wine. Safe to assume?
    std::vector<std::string> cmd = {"cmd.exe", "/C", "ping",
                                    "-n",      "2",  "127.0.0.1"};
#endif

    EXPECT_FALSE(Process::runCommand(cmd, Process::Options::Default,
                                     Process::RunOptions::TerminateOnTimeout,
                                     "", 20));

    Process::Pid pid = 0;
    Process::ExitCode exitCode = 666;
    EXPECT_TRUE(Process::runCommand(cmd, Process::Options::Default,
                                    Process::RunOptions::Default, "",
                                    Process::kInfinite, &exitCode, &pid));
    EXPECT_EQ(0, exitCode);
    EXPECT_GT(pid, 0);
}

TEST(Process, runCommandWithOutput) {
#ifndef _WIN32
    std::vector<std::string> cmd = {"ls"};
#else
    // 'dir' is a builtin command, so all you need is a sane cmd.exe
    std::vector<std::string> cmd = {"cmd.exe", "/C", "dir"};
#endif
    Process::Pid pid = 666;
    Process::ExitCode exitCode = 0;
    auto sys = System::get();
    std::string outputFile =
            PathUtils::recompose({sys->getTempDir(),
                StringFormat("test-%s.txt", Uuid::generate().toString())});

    EXPECT_FALSE(sys->pathExists(outputFile));
    EXPECT_TRUE(Process::runCommand(cmd, Process::Options::DumpOutputToFile,
                                    Process::RunOptions::Default, outputFile,
                                    Process::kInfinite, &exitCode, &pid));

    EXPECT_TRUE(sys->pathExists(outputFile));
    EXPECT_TRUE(sys->pathIsFile(outputFile));
    EXPECT_TRUE(sys->pathCanRead(outputFile));
    System::FileSize fileSize;
    EXPECT_TRUE(sys->pathFileSize(outputFile, &fileSize));
    EXPECT_LT(0, fileSize);
}

}  // namespace base
}  // namespace android
