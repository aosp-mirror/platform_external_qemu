
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

#include <iosfwd>
#include <string>
#include <thread>

#include "android/emulation/control/utils/EventSupport.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

class EndofLineObserver : public std::streambuf,
                          public EventChangeSupport<std::string> {
public:
    /**
     * An EndOfLineObserver fires a change event whenever a full line is written
     * to this streambuf.
     * @param buf The underlying streambuf to observe.
     */
    EndofLineObserver(std::streambuf* buf) : buf(buf) {}

protected:
    virtual int_type overflow(int_type c) override;

private:
    std::streambuf* buf;
    std::string currentLine;
};

class AdbLogcat : public EventChangeSupport<std::string> {
public:
    AdbLogcat();
    static AdbLogcat& instance();

private:
    void readStream();
    std::thread mStreamReader;
};

class LogcatEventStreamWriter
    : public BaseEventStreamWriter<LogMessage, std::string> {
public:
    LogcatEventStreamWriter(EventChangeSupport<std::string>* observing,
                            LogMessage request);

    void eventArrived(const std::string event) override;

private:
    LogMessage::LogType mSort;
};
}  // namespace control
}  // namespace emulation
}  // namespace android