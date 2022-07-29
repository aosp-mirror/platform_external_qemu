// Copyright 2022 The Android Open Source Project
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
#ifndef ANDROID_WEBRTC_EXPORT
    #ifdef _MSC_VER
        #ifdef ANDROID_WEBRTC
            #define ANDROID_WEBRTC_EXPORT __declspec(dllexport)
        #else // ANDROID_WEBRTC
            #define ANDROID_WEBRTC_EXPORT
        #endif // !ANDROID_WEBRTC
    #else // _MSC_VER
        // Covered by fvisibility=default
        #define ANDROID_WEBRTC_EXPORT
    #endif // !_MSC_VER
#endif // !ANDROID_WEBRTC_EXPORT
