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

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// C interface to expose Qemu implementations of common VM related operations.
typedef struct QAndroidVMOperations {
    // Start/Stop VM emulating a hardware interrupt.
    bool (*stopVMInterrupt)(void);
    bool (*startVMInterrupt)(void);
    bool (*isVMRunning)(void);

    // Snapshot related VM operations.
    // |outMessage| is used to return any output from the commands.
    //   Expectation: outMessage != NULL && *outMessage == NULL
    // |errMessage| is similarly used for any errors from the commands.
    bool (*snapshotList)(char** const outMessage, char** const errMessage);
    bool (*snapshotSave)(const char* name, char** const errMessage);
    bool (*snapshotLoad)(const char* name, char** const errMessage);
    bool (*snapshotDelete)(const char* name, char** const errMessage);

} QAndroidVMOperations;

ANDROID_END_HEADER
