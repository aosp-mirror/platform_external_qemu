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

#include "android/emulation/DmaMap.h"
#include "android/emulation/testing/TestDmaMap.h"

#include <gtest/gtest.h>


namespace android {


TEST(DmaMap, Default) {
    DmaMap* dmaMap = DmaMap::get();
    ASSERT_TRUE(dmaMap);
}

TEST(DmaMap, CustomDmaMap) {
    TestDmaMap myMap;
    const DmaMapState& currState = myMap.dumpState();
    EXPECT_EQ(currState.size(), 0);
}

TEST(DmaMap, SingleMapAdd) {
    TestDmaMap myMap;
    myMap.addBuffer(nullptr, 1, 2);
    const DmaMapState& currState = myMap.dumpState();
    const auto& it = currState.find(1);
    EXPECT_TRUE(it != currState.end());
    const DmaBufferInfo& info = it->second;
    EXPECT_EQ(info.guestAddr, 1);
    EXPECT_EQ(info.sz, 2);
    EXPECT_FALSE(info.hostAddrValid);
}

TEST(DmaMap, SingleMapAddRemove) {
    TestDmaMap myMap;
    const DmaMapState& currState = myMap.dumpState();
    myMap.addBuffer(nullptr, 1, 2);
    const auto& it = currState.find(1);
    EXPECT_TRUE(it != currState.end());
    myMap.removeBuffer(1);
    const auto& it2 = currState.find(1);
    EXPECT_TRUE(it2 == currState.end());
}

TEST(DmaMap, SingleMapAddMulti) {
    TestDmaMap myMap;
    myMap.addBuffer(nullptr, 1, 2);
    myMap.addBuffer(nullptr, 1, 3);
    const DmaMapState& currState = myMap.dumpState();
    const DmaBufferInfo& info = currState.find(1)->second;
    EXPECT_EQ(info.guestAddr, 1);
    EXPECT_EQ(info.sz, 3);
}

TEST(DmaMap, MultiMap) {
    TestDmaMap myMap;
    myMap.addBuffer(nullptr, 1, 2);
    myMap.addBuffer(nullptr, 3, 4);
    const DmaMapState& currState = myMap.dumpState();
    const auto& it1 = currState.find(1);
    const auto& it3 = currState.find(3);
    EXPECT_TRUE(it1 != currState.end());
    EXPECT_TRUE(it3 != currState.end());
    const DmaBufferInfo& info1 = it1->second;
    const DmaBufferInfo& info3 = it3->second;
    EXPECT_EQ(info1.guestAddr, 1);
    EXPECT_EQ(info1.sz, 2);
    EXPECT_FALSE(info1.hostAddrValid);
    EXPECT_EQ(info3.guestAddr, 3);
    EXPECT_EQ(info3.sz, 4);
    EXPECT_FALSE(info3.hostAddrValid);
    myMap.removeBuffer(1);
    myMap.removeBuffer(3);
    const auto& it1_notfound = currState.find(1);
    const auto& it3_notfound = currState.find(3);
    EXPECT_TRUE(it1_notfound == currState.end());
    EXPECT_TRUE(it3_notfound == currState.end());
}

TEST(DmaMap, SingleMapHostRead) {
    TestDmaMap myMap;
    myMap.addBuffer(nullptr, 1, 2);
    myMap.getHostAddr(1);

    const DmaMapState& currState = myMap.dumpState();
    const auto& it = currState.find(1);
    const DmaBufferInfo& info = it->second;
    EXPECT_EQ(info.guestAddr, 1);
    EXPECT_EQ(info.sz, 2);
    EXPECT_TRUE(info.hostAddrValid);

    myMap.getHostAddr(1);
    EXPECT_TRUE(currState.find(1)->second.hostAddrValid);
    myMap.getHostAddr(1);
    EXPECT_TRUE(currState.find(1)->second.hostAddrValid);
}

TEST(DmaMap, SingleMapHostReadInvalidate) {
    TestDmaMap myMap;
    myMap.addBuffer(nullptr, 1, 2);
    myMap.getHostAddr(1);
    myMap.invalidateHostMappings();
    const DmaMapState& currState = myMap.dumpState();
    const auto& it = currState.find(1);
    const DmaBufferInfo& info = it->second;
    EXPECT_EQ(info.guestAddr, 1);
    EXPECT_EQ(info.sz, 2);
    EXPECT_FALSE(info.hostAddrValid);
}

TEST(DmaMap, MultiMapHostReadInvalidate) {
    TestDmaMap myMap;
    const DmaMapState& currState = myMap.dumpState();
    myMap.addBuffer(nullptr, 1, 2);
    myMap.addBuffer(nullptr, 3, 4);
    myMap.addBuffer(nullptr, 5, 6);
    myMap.getHostAddr(1);
    myMap.getHostAddr(3);
    EXPECT_TRUE(currState.find(1)->second.hostAddrValid);
    EXPECT_TRUE(currState.find(3)->second.hostAddrValid);
    EXPECT_FALSE(currState.find(5)->second.hostAddrValid);
    myMap.invalidateHostMappings();
    EXPECT_FALSE(currState.find(1)->second.hostAddrValid);
    EXPECT_FALSE(currState.find(3)->second.hostAddrValid);
    EXPECT_FALSE(currState.find(5)->second.hostAddrValid);
}

TEST(DmaMap, MultiMapHostReadInvalidateRemap) {
    TestDmaMap myMap;
    const DmaMapState& currState = myMap.dumpState();
    myMap.addBuffer(nullptr, 1, 2);
    myMap.addBuffer(nullptr, 3, 4);
    myMap.addBuffer(nullptr, 5, 6);
    myMap.getHostAddr(1);
    myMap.getHostAddr(3);
    EXPECT_TRUE(currState.find(1)->second.hostAddrValid);
    EXPECT_TRUE(currState.find(3)->second.hostAddrValid);
    EXPECT_FALSE(currState.find(5)->second.hostAddrValid);
    myMap.invalidateHostMappings();
    myMap.getHostAddr(3);
    myMap.getHostAddr(5);
    EXPECT_FALSE(currState.find(1)->second.hostAddrValid);
    EXPECT_TRUE(currState.find(3)->second.hostAddrValid);
    EXPECT_TRUE(currState.find(5)->second.hostAddrValid);
}

TEST(DmaMap, ErrorCaseMapNonExistent) {
    TestDmaMap myMap;
    myMap.getHostAddr(1);
}

TEST(DmaMap, ErrorCaseRemoveNonExistent) {
    TestDmaMap myMap;
    myMap.removeBuffer(1);
}

} // namespace android
