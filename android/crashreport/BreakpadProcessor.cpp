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

#include "android/base/system/System.h"
#include "android/crashreport/crash-handler.h"
#include "android/crashreport/BreakpadUtils.h"
#include "android/utils/debug.h"

#include <stdio.h>
#include <stdlib.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(init, __VA_ARGS__)
#define I(...) printf(__VA_ARGS__)

using namespace android::base;
namespace BreakpadProcessor {
namespace {

bool processCrash(const char * crashpath) {
    const std::string crashDirectory = BreakpadUtils::get()->getCrashDirectory();
    if (crashpath != NULL && BreakpadUtils::isDump(crashpath)) {
        if (BreakpadUtils::uploadMiniDump(crashpath)) {
          BreakpadUtils::cleanupMiniDump(crashpath);remove(crashpath);
        }
        return true;
    } else {
        return false;
    }
}

void listCrashes() {
    const std::string crashDirectory = BreakpadUtils::get()->getCrashDirectory();
    StringVector crashDumps = System::get()->scanDirEntries(crashDirectory.c_str(), true);
    for (auto i: crashDumps) {
        if (!BreakpadUtils::isDump(i.c_str())) {
            continue;
        }
        I("%s\n", i.c_str());

    }
}

}  //namespace
}  //namespace BreakpadProcessor

extern "C" bool process_crash (const char * crashpath) {
    return BreakpadProcessor::processCrash(crashpath);
}

extern "C" void list_crashes () {
    BreakpadProcessor::listCrashes();
}
