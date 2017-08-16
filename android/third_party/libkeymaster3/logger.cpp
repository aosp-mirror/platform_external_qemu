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

#include <keymaster/logger.h>

namespace keymaster {

Logger* Logger::instance_ = 0;

/* static */
int Logger::Log(LogLevel level, const char* fmt, va_list args) {
    if (!instance_)
        return 0;
    return instance_->log_msg(level, fmt, args);
}

/* static */
int Logger::Log(LogLevel level, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = Log(level, fmt, args);
    va_end(args);
    return result;
}

/* static */
int Logger::Debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = Log(DEBUG_LVL, fmt, args);
    va_end(args);
    return result;
}

/* static */
int Logger::Info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = Log(INFO_LVL, fmt, args);
    va_end(args);
    return result;
}
/* static */
int Logger::Warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = Log(WARNING_LVL, fmt, args);
    va_end(args);
    return result;
}
/* static */
int Logger::Error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = Log(ERROR_LVL, fmt, args);
    va_end(args);
    return result;
}
/* static */
int Logger::Severe(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = Log(SEVERE_LVL, fmt, args);
    va_end(args);
    return result;
}


}  // namespace keymaster
