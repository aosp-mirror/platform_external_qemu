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

#ifndef ANDROID_METRICS_STUDIO_PREFERENCES_H
#define ANDROID_METRICS_STUDIO_PREFERENCES_H

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// This header files describes functions that help the android emulator
// obtain values stored in Android Studio preferences/configuration files.
// Values are read from XML files under predefined paths (location might
// vary among platforms and Android Studio versions).
// If one or more installations of AndroidStudio exist, the
// latest Studio (by revision) installation will be selected.
// If no such installation can be found, default values
// will be returned.

// This function returns 1 if the emulator user has opted-in to
// crash reporting in Android Studio. If users have not opted-in,
// or in the event of a failure, the default value 0 will be returned.
//
int android_studio_get_optins(void);

// This function returns a string that describes the Android
// Studio installation ID. If this installation ID cannot be
// retrieved, a random string following the Android Studio
// pattern of installation IDs
// (00000000-0000-0000-0000-000000000000) will be returned.
// The caller is responsible for freeing the returned string.
//
char* android_studio_get_installation_id(void);

ANDROID_END_HEADER

#endif  // ANDROID_METRICS_STUDIO_PREFERENCES_H
