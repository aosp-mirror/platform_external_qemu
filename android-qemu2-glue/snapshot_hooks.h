// Copyright 2017 The Android Open Source Project
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

#include <inttypes.h>

ANDROID_BEGIN_HEADER

// Sets up the snapshot hooks in QEMU.
void qemu_snapshot_hooks_setup();

// Convenience functions to associate host/guest RAM addresses.
void qemu_snapshot_ram_map(void* hva, uint64_t len, uint64_t gpa);
void qemu_snapshot_ram_unmap(uint64_t gpa, uint64_t len);

uint64_t qemu_snapshot_ram_hva2gpa(void* hva, bool* found);
void* qemu_snapshot_ram_gpa2hva(uint64_t gpa, bool* found);

// Function for guest to trigger faults.
void qemu_snapshot_guest_fault(uint64_t gpa, uint64_t len);

typedef uint64_t (*qemu_snapshot_ram_hva2gpa_t)(void*, bool*);
typedef void* (*qemu_snapshot_ram_gpa2hva_t)(uint64_t, bool*);
typedef void* (*qemu_snapshot_guest_fault_t)(uint64_t, uint64_t);

ANDROID_END_HEADER
