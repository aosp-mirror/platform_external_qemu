/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/utils/debug.h"

#include "host-common/logging.h"
#include "render-utils/render_api_types.h"

#ifdef _MSC_VER
#ifdef BUILDING_EMUGL_COMMON_SHARED
#define EMUGL_COMMON_API __declspec(dllexport)
#else
#define EMUGL_COMMON_API __declspec(dllimport)
#endif
#else
#define EMUGL_COMMON_API
#endif

namespace emugl {

EMUGL_COMMON_API extern gfxstream_logger_t emugl_logger;
EMUGL_COMMON_API void set_emugl_logger(gfxstream_logger_t f);

}  // namespace emugl
#define GL_LOGGING 1

#if GL_LOGGING

#define GL_LOG(fmt, ...)                                \
    do {                                                \
        GFXSTREAM_LOG(stderr, 'I', fmt, ##__VA_ARGS__); \
    } while (0)

#define GL_CXT_LOG(fmt, ...)                            \
    do {                                                \
        GFXSTREAM_LOG(stderr, 'D', fmt, ##__VA_ARGS__); \
    } while (0)

#else
#define GL_LOG(...) 0
#endif

// #define ENABLE_DECODER_LOG 1
#if defined(ENABLE_DECODER_LOG)
#define DECODER_DEBUG_LOG(...) GL_LOG(stderr, "I", __VA_ARGS__)
#else
#define DECODER_DEBUG_LOG(...) ((void)0)
#endif
