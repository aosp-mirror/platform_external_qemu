// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use connection file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "os/log.h"

#include <stdarg.h>                        // for va_end, va_list, va_start
#include <fstream>                         // for basic_filebuf<>::int_type
#include <memory>                          // for allocator
#include <string>                          // for operator+, to_string, basi...

#include "aemu/base/logging/LogFormatter.h" // for VerboseLogFormatter
#include "aemu/base/files/PathUtils.h"      // for pj
#include "android/base/system/System.h"        // for System
#include "android/utils/path.h"                // for path_mkdir_if_needed

static constexpr std::string_view BLUETOOTH_LOG{"bluetooth"};
using android::base::System;
using android::base::VerboseLogFormatter;


namespace android::bluetooth {
std::shared_ptr<std::ostream> getLogstream(std::string_view id) {
    auto basedir = android::base::pj({
            System::get()->getTempDir(), BLUETOOTH_LOG.data(),
            std::to_string(System::get()->getCurrentProcessId())});

    if (path_mkdir_if_needed(basedir.c_str(), 0700) != 0) {
        dfatal("Unable to create bluetooth logging directory: %s",
                         basedir.c_str());
    }
    std::string filename = android::base::pj(basedir, id.data());
    for (int i = 0; System::get()->pathExists(filename); i++) {
        filename = android::base::pj(basedir,
                                     std::to_string(i) + "_" + std::string(id));
    }
    dinfo("Creating bluetooth log: %s", filename.c_str());

    return std::make_shared<std::ofstream>(filename, std::ios::binary);
}
}  // namespace android::bluetooth

extern "C" void __blue_write_to_file(LogSeverity prio,
                                     const char* file,
                                     int line,
                                     const char* fmt,
                                     ...) {
    static VerboseLogFormatter formatter;
    static std::shared_ptr<std::ostream> logStream =
            android::bluetooth::getLogstream("rootcanal.log");
    va_list args;
    va_start(args, fmt);
    std::string msg = formatter.format({file, line, prio}, fmt, args);
    va_end(args);

    if (!msg.empty()) {
        if (msg.back() == '\n') {
            *logStream << msg;
        } else {
            *logStream << msg << std::endl;
        }
    }
}
