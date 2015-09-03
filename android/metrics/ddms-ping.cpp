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
#include "android/base/String.h"
#include "android/base/StringFormat.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

#include <errno.h>
#ifndef _WIN32
#include <fcntl.h>
#include <signal.h>
#endif
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

using android::base::System;
using android::base::String;
using android::base::StringFormat;
using android::base::StringVector;

#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)

#ifdef _WIN32
#define _ANDROID_DDMS_PROGRAM "ddms.bat"
#else
#define _ANDROID_DDMS_PROGRAM "ddms"
#endif

namespace {

#ifndef _WIN32
// Small utility class used to block all signals on creation, and restore
// the previous signal set on exit.
class ScopedSignalBlocker {
public:
    ScopedSignalBlocker() {
        // Block all signals and save current set into |mOldSet|.
        sigset_t newSet;
        sigfillset(&newSet);
        pthread_sigmask(SIG_SETMASK, &newSet, &mOldSet);
    }

    ~ScopedSignalBlocker() {
        // Restore the old signal set from |mOldSet|
        pthread_sigmask(SIG_SETMASK, &mOldSet, NULL);
    }

private:
    sigset_t mOldSet;
};
#endif  // !_WIN32

}  // namespace

void android_ddms_ping(const char* emulatorVersion,
                       const char* glVendor,
                       const char* glRenderer,
                       const char* glVersion) {
    System* system = System::get();
    String ddmsPath =
            StringFormat("%s/%s", system->getProgramDirectory().c_str(),
                         _ANDROID_DDMS_PROGRAM);
    if (!system->pathExists(ddmsPath.c_str())) {
        D("Missing ddms program: %s", ddmsPath.c_str());
        return;
    }

    D("Found ddms program: %s", ddmsPath.c_str());

    StringVector commandLine;

    commandLine.push_back(ddmsPath);
    commandLine.push_back(String("ping"));
    commandLine.push_back(String("emulator"));
    commandLine.push_back(String(emulatorVersion));
    commandLine.push_back(String(glVendor));
    commandLine.push_back(String(glRenderer));
    commandLine.push_back(String(glVersion));

#ifndef _WIN32
    ScopedSignalBlocker signalBlocker;
#endif
    system->runSilentCommand(commandLine);
}
