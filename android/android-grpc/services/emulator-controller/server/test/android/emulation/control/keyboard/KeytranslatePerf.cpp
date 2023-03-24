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

#include <stddef.h>
#include <stdint.h>
#include <map>
// A small benchmark used to compare the performance of android::base::Lock
// with other mutex implementions.
#include <unordered_map>
#include <utility>

#include "aemu/base/ArraySize.h"
#include "android/emulation/control/keyboard/EmulatorKeyEventSender.h"
#include "benchmark/benchmark.h"

// Results will vary pending optimization. List is never worse
// than a map.
//#define OPTIMIZE __attribute__((noinline))
//#define OPTIMIZE inline

#define OPTIMIZE

const size_t kDomKeyMapEntries =
        ARRAY_SIZE(android::emulation::control::keyboard::dom_key_map);
const size_t kKeycodeMapEntries =
        ARRAY_SIZE(android::emulation::control::keyboard::usb_keycode_map);

// This is just to get multiple measurements, we do not rely on input size.
#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->Complexity()


std::map<android::emulation::control::keyboard::DomCode, uint32_t> gLookup;

 OPTIMIZE uint32_t domCodeToEvDevKeycodeMap(
        android::emulation::control::keyboard::DomCode key) {
    auto entry = gLookup.find(key);
    if (entry == gLookup.end())
        return 0;

    return entry->second;
}

void BM_TranslateKeysWithMap(benchmark::State& state) {
    for (size_t i = 0; i < kKeycodeMapEntries; ++i) {
        gLookup[android::emulation::control::keyboard::usb_keycode_map[i].id] =
                android::emulation::control::keyboard::usb_keycode_map[i].evdev;
    };
    while (state.KeepRunning()) {
        for (int i = 0; i < kKeycodeMapEntries; ++i) {
            domCodeToEvDevKeycodeMap(
                    android::emulation::control::keyboard::usb_keycode_map[i]
                            .id);
        }
    }
}


std::unordered_map<android::emulation::control::keyboard::DomCode, uint32_t> gUnorderedLookup;
OPTIMIZE uint32_t domCodeToEvDevKeycodeUnorderedMap(
        android::emulation::control::keyboard::DomCode key) {
    auto entry = gUnorderedLookup.find(key);
    if (entry == gUnorderedLookup.end())
        return 0;

    return entry->second;
}

void BM_TranslateKeysWithUnorderedMap(benchmark::State& state) {
    gUnorderedLookup.reserve(kKeycodeMapEntries);
    for (size_t i = 0; i < kKeycodeMapEntries; ++i) {
        gUnorderedLookup[android::emulation::control::keyboard::usb_keycode_map[i].id] =
                android::emulation::control::keyboard::usb_keycode_map[i].evdev;
    };
    while (state.KeepRunning()) {
        for (int i = 0; i < kKeycodeMapEntries; ++i) {
            domCodeToEvDevKeycodeUnorderedMap(
                    android::emulation::control::keyboard::usb_keycode_map[i]
                            .id);
        }
    }
}

OPTIMIZE uint32_t domCodeToEvDevKeycode(
        android::emulation::control::keyboard::DomCode key) {
    for (size_t i = 0; i < kKeycodeMapEntries; ++i) {
        if (android::emulation::control::keyboard::usb_keycode_map[i].id ==
            key) {
            return android::emulation::control::keyboard::usb_keycode_map[i]
                    .evdev;
        }
    }
    return 0;
}

void BM_TranslateKeysWithList(benchmark::State& state) {
    while (state.KeepRunning()) {
        for (int i = 0; i < kKeycodeMapEntries; ++i) {
            domCodeToEvDevKeycode(
                    android::emulation::control::keyboard::usb_keycode_map[i]
                            .id);
        }
    }
}

BASIC_BENCHMARK_TEST(BM_TranslateKeysWithMap);
BASIC_BENCHMARK_TEST(BM_TranslateKeysWithUnorderedMap);
BASIC_BENCHMARK_TEST(BM_TranslateKeysWithList);


BENCHMARK_MAIN();
