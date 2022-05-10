// Copyright (C) 2022 The Android Open Source Project
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

// The temporary command line options will inject the passed in options
// and store the previous one. Upon destruction of this object
// the previous options will be injected again.
//
// Use this in unit tests to make sure your command line options are
// always in a proper state.
#pragma once
#include "android/console.h"
#include "android/cmdline-definitions.h"

class TemporaryCommandLineOptions {
public:
    TemporaryCommandLineOptions(AndroidOptions* tmp) {
        mOld = getConsoleAgents()->settings->android_cmdLineOptions();
        getConsoleAgents()->settings->inject_cmdLineOptions(tmp);
    }
    ~TemporaryCommandLineOptions() {
        getConsoleAgents()->settings->inject_cmdLineOptions(mOld);
    }

private:
    AndroidOptions* mOld;
};
