// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/cmdline-option.h"

#include <gtest/gtest.h>

TEST(CmdLineOptions, parsePort) {
    struct {
        const char* input;
        bool expectSuccess;
        int expectOutput;
    } kData[] = {
        {"5554", true, 5554},
        {"5555", true, 5554}, // Odd ports not allowed, adjusted down
        {"5552", false, 0},   // Port below lower bound of 5554
        {"5553", false, 0},   // Port below lower bound of 5554
        {"5681", false, 0},   // Odd port adjusted down still not valid
        {"5682", false, 0},   // Even port above range
        {"80", false, 0},     // Out of range
        {"foo", false, 0},
        {"5554trailingText", false, 0},
        {"", false, 0},
        {nullptr, false, 0},
    };

    for (const auto& data : kData) {
        int output = 0;
        bool result = android_parse_port_option(data.input, &output);
        EXPECT_EQ(data.expectSuccess, result);
        EXPECT_EQ(data.expectOutput, output);
    }
}

TEST(CmdLineOptions, parsePorts) {
    struct {
        const char* input;
        bool expectSuccess;
        int expectConsolePort;
        int expectAdbPort;
    } kData[] = {
        {"5554,5555", true, 5554, 5555},
        {"1,200", true, 1, 200},           // No restrictions on ports
        {"65000,7", true, 65000, 7},
        {"-2,5555", false, 0, 0},          // No negative values
        {"5554,-42", false, 0, 0},         // No negative values
        {" 5554, 5555", true, 5554, 5555}, // Leading spaces are fine
        {"5554 ,5555", false, 0, 0},       // But not trailing spaces
        {"5554,5555 ", false, 0, 0},       // But not trailing spaces
        {"5554,5555trailingText", false, 0, 0},
        {"5554trailingtext,5555", false, 0, 0},
        {"345,", false, 0, 0},
        {",678", false, 0, 0},
        {"123", false, 0, 0},
        {"123 456", false, 0, 0},
        {"0,234", false, 0, 0},            // No ports below 1
        {"234,0", false, 0, 0},
        {"65536,234", false, 0, 0},        // No ports above 65535
        {"234,65536", false, 0, 0},
        {nullptr, false, 0, 0},
    };

    for (const auto& data : kData) {
        int consolePort = 0, adbPort = 0;
        bool result = android_parse_ports_option(data.input,
                                                 &consolePort,
                                                 &adbPort);
        EXPECT_EQ(data.expectSuccess, result);
        EXPECT_EQ(data.expectConsolePort, consolePort);
        EXPECT_EQ(data.expectAdbPort, adbPort);
    }
}

