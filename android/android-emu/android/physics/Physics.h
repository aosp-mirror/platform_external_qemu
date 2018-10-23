/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef enum {
    PHYSICAL_INTERPOLATION_SMOOTH=0,
    PHYSICAL_INTERPOLATION_STEP=1,
} PhysicalInterpolation;

typedef enum {
    PARAMETER_VALUE_TYPE_TARGET = 0,
    PARAMETER_VALUE_TYPE_CURRENT = 1,
    PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION = 2,
    PARAMETER_VALUE_TYPE_DEFAULT = 3,
} ParameterValueType;

ANDROID_END_HEADER
