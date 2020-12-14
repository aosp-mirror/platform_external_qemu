// Copyright 2020 The Android Open Source Project
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

// A quick benchmark to test various approaches to convert RGBA -> RGB
// it tests 2 approaches so far.

#include <assert.h>                   // for assert
#include <cstring>                    // for memcpy
#include <cstdint>                    // for uint8_t
#include <vector>                     // for vector, vector<>::value_type

#include "benchmark/benchmark_api.h"  // for State, Benchmark, BENCHMARK

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)                \
            ->Arg(1 << 22)      \
            ->Arg(1 << 23)      \
            ->Arg(1 << 24)      \
            ->Arg(1 << 25)

// A fake set of bytes. Used to make sure we can
// validate the correctness of our conversion.
std::vector<uint8_t> fakeRGBAImage(int size) {
    std::vector<uint8_t> sz(size);
    for (int i = 0; i < size; i++) {
        sz[i] = i;
    }
    return sz;
}

std::vector<uint8_t>& convert_dma_byte(std::vector<uint8_t>& source,
                                       std::vector<uint8_t>& dest) {
    auto len = source.size() / 4 * 3;
    uint8_t* dst = dest.data();
    const uint8_t* src = source.data();
    const uint8_t* src_end = source.data() + source.size();
    int j = 0;
    while (src < src_end) {
        // Loop invariant when doing in place: assert(dst <= src);
        j++;
        if (j % 4 != 0) {
            *dst = *src;
            dst++;
        }
        src++;
    }
    dest.resize(len);
    return dest;
}

std::vector<uint8_t>& convert_dma_hexlet(std::vector<uint8_t>& source,
                                         std::vector<uint8_t>& dest) {
    // This only works if we have "native" support for uint128_t (which clang
    // has)
    static_assert(sizeof(__uint128_t) == 16);
    if (dest.size() < (source.size() / 4 * 3 + 16)) {
        dest.resize(source.size() / 4 * 3 + 16);
    }

    // an (uint32_t) ABGR value ends up like this in memory:
    //               |||\- [0] R
    //               ||\-- [1] G
    //               |\--- [2] B
    //               \-----[3] A

    // in a uint64_t  ABGR2 ABRG1
    // in a __uint128 ABGR4 ABGR3 ABGR2 ABRG1

    // The idea is that we are going to mask the Alpha byte
    // and move the bytes over so we turn

    // in a __uint128 ABGR4 ABGR3 ABGR2 ABRG1 --> 0x00000000 BGR4 BGR3 BGR2 BGR1
    // which end up in memory like:  R1,G1,B1,R2,G2,B2,R3,G3,B3,R4,G4,B4,0,0,0,0
    auto final_size = source.size() / 4 * 3;

    // Start & ending pointers.
    const uint8_t* src = source.data();
    const uint8_t* src_end = source.data() + source.size();
    uint8_t* dst = dest.data();

    // Various masks to mask out the alpha channel.
    constexpr __uint128_t RGB1 = 0xFFFFFF, RGB2 = RGB1 << 32, RGB3 = RGB2 << 32,
                          RGB4 = RGB3 << 32;

    // Make sure we can read at least 16 bytes.. (128 bits)
    // This guarantees that we do not access any memory we do not own.
    while (src + 16 < src_end) {
        // When doing in place this holds: assert(dst <= src); ..
        const __uint128_t pixel = *((__uint128_t*)src);
        __uint128_t* to_write = (__uint128_t*)dst;
        __uint128_t newValue = ((pixel & RGB4) >> 24) | ((pixel & RGB3) >> 16) |
                               ((pixel & RGB2) >> 8) | (pixel & RGB1);

        // Note that endianness is very important! the most significant bits end
        // up in the address furthest away resulting in 4 zero bytes! which will
        // be filled up in the next round (or will get chopped up in the end).
        *to_write = newValue;

        dst += 12;  // We wrote 12 bytes. (well 16 but the last 4 bytes we don't
                    // care for)
        src += 16;  // We read 16 bytes.
    }

    // Move the last 16 bytes if needed, this happens if we are not aligned to a
    // 128 bit boundary.
    int j = 0;
    while (src < src_end) {
        // assert(dst <= src);
        j++;
        if (j % 4 != 0) {
            *dst = *src;
            dst++;
        }
        src++;
    }

    // Shrink our vector
    dest.resize(final_size);
    return dest;
}

