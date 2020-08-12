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
#include "android/emulation/control/AndroidAgentFactory.h"

#include <stdio.h>

struct QAndroidAutomationAgent;

using android::emulation::AndroidConsoleFactory;

static AndroidConsoleAgents sConsoleAgents{0};
static bool isInitialized = false;


const AndroidConsoleAgents* getConsoleAgents() {
    if (!isInitialized) {
        // Let's not get involved with undefined behavior, if this happens the
        // developer is not calling injectConsoleAgents early enough.
        fprintf(stderr, "Accessing console agents before injecting them.\n");
        exit(-1);
    }
    return &sConsoleAgents;
}

#define ANDROID_DEFINE_CONSOLE_GETTER_IMPL(typ, name)                   \
    const typ* const AndroidConsoleFactory::android_get_##typ() const { \
        return sConsoleAgents.name;                                     \
    };

ANDROID_CONSOLE_AGENTS_LIST(ANDROID_DEFINE_CONSOLE_GETTER_IMPL)

#define ANDROID_CONSOLE_AGENT_SETTER(typ, name) \
    sConsoleAgents.name = factory.android_get_##typ();

void android::emulation::injectConsoleAgents(const AndroidConsoleFactory& factory) {
    if (isInitialized) {
        // This is not necessarily an error, as there are cases where this might be acceptable.
        fprintf(stderr, "Warning: console agents were already injected!\n");
    }
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_AGENT_SETTER);
    isInitialized = true;
}
