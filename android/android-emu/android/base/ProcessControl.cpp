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

#include "android/utils/path.h"

#include <fstream>
#include <sstream>

namespace android {
namespace base {

bool ProcessLaunchParameters::operator==(const ProcessLaunchParameters& other)  const {
    if (workingDirectory != other.workingDirectory) return false;
    if (programPath != other.programPath) return false;
    if (argv.size() != other.argv.size()) return false;

    for (int i = 0; i < argv.size(); ++i) {
        if (argv[i] != other.argv[i]) return false;
    }

    return true;
}

std::vector<std::string> makeArgvStrings(int argc, const char** argv) {
    std::vector<std::string> res;

    for (int i = 0; i < argc; ++i) {
        res.push_back(argv[i]);
    }

    return res;
}

void saveLaunchParameters(const ProcessLaunchParameters& launchParams, StringView filename) {
    std::ofstream file(c_str(filename));

    file << launchParams.workingDirectory << std::endl;
    file << launchParams.programPath << std::endl;
    file << launchParams.argv.size() << std::endl;

    for (const auto& arg : launchParams.argv) {
        // argv[0] is still saved, but can be skipped when launching
        file << arg << std::endl;
    }
}

void killAndWaitProcess(int pid) {
}

ProcessLaunchParameters loadLaunchParameters(StringView filename) {
    ProcessLaunchParameters res;

    std::ifstream file(filename);

    std::string line;

    std::getline(file, line);
    res.workingDirectory = line;

    std::getline(file, line);
    res.programPath = line;

    std::getline(file, line);
    std::istringstream iss(line);

    int argc = 0;
    iss >> argc;
    res.argv.resize(argc);

    for (int i = 0; i < argc; i++) {
        std::getline(file, line);
        res.argv[i] = line;
    }

    return res;
}

void launchProcessFromParametersFile(const ProcessLaunchParameters& launchParams, bool useArgv0) {
}

} // namespace base
} // namespace android

