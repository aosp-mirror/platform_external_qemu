/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <GLcommon/GLutils.h>

bool isPowerOf2(int num) {
    return (num & (num -1)) == 0;
}

static bool s_gles2Gles = false;
static bool s_coreProfile = false;

void setGles2Gles(bool isGles2Gles) {
    s_gles2Gles = isGles2Gles;
}

bool isGles2Gles() {
    return s_gles2Gles;
}

void setCoreProfile(bool isCore) {
    s_coreProfile = isCore;
}

bool isCoreProfile() {
    return s_coreProfile;
}
