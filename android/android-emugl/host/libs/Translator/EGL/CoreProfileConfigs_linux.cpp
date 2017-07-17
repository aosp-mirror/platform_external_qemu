/*
* Copyright (C) 2017 The Android Open Source Project
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

#include "CoreProfileConfigs.h"

#include <GL/glx.h>

// Based on https://git.reviewboard.kde.org/r/130118/diff/1#index_header
static const int kCoreProfileConfig_4_5[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 5,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int kCoreProfileConfig_4_4[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 4,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int kCoreProfileConfig_4_3[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int kCoreProfileConfig_4_2[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 2,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int kCoreProfileConfig_4_1[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 1,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int kCoreProfileConfig_4_0[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    GLX_CONTEXT_MINOR_VERSION_ARB, 0,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int kCoreProfileConfig_3_3[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int kCoreProfileConfig_3_2[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 2,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};

static const int* kCoreProfileConfigs[] = {
    kCoreProfileConfig_4_5,
    kCoreProfileConfig_4_4,
    kCoreProfileConfig_4_3,
    kCoreProfileConfig_4_2,
    kCoreProfileConfig_4_1,
    kCoreProfileConfig_4_0,
    kCoreProfileConfig_3_3,
    kCoreProfileConfig_3_2,
};

// Versions 3.1 and below ignored since they won't
// help us support ES 3.x

int getNumCoreProfileCtxAttribs() {
    return sizeof(kCoreProfileConfigs) / sizeof(kCoreProfileConfigs[0]);
}

const int* getCoreProfileCtxAttribs(int index) {
    return kCoreProfileConfigs[index];
}

void getCoreProfileCtxAttribsVersion(const int* attribs, int* maj, int* min) {
    int i = 0;
    if (!attribs) return;

    while (attribs[i] != None) {
        switch (attribs[i]) {
            case GLX_CONTEXT_MAJOR_VERSION_ARB:
                if (maj) *maj = attribs[i + 1];
                break;
            case GLX_CONTEXT_MINOR_VERSION_ARB:
                if (min) *min = attribs[i + 1];
                break;
            default:
                break;
        }
        i += 2;
    }
}
