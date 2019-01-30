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

#include "OpenglRender/render_api_types.h"

#ifdef _MSC_VER
# ifdef BUILDING_EMUGL_COMMON_SHARED
#  define EMUGL_COMMON_API __declspec(dllexport)
# else
#  define EMUGL_COMMON_API __declspec(dllimport)
#endif
#else
# define EMUGL_COMMON_API
#endif

namespace emugl {

EMUGL_COMMON_API extern emugl_logger_t emugl_logger;
EMUGL_COMMON_API extern emugl_logger_t emugl_cxt_logger;
EMUGL_COMMON_API void set_emugl_logger(emugl_logger_t f);
EMUGL_COMMON_API void set_emugl_cxt_logger(emugl_logger_t f);

}  // namespace emugl
#define GL_LOGGING 1

#if GL_LOGGING

#define GL_LOG(...)                       \
    do {                                  \
        emugl::emugl_logger(__VA_ARGS__); \
    } while (0)

#define GL_CXT_LOG(...)                       \
    do {                                      \
        emugl::emugl_cxt_logger(__VA_ARGS__); \
    } while (0)

#else
#define GL_LOG(...) 0
#endif
