// Copyright 2016 The Android Open Source Project
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

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

int aemu_mutex_init(void** lock_addr);
int aemu_mutex_lock(void* lock_addr);
int aemu_mutex_unlock(void* lock_addr);

int aemu_cond_init(void** cv_addr);
int aemu_cond_wait(void* cv_addr, void* lock);
int aemu_cond_signal(void* cv_addr);

ANDROID_END_HEADER
