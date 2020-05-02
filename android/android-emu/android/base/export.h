// Copyright 2019 The Android Open Source Project
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

// Convenient declspec dllexport macro for android-emu-shared on Windows
#ifndef AEMU_EXPORT
    #ifdef _MSC_VER
        #ifdef AEMU_MIN
            #define AEMU_EXPORT __declspec(dllexport)
        #else // AEMU_MIN
            #define AEMU_EXPORT
        #endif // !AEMU_MIN
    #else // _MSC_VER
        // Covered by fvisibility=default
        #define AEMU_EXPORT
    #endif // !_MSC_VER
#endif // !AEMU_EXPORT