// A simple check to make sure we actually did the right thing.
bool check(const std::vector<uint8_t>& vec) {
    uint8_t i = 0, j = 0;
    for (uint8_t elem : vec) {
        if (elem != i) {
            return false;
        }
        i++;
        j++;
        // Skip?
        if (j == 3) {
            j = 0;
            i++;
        }
    }

    return true;
}


// This gives an idea of how fast a plain memcpy goes.
void BM_cnv_memcpy(benchmark::State& state) {
    auto img = fakeRGBAImage(state.range_x());
    std::vector<uint8_t> copy(img.size());
    while (state.KeepRunning()) {
        copy.resize(img.size());
        memcpy(copy.data(), img.data(), img.size());
    }
    state.SetBytesProcessed(state.iterations() * state.range_x());
}

// Test performance if we were doing an "inplace" conversion.
// Note: in place translation destroys the source, so
// we have to do an additional memcpy.
void BM_cnv_dma_byte_in_place(benchmark::State& state) {
    auto img = fakeRGBAImage(state.range_x());
    std::vector<uint8_t> copy(img.size());
    while (state.KeepRunning()) {

        // We will use this snippet everywhere, so we can see
        // the difference between inplace (move) vs copy
        copy.resize(img.size());
        memcpy(copy.data(), img.data(), img.size());

        auto x = convert_dma_byte(copy, copy);
        assert(check(x));
    }
    state.SetBytesProcessed(state.iterations() * state.range_x());
}

// Create the image in a new location.
void BM_cnv_dma_byte(benchmark::State& state) {
    auto img = fakeRGBAImage(state.range_x());
    std::vector<uint8_t> copy(img.size());
    std::vector<uint8_t> dst(img.size() * 4 / 3 + 16);
    while (state.KeepRunning()) {

        // We will use this snippet everywhere, so we can see
        // the difference between inplace (move) vs copy
        copy.resize(img.size());
        memcpy(copy.data(), img.data(), img.size());

        auto x = convert_dma_byte(img, dst);
        assert(check(x));
    }
    state.SetBytesProcessed(state.iterations() * state.range_x());
}

// Strip out the alpha channel using hexlets, and "in place"
// we operate on only a single buffer.
void BM_cnv_dma_hexlet_in_place(benchmark::State& state) {
    auto img = fakeRGBAImage(state.range_x());
    std::vector<uint8_t> copy(img.size());
    while (state.KeepRunning()) {

        // We will use this snippet everywhere, so we can see
        // the difference between inplace (move) vs copy
        copy.resize(img.size());
        memcpy(copy.data(), img.data(), img.size());

        auto x = convert_dma_hexlet(copy, copy);
        assert(check(x));
    }
    state.SetBytesProcessed(state.iterations() * state.range_x());
}

// Strip out the alpha channel using hexlets, and copy the
// results to a new buffer.
void BM_cnv_dma_hexlet(benchmark::State& state) {
    auto img = fakeRGBAImage(state.range_x());
    std::vector<uint8_t> dst(img.size() * 4 / 3 + 16);
    std::vector<uint8_t> copy(img.size());

    while (state.KeepRunning()) {
        // We will use this snippet everywhere, so we can see
        // the difference between inplace (move) vs copy
        copy.resize(img.size());
        memcpy(copy.data(), img.data(), img.size());

        auto x = convert_dma_hexlet(img, dst);
        assert(check(x));
    }
    state.SetBytesProcessed(state.iterations() * state.range_x());
}

// Strip out the alpha channel using hexlets
void BM_cnv_dma_hexlet_no_cpy(benchmark::State& state) {
    auto img = fakeRGBAImage(state.range_x());
    std::vector<uint8_t> dst(img.size() * 4 / 3 + 16);
    std::vector<uint8_t> copy(img.size());

    while (state.KeepRunning()) {
        auto x = convert_dma_hexlet(img, dst);
        assert(check(x));
    }
    state.SetBytesProcessed(state.iterations() * state.range_x());
}


BASIC_BENCHMARK_TEST(BM_cnv_dma_byte_in_place);
BASIC_BENCHMARK_TEST(BM_cnv_dma_byte);
BASIC_BENCHMARK_TEST(BM_cnv_dma_hexlet_in_place);
BASIC_BENCHMARK_TEST(BM_cnv_dma_hexlet);
BASIC_BENCHMARK_TEST(BM_cnv_memcpy);
BASIC_BENCHMARK_TEST(BM_cnv_dma_hexlet_no_cpy);
