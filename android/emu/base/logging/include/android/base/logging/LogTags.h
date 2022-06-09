// Copyright (C) 2021 The Android Open Source Project
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
#ifdef __cplusplus
extern "C" {
#endif

#ifndef VERBOSE_TAG_LIST
    #error "_VERBOSE_TAG(xx, yy) List not defined."
#endif 

#define _VERBOSE_TAG(x, y) VERBOSE_##x,
typedef enum {
    VERBOSE_TAG_LIST VERBOSE_MAX /* do not remove */
} VerboseTag;
#undef _VERBOSE_TAG

extern uint64_t android_verbose;

#ifdef __cplusplus
// Make sure we don't accidentally add in to many tags..
static_assert(VERBOSE_MAX <= (sizeof(android_verbose) * 8));
#endif


#define VERBOSE_ENABLE(tag) android_verbose |= (1ULL << VERBOSE_##tag)
#define VERBOSE_DISABLE(tag) android_verbose &= (1ULL << VERBOSE_##tag)
#define VERBOSE_CHECK(tag) ((android_verbose & (1ULL << VERBOSE_##tag)) != 0)
#define VERBOSE_CHECK_ANY() (android_verbose != 0)

#define VERBOSE_PRINT(tag, ...)                      \
    if (VERBOSE_CHECK(tag)) {                        \
        EMULOG(EMULATOR_LOG_VERBOSE, ##__VA_ARGS__); \
    }

#define VERBOSE_INFO(tag, ...)                    \
    if (VERBOSE_CHECK(tag)) {                     \
        EMULOG(EMULATOR_LOG_INFO, ##__VA_ARGS__); \
    }

#define VERBOSE_DPRINT(tag, ...) VERBOSE_PRINT(tag, __VA_ARGS__)


#ifdef __cplusplus
}
#endif