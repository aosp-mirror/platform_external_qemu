/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdio.h>

#include <keymaster/logger.h>

#define LOG_TAG "trusty_keymaster"
#define KEYMASTER_LOG_LEVEL WARNING_LVL

namespace keymaster {

class TrustyLogger : public Logger {
  public:
    static void initialize() {
        set_instance(new TrustyLogger);
    }

    virtual int log_msg(LogLevel level, const char* fmt, va_list args) const {
        //if (level < KEYMASTER_LOG_LEVEL) {
        //    return 0;
        //}

        int retval;
        switch (level) {
        case DEBUG_LVL:
            retval = fprintf(stdout, "%s (debug): ", LOG_TAG);
            break;
        case INFO_LVL:
            retval = fprintf(stdout, "%s (info): ", LOG_TAG);
            break;
        case WARNING_LVL:
            retval = fprintf(stdout, "%s (warn): ", LOG_TAG);
            break;
        case ERROR_LVL:
            retval = fprintf(stdout, "%s (err): ", LOG_TAG);
            break;
        case SEVERE_LVL:
            retval = fprintf(stdout, "%s (severe): ", LOG_TAG);
            break;
        default:
            retval = fprintf(stdout, "%s (critical): ", LOG_TAG);
            break;
        }
        retval += vfprintf(stdout, fmt, args);
        retval += fprintf(stdout, "\n");
        fflush(stdout);
        return retval;
    }
};

}  // namespace keymaster
