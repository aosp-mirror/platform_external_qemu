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
#include "android/base/Backtrace.h"

#if defined(__WIN32) || defined(_MSC_VER)
namespace android {
namespace base {

std::string bt() {
    return "<bt not implemented>";
}

}  // namespace base
}  // namespace android

#else

#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <cxxabi.h>
#include <errno.h>
#include <libunwind.h>
#include <stdio.h>


namespace android {
namespace base {

// Implementation from:
// https://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/

#define E(fmt,...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

std::string bt() {
    unw_cursor_t cursor;
    unw_context_t context;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    std::stringstream ss;

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0) {
            break;
        }
        ss << std::hex << pc;

        char sym[256];
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            char* nameptr = sym;
            int status;
            char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
            if (status == 0) {
                nameptr = demangled;
            }
            ss << " (" << nameptr << ")+" << std::hex << offset << "\n";
            free(demangled);
        } else {
            printf(" -- error: unable to obtain symbol name for this frame\n");
        }
    }

    return ss.str();
}


} // namespace android
} // namespace base

#endif
