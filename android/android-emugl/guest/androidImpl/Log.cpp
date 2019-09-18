// Copyright 2018 The Android Open Source Project
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
#include <cstdarg>

#include <stdio.h>
#include <inttypes.h>

#include "AndroidHostCommon.h"

#define LOG_BUF_SIZE 1024

extern "C" {

EXPORT int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    va_end(ap);
    return 0;
}

EXPORT void __android_log_assert(const char* cond, const char* tag, const char* fmt, ...) {
    // Placeholder
}

EXPORT int __android_log_error_write(
    int tag, const char* subTag, int32_t uid,
    const char* data, uint32_t dataLen) {
    printf("android_log_error_write: "
           "tag: %d subtag: [%s] uid: %d datalen: %u data: [%s]\n",
           tag, subTag, uid, dataLen, data);
    return 0;
}

} // extern "C"
