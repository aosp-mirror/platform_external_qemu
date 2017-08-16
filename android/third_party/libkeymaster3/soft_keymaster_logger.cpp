/*
 * Copyright 2015 The Android Open Source Project
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

#include <keymaster/soft_keymaster_logger.h>

#include <stdarg.h>
#include <syslog.h>

#define LOG_TAG "SoftKeymaster"
#include <cutils/log.h>

namespace keymaster {

int SoftKeymasterLogger::log_msg(LogLevel level, const char* fmt, va_list args) const {

    int android_log_level = ANDROID_LOG_ERROR;
    switch (level) {
    case DEBUG_LVL:
        android_log_level = ANDROID_LOG_DEBUG;
        break;
    case INFO_LVL:
        android_log_level = ANDROID_LOG_INFO;
        break;
    case WARNING_LVL:
        android_log_level = ANDROID_LOG_WARN;
        break;
    case ERROR_LVL:
        android_log_level = ANDROID_LOG_ERROR;
        break;
    case SEVERE_LVL:
        android_log_level = ANDROID_LOG_ERROR;
        break;
    }

    return LOG_PRI_VA(android_log_level, LOG_TAG, fmt, args);
}

}  // namespace keymaster
