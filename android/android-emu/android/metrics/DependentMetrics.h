// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <stdbool.h>

#include "android/utils/compiler.h"  // for ANDROID_BEGIN_HEADER, ANDROID_EN...
#include "android/metrics/metrics.h"

ANDROID_BEGIN_HEADER

// |opaque| is assumed to be a pointer to the
// android_studio::AndroidStudioEvent protobuf. Since this is a C file used in
// many other C environments, we do not include the protobuf headers here.
void android_metrics_fill_common_info(bool openglAlive, void* opaque);

void android_metrics_report_common_info(bool openglAlive);


ANDROID_END_HEADER
