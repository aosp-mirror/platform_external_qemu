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

#ifndef ANDROID_METRICS_STUDIO_HELPER_H
#define ANDROID_METRICS_STUDIO_HELPER_H

#include "android/utils/compiler.h"
#include "android/base/Version.h"

#ifdef __APPLE__
#define ANDROID_STUDIO_DIR_PREFIX "Library/Preferences"
#define ANDROID_STUDIO_DIR "AndroidStudio"
#else  // ! ___APPLE__
#define ANDROID_STUDIO_DIR ".AndroidStudio"
#define ANDROID_STUDIO_DIR_INFIX "config"
#endif  // __APPLE__
#define ANDROID_STUDIO_DIR_SUFFIX "options"
#define ANDROID_STUDIO_DIR_PREVIEW "Preview"


#define ANDROID_STUDIO_UID_HEXPATTERN "00000000-0000-0000-0000-000000000000"

const android::base::Version extractAndroidStudioVersion(const char * const dirName);
char* latestAndroidStudioDir(const char * const scanPath) ;
char* pathToStudioXML(const char *studioPath, const char *filename ) ;

#endif  // ANDROID_METRICS_STUDIO_HELPER_H
