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
typedef struct QAndroidVmOperations {
    bool (*vmStop)(void);
    bool (*vmStart)(void);
    bool (*vmIsRunning)(void);

    // Snapshot-related VM operations.
    // On exit, sets |*outMessage| and |*errMessage| to heap-allocated strings
    // corresponding to the command's output and error message that must be
    // freed by the caller.
    // Returns true on success, false on failure.
    bool (*snapshotList)(char** const outMessage, char** const errMessage);
    bool (*snapshotSave)(const char* name, char** const errMessage);
    bool (*snapshotLoad)(const char* name, char** const errMessage);
    bool (*snapshotDelete)(const char* name, char** const errMessage);
} QAndroidVmOperations;

ANDROID_END_HEADER
