// Copyright 2016 The Android Open Source Project
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

#include "android/avd/info.h"
#include "android/avd/util.h"
#include "android/cmdline-option.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

typedef struct QemuParameters QemuParameters;

QemuParameters* qemu_parameters_create(const char* argv0,
                                       const AndroidOptions* opts,
                                       const AvdInfo* avd,
                                       const char* androidHwInitPath,
                                       bool is_qemu2);

size_t qemu_parameters_size(const QemuParameters* params);
char** qemu_parameters_array(const QemuParameters* params);

void qemu_parameters_free(QemuParameters* params);

ANDROID_END_HEADER
