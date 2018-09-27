// Copyright (C) 2018 The Android Open Source Project
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
#include <gtest/gtest.h>

#include "android/base/system/System.h"
#include "android/opengles.h"

#include <stdio.h>

using android::base::System;

TEST(Combined, Hello) {
    android_initOpenglesEmulation();

    int maj;
    int min;

    android_startOpenglesRenderer(256, 256, 1, 28, &maj, &min);
    fprintf(stderr, "%s: maj %d min %d.\n", __func__, maj, min);

    char* vendor;
    char* renderer;
    char* version;
    android_getOpenglesHardwareStrings(&vendor, &renderer, &version);

    fprintf(stderr, "%s: GL strings; [%s] [%s] [%s].\n", __func__,
            vendor, renderer, version);

    android_stopOpenglesRenderer(true);
}
