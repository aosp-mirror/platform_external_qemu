/*
* Copyright 2011 The Android Open Source Project
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

#include "MacPixelFormatsAttribs.h"

static const NSOpenGLPixelFormatAttribute attrs32_1[] =
{
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFAStencilSize ,8,
    0
};

static const NSOpenGLPixelFormatAttribute attrs32_2[] =
{
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFAAlphaSize   ,8,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFAStencilSize ,8,
    0
};

static const NSOpenGLPixelFormatAttribute attrs32_3[] =
{
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFAAlphaSize   ,8,
    0
};

static const NSOpenGLPixelFormatAttribute attrs32_4[] =
{
    NSOpenGLPFAColorSize   ,32,
    0
};

static const NSOpenGLPixelFormatAttribute attrs32_5[] =
{
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFASamples     ,2,
    0
};

static const NSOpenGLPixelFormatAttribute attrs32_6[] =
{
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFASamples     ,4,
    0
};

static const NSOpenGLPixelFormatAttribute attrs32_7[] =
{
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFAAlphaSize   ,8,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFAStencilSize ,8,
    NSOpenGLPFASamples     ,4,
    0
};

static const NSOpenGLPixelFormatAttribute attrs16_1[] =
{
    NSOpenGLPFAColorSize   ,16,
    NSOpenGLPFADepthSize   ,24,
    0
};

static NSOpenGLPixelFormatAttribute attrs16_2[] =
{
    NSOpenGLPFAColorSize   ,16,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFAStencilSize ,8,
    0
};

const NSOpenGLPixelFormatAttribute* const* getPixelFormatsAttributes(int* size){
    static const NSOpenGLPixelFormatAttribute* const arr[] = {
        attrs32_1,
        attrs32_2,
        attrs32_3,
        attrs32_4,
        attrs32_5,
        attrs32_6,
        attrs32_7,
    };
    *size = sizeof(arr)/sizeof(arr[0]);
    return arr;
}

// Variants
static const NSOpenGLPixelFormatAttribute Legacy[] = {
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAWindow,
    NSOpenGLPFAPixelBuffer,
    0
};

static const NSOpenGLPixelFormatAttribute Core3_2[] = {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
    NSOpenGLPFADoubleBuffer,
    0
};

static const NSOpenGLPixelFormatAttribute Core4_1[] = {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
    NSOpenGLPFADoubleBuffer,
    0
};

static NSOpenGLPixelFormatAttribute sWantedCoreProfileLevel = 0;

void setCoreProfileLevel(NSOpenGLPixelFormatAttribute level) {
    sWantedCoreProfileLevel = level;
}

const NSOpenGLPixelFormatAttribute* const  getLegacyProfileAttributes() {
    return Legacy;
}

const NSOpenGLPixelFormatAttribute* const getCoreProfileAttributes() {
    if (sWantedCoreProfileLevel == NSOpenGLProfileVersion4_1Core) {
        return Core4_1;
    } else if (sWantedCoreProfileLevel == NSOpenGLProfileVersion3_2Core) {
        return Core3_2;
    } else {
        return Legacy;
    }
}
