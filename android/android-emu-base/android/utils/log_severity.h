// Copyright (C) 2021 The Android Open Source Project
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
#include "android/utils/log_severity.h"

ANDROID_BEGIN_HEADER

// Defines the available log severities.

typedef enum LogSeverity {
    EMULATOR_LOG_DEBUG = -2,
    EMULATOR_LOG_VERBOSE = -1,
    EMULATOR_LOG_INFO = 0,
    EMULATOR_LOG_WARNING = 1,
    EMULATOR_LOG_ERROR = 2,
    EMULATOR_LOG_FATAL = 3,
    EMULATOR_LOG_NUM_SEVERITIES,

// DFATAL will be ERROR in release builds, and FATAL in debug ones.
#ifdef NDEBUG
    EMULATOR_LOG_DFATAL = EMULATOR_LOG_ERROR,
#else
    EMULATOR_LOG_DFATAL = EMULATOR_LOG_FATAL,
#endif
} LogSeverity;

extern LogSeverity android_log_severity;
// #endif

ANDROID_END_HEADER
