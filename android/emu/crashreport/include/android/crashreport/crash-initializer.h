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
#pragma once
#include "android/utils/compiler.h"
#include "host-common/crash-handler.h"

ANDROID_BEGIN_HEADER

// Enable crash reporting by starting crash service process, initializing and
// attaching crash handlers.  Should only be run once at the start of the
// program.
//
// Once you have a UI active to provide consent for uploading of generated
// crashes you should make a call to upload_crashes.
//
// It will:
//  - Activate the crash handler
//  - Set an attribute containing the launch parameters.
bool crashhandler_init(int, char**);

// This will seek approval from the user for any crash reports
// that are stored on the local system that have not yet been
// reported to google.
//
// This should be invoked as soon as the UI has been activated
// as this could require user input.
void upload_crashes(void);

ANDROID_END_HEADER

