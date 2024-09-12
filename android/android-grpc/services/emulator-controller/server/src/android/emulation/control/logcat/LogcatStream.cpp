
// Copyright (C) 2023 The Android Open Source Project
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

#include "android/emulation/control/logcat/LogcatStream.h"

#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "aemu/base/memory/LazyInstance.h"
#include "android/emulation/control/adb/AdbConnection.h"
#include "android/emulation/control/adb/AdbShellStream.h"
#include "android/emulation/control/logcat/LogcatParser.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#define ADB_LOG_DEBUG 0

/* set  >1 for very verbose debugging */
#if ADB_LOG_DEBUG <= 1
#define DD(...) (void)0
#else
#include "android/utils/debug.h"
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace control {

static android::base::LazyInstance<AdbLogcat> sAdbLogcat = LAZY_INSTANCE_INIT;

AdbLogcat::AdbLogcat() : mStreamReader([this]() { this->readStream(); }) {}

void AdbLogcat::readStream() {
    auto connection = AdbConnection::connection(std::chrono::milliseconds(500));
    AdbShellStream shell("logcat", connection);

    std::string currentLine;
    std::vector<char> sout;
    std::vector<char> serr;
    int exitCode;

    while (shell.read(sout, serr, exitCode)) {
        for (char c : sout) {
            if (c == '\n') {
                fireEvent(currentLine);
                currentLine.clear();
            } else {
                currentLine += c;
            }
        }
    }
}

AdbLogcat& AdbLogcat::instance() {
    return sAdbLogcat.get();
}

EndofLineObserver::int_type EndofLineObserver::overflow(int_type c) {
    if (c != traits_type::eof()) {
        char character = static_cast<char>(c);
        if (character == '\n') {
            fireEvent(currentLine);
            currentLine.clear();
        } else {
            currentLine += character;
        }
    }
    auto x = buf->sputc(c);
    return x;
}

LogcatEventStreamWriter::LogcatEventStreamWriter(
        EventChangeSupport<std::string>* observing,
        LogMessage request)
    : BaseEventStreamWriter<LogMessage, std::string>(observing),
      mSort(request.sort()) {}

void LogcatEventStreamWriter::eventArrived(std::string line) {
    LogMessage log;
    if (mSort == LogMessage::Parsed) {
        auto parsed = LogcatParser::parseLines(line);
        log.clear_entries();
        for (auto entry : parsed.second) {
            *log.add_entries() = entry;
        }
    } else {
        log.set_contents(line);
    }
    DD("Forwarding: %s", log.ShortDebugString().c_str());
    SimpleServerWriter<LogMessage>::Write(log);
}

}  // namespace control
}  // namespace emulation
}  // namespace android