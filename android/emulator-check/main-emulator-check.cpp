// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/Optional.h"
#include "android/base/StringView.h"
#include "android/cpu_accelerator.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/emulator-check/PlatformInfo.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <tuple>

#include <stdio.h>
#include <stdlib.h>

using android::base::StringView;
using CommandReturn = std::pair<int, std::string>;

static const int kGenericError = 100;

static CommandReturn help();

// check ability to launch haxm/kvm accelerated VM and exit
// designed for use by Android Studio
static CommandReturn checkCpuAcceleration() {
    char* status = nullptr;
    AndroidCpuAcceleration capability =
            androidCpuAcceleration_getStatus(&status);
    std::string message = status ? status : "";
    free(status);
    return std::make_pair(capability, std::move(message));
}

static CommandReturn checkHyperV() {
    return android::GetHyperVStatus();
}

static CommandReturn getCpuInfo() {
    auto pair = android::GetCpuInfo();
    // GetCpuInfo() returns a set of lines, we need to turn it into a
    // single line.
    std::replace(pair.second.begin(), pair.second.end(), '\n', '|');
    return pair;
}

static CommandReturn getWindowManager() {
    const std::string name = android::getWindowManagerName();
    return std::make_pair(name.empty() ? kGenericError : 0, name);
}

constexpr struct Option {
    const char* arg;
    const char* help;
    CommandReturn (*handler)();
    bool printRawAndStop;
} options[] = {
        {"-h", "Show this help message", &help, true},
        {"-help", "Show this help message", &help, true},
        {"--help", "Show this help message", &help, true},
        {"accel", "Check the CPU acceleration support", &checkCpuAcceleration},
        {"hyper-v", "Check if hyper-v is installed and running (Windows)",
         &checkHyperV},
        {"cpu-info", "Return the CPU model information", &getCpuInfo},
        {"window-mgr", "Return the current window manager name",
         &getWindowManager},
};

static std::string usage() {
    std::ostringstream str;
    str <<
R"(Usage: emulator-check <argument1> [<argument2>...]

Performs the set of checks requested in <argumentX> and returns the result in
the following format:
<argument1>:
<return code for <argument1>>
<a single line of text information returned for <argument1>>
<argument1>
<argument2>:
<return code for <argument2>>
<a single line of text information returned for <argument2>>
<argument2>
...

<argumentX> is any of:

)";

    for (const auto& option : options) {
        str << "    " << option.arg << "\t\t" << option.help << '\n';
    }

    str << '\n';

    return str.str();
}

static CommandReturn help() {
    return std::make_pair(0, usage());
}

static int error(const char* format, const char* arg = nullptr) {
    if (format) {
        fprintf(stderr, format, arg);
        fprintf(stderr, "\n\n");
    }
    fprintf(stderr, "%s\n", usage().c_str());
    return kGenericError;
}

static int processArguments(int argc, const char* const* argv) {
    android::base::Optional<int> retCode;

    for (int i = 1; i < argc; ++i) {
        const StringView arg = argv[i];
        const auto opt = std::find_if(
                std::begin(options), std::end(options),
                [arg](const Option& opt) { return arg == opt.arg; });
        if (opt == std::end(options)) {
            printf("%s:\n%d\nUnknown argument\n%s\n",
                   arg.c_str(), kGenericError, arg.c_str());
            continue;
        }

        auto handlerRes = opt->handler();
        if (!retCode) {
            // Remember the first return value for the compatibility with the
            // previous version.
            retCode = handlerRes.first;
        }

        if (opt->printRawAndStop) {
            printf("%s\n", handlerRes.second.c_str());
            break;
        }

        printf("%s:\n%d\n%s\n%s\n",
               arg.c_str(),
               handlerRes.first,
               handlerRes.second.c_str(),
               arg.c_str());
    }

    return retCode.valueOr(kGenericError);
}

int main(int argc, const char* const* argv) {
    if (argc < 2) {
        return error("Missing a required argument(s)");
    }

    return processArguments(argc, argv);
}
