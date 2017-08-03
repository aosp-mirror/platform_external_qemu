// Copyright 2017 The Android Open Source Project
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

#include "android/utils/compiler.h"

#include "android/emulation/control/vm_operations.h"

ANDROID_BEGIN_HEADER

typedef int AndroidSnapshotStatus;

void androidSnapshot_initialize(const QAndroidVmOperations* vmOperations);
void androidSnapshot_finalize();

AndroidSnapshotStatus androidSnapshot_prepareForLoading(const char* name);
AndroidSnapshotStatus androidSnapshot_load(const char* name);

AndroidSnapshotStatus androidSnapshot_prepareForSaving(const char* name);
AndroidSnapshotStatus androidSnapshot_save(const char* name);

ANDROID_END_HEADER
