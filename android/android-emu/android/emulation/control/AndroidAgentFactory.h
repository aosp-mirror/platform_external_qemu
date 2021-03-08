// Copyright (C) 2020 The Android Open Source Project
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

#pragma once

#include "android/console.h"  // for ANDROID_CONSOLE_AGENTS_LIST

struct QAndroidAutomationAgent;

namespace android {
namespace emulation {
#define ANDROID_DEFINE_CONSOLE_GETTER(typ, name) \
    virtual const typ* const android_get_##typ() const;

// The default android console factory will not do anything, it will
// leave the console agents intact.
//
// You an call injectConsoleAgents multiple times with this factory.
//
// If you want to override existing agents you can subclass this factory,
// override the method of interest and call injectConsoleAgents, it will replace
// the existing agents with the one your factory provides.
class AndroidConsoleFactory {
public:
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_DEFINE_CONSOLE_GETTER)
};


// Call this method to inject the console agents into the emulator. You usally
// want to call this function *BEFORE* any calls to getConsoleAgents are made.
//
// You can provide a factory that will be used to construct all the individual
// agents.
//
// Note: It is currently not safe to inject agents after the first injection has
// taken place.
void injectConsoleAgents(
        const AndroidConsoleFactory& factory);

}  // namespace emulation
}  // namespace android
