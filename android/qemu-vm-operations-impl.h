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

#pragma once

#include "android/emulation/vm_operations.h"

ANDROID_BEGIN_HEADER

// Main entry point for populating a new QAndroidVMOperations for QEMU1.
// Caller takes ownership of returned object.
// See android/emulation/vm_operations.h to learn how to use this.
QAndroidVMOperations* qemu_get_android_vm_operations(void);

ANDROID_END_HEADER
