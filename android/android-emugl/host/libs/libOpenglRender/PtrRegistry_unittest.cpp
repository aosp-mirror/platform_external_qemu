// Copyright (C) 2017 The Android Open Source Project
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

#include "PtrRegistry.h"

#include <gtest/gtest.h>

TEST(PtrRegistry, Constructor) {
    PtrRegistry<void> reg;
}

TEST(PtrRegistry, Add) {
    PtrRegistry<void> reg;
    EXPECT_TRUE(reg.numCurrEntries() == 0);
    reg.addPtr(nullptr);
    EXPECT_TRUE(reg.numCurrEntries() == 1);
}

TEST(PtrRegistry, AddRemove) {
    PtrRegistry<void> reg;
    EXPECT_TRUE(reg.numCurrEntries() == 0);
    reg.addPtr(nullptr);
    EXPECT_TRUE(reg.numCurrEntries() == 1);
    reg.removePtr(nullptr);
    EXPECT_TRUE(reg.numCurrEntries() == 0);
}

TEST(PtrRegistry, AddMakeStale) {
    PtrRegistry<void> reg;
    EXPECT_TRUE(reg.numCurrEntries() == 0);
    reg.addPtr(nullptr);
    EXPECT_TRUE(reg.numCurrEntries() == 1);
    EXPECT_TRUE(reg.numStaleEntries() == 0);
    reg.makeCurrentPtrsStale();
    EXPECT_TRUE(reg.numCurrEntries() == 0);
    EXPECT_TRUE(reg.numStaleEntries() == 1);
}

TEST(PtrRegistry, Get) {
    PtrRegistry<void> reg;
    EXPECT_TRUE(reg.numCurrEntries() == 0);
    reg.addPtr(nullptr);
    EXPECT_TRUE(reg.numCurrEntries() == 1);
    EXPECT_TRUE(reg.getPtr(0) == nullptr);
}

TEST(PtrRegistry, GetNonExisting) {
    PtrRegistry<void> reg;
    EXPECT_TRUE(reg.numCurrEntries() == 0);
    EXPECT_TRUE(reg.getPtr(0) == nullptr);
    EXPECT_TRUE(reg.numCurrEntries() == 0);
}

TEST(PtrRegistry, AddMakeStaleGetStale) {
    PtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_TRUE(reg.numCurrEntries() == 0);

    reg.addPtr(ptr);

    EXPECT_TRUE(reg.numCurrEntries() == 1);
    EXPECT_TRUE(reg.numStaleEntries() == 0);

    reg.makeCurrentPtrsStale();

    EXPECT_TRUE(reg.numCurrEntries() == 0);
    EXPECT_TRUE(reg.numStaleEntries() == 1);

    EXPECT_TRUE(ptr == reg.getPtr(handle));
}

TEST(PtrRegistry, AddMakeStaleGetStaleWithDelete) {
    PtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_TRUE(reg.numCurrEntries() == 0);

    reg.addPtr(ptr);

    EXPECT_TRUE(reg.numCurrEntries() == 1);
    EXPECT_TRUE(reg.numStaleEntries() == 0);

    reg.makeCurrentPtrsStale();

    EXPECT_TRUE(reg.numCurrEntries() == 0);
    EXPECT_TRUE(reg.numStaleEntries() == 1);

    EXPECT_TRUE(ptr == reg.getPtr(handle, true));
    EXPECT_TRUE(nullptr == reg.getPtr(handle, true));
}

TEST(PtrRegistry, AddMakeStaleWithRemap) {
    PtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    void* remapped = (void*)0xbcdefa;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_TRUE(reg.numCurrEntries() == 0);

    reg.addPtr(ptr);

    EXPECT_TRUE(reg.numCurrEntries() == 1);
    EXPECT_TRUE(reg.numStaleEntries() == 0);

    reg.makeCurrentPtrsStale();

    EXPECT_TRUE(reg.numCurrEntries() == 0);
    EXPECT_TRUE(reg.numStaleEntries() == 1);

    reg.remapStalePtr(handle, remapped);

    EXPECT_TRUE(remapped == reg.getPtr(handle));
}

TEST(PtrRegistry, AddMakeStaleWithRemapNameCollision) {
    PtrRegistry<void> reg;
    void* ptr = (void*)0xabcdef;
    void* remapped = (void*)0xbcdefa;
    uint64_t handle = (uint64_t)(uintptr_t)ptr;

    EXPECT_TRUE(reg.numCurrEntries() == 0);

    reg.addPtr(ptr);

    EXPECT_TRUE(reg.numCurrEntries() == 1);
    EXPECT_TRUE(reg.numStaleEntries() == 0);

    reg.makeCurrentPtrsStale();

    EXPECT_TRUE(reg.numCurrEntries() == 0);
    EXPECT_TRUE(reg.numStaleEntries() == 1);

    reg.remapStalePtr(handle, remapped);

    EXPECT_TRUE(remapped == reg.getPtr(handle));

    reg.addPtr(ptr);

    EXPECT_TRUE(ptr == reg.getPtr(handle));
}
