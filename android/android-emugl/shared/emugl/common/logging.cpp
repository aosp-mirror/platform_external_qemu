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

#include "emugl/common/logging.h"

#include "render-utils/render_api_types.h"

namespace emugl {

void default_logger(char severity,
                    const char* file,
                    unsigned int line,
                    int64_t timestamp_us,
                    const char* message) {}

emugl_logger_t emugl_logger = default_logger;

void set_emugl_logger(emugl_logger_t f) {
    set_gfxstream_logger(f);
    if (!f) {
        emugl_logger = default_logger;
    } else {
        emugl_logger = f;
    }
}

}  // namespace emugl
