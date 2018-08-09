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

// Library to:
// save argv,
// kill and wait a process ID,
// launch process with given cwd and args.

#include "android/base/FunctionView.h"
#include "android/base/StringView.h"

#include <string>
#include <vector>

namespace android {
namespace base {

struct ProcessLaunchParameters {
    bool operator==(const ProcessLaunchParameters& other)  const;
    std::string str() const;

    std::string workingDirectory;
    std::string programPath;
    std::vector<std::string> argv;
    // TODO: save env variables, which is good for the case
    // where we are launching not from a child process
};

std::vector<std::string> makeArgvStrings(int argc, const char** argv);

ProcessLaunchParameters createLaunchParametersForCurrentProcess(int argc, char** argv);

void saveLaunchParameters(const ProcessLaunchParameters& launchParams, StringView filename);

void finalizeEmulatorRestartParameters(const char* dir);

ProcessLaunchParameters loadLaunchParameters(StringView filename);
void launchProcessFromParameters(const ProcessLaunchParameters& launchParams, bool useArgv0 = false);

// Restart mechanism
void disableRestart(); // Disables the restart mechanism.
bool isRestartDisabled();
void initializeEmulatorRestartParameters(int argc, char** argv, const char* paramFolder);
void setEmulatorRestartOnExit();

void registerEmulatorQuitCallback(FunctionView<void()> func);
void restartEmulator();

} // namespace base
} // namespace android
