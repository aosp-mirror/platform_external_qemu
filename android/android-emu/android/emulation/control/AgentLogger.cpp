// Copyright (C) 2020 The Android Open Source Project
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
#include "android/emulation/control/AgentLogger.h"

#include <iostream>                             // for operator<<, basic_ost...
#include <sstream>                              // for stringstream
#include <string>                               // for basic_string

#include "android/base/Log.h"                   // for operator<<, LogStream...
#include "android/base/system/System.h"         // for System
#include "android/console.h"                    // for getConsoleAgents, And...
#include "android/skin/generic-event-buffer.h"  // for SkinGenericEventCode

using android::base::System;
using android::emulation::AndroidLoggingConsoleFactory;

// General macros to transform x into a string.
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Add support for logging SkinGenericEventCode's
std::ostream& operator<<(std::ostream& os, const SkinGenericEventCode& code) {
    os << "[" << code.type << ", " << code.code << ", " << code.value << ", "
       << code.displayId << "]";
    return os;
}

// Log an arbitrary set of parameters as a comma seperated list.
// for example:
//
// std::stringstream ss;
// log_params(ss, 1, 2, 3);
// ss.str() == "1, 2, 3"
// Base case, nothing..
inline void log_params(std::ostream& os) { }

// Base case, one thing.. no "," at the end.
template <typename Last>
inline void log_params(std::ostream& os, Last elem) {
    os << elem;
}

// Recursive case (2> params).
template <typename First, typename Second, typename ...Rest>
inline void log_params(std::ostream& os, First fs, Second sn, Rest... args) {
    log_params<First>(os, fs);
    os << ", ";
    log_params<Second, Rest...>(os, sn, args...);
}

// Logs all values in an array
template <typename T>
void log_array(const char* name, T* array, int count) {
     std::stringstream ss;
     ss << "[";
     for(int i = 0; i < count; i++) {
         if (i > 0)
            ss << ", ";
         ss << array[i];
     }
     ss << "]";
     LOG(INFO) << System::get()->getUnixTimeUs() << android::base::LogString(": %s: %s", name, ss.str().c_str());
}


// Function that will log an arbitrary of nr of types as:
// timstamp : <name> : <params>
template <typename ... Types>
void log_fun(const char* name, Types... args) {
    std::stringstream ss;
    log_params(ss, args...);
    LOG(INFO) << System::get()->getUnixTimeUs() << android::base::LogString(": %s: %s", name, ss.str().c_str());
}

// A nop, this will get optimized away..
template <typename ... Types>
void nop_fun(const char* name, Types... args) { }

// Invoke the function pointer with all supplied parameters.
template <typename R, typename ... Types>
R fwd_fun(R(*fn)(Types...), Types... args) {
    return fn(args...);
}

// Calls the function pre(const const char**, args...) before calling the
// actual function on the obj_struct with the given name.
#define agent_fwd_with_pre(obj_struct, name, pre) [](auto ...args) { \
    pre(TOSTRING(name), args...); \
    return fwd_fun(((obj_struct))->name, args...); \
}

// Forwards the call to the agent, logging the timestamp and parameters.
#define agent_fwd_with_logging(fn, name) agent_fwd_with_pre(fn, name, log_fun)

// Forwards the call to the agent, logging the timestamp and parameters,
// treating them as an array.
#define agent_fwd_with_logging_as_array(fn, name) agent_fwd_with_pre(fn, name, log_array)

// Forwards the function without modifying the behavior.
#define agent_fwd(fn, name) agent_fwd_with_pre(fn, name, nop_fun)


QAndroidUserEventAgent* realUserEventAgent;

// Forwards all calls to the realUserEventAgent, logging invoctions of some functions of interest.
QAndroidUserEventAgent sLoggingUserEventAgent = {
        .sendKey = agent_fwd(realUserEventAgent, sendKey),
        .sendKeyCode = agent_fwd_with_logging(realUserEventAgent, sendKeyCode),
        .sendKeyCodes = agent_fwd_with_logging_as_array(realUserEventAgent, sendKeyCodes),
        .sendMouseEvent = agent_fwd_with_logging(realUserEventAgent, sendMouseEvent),
        .sendRotaryEvent = agent_fwd(realUserEventAgent, sendRotaryEvent),
        .sendGenericEvent = agent_fwd_with_logging(realUserEventAgent, sendGenericEvent),
        .sendGenericEvents = agent_fwd_with_logging_as_array(realUserEventAgent, sendGenericEvents),
        .onNewUserEvent = agent_fwd(realUserEventAgent, onNewUserEvent),
        .eventsDropped = agent_fwd(realUserEventAgent, eventsDropped)};


const QAndroidUserEventAgent* const
AndroidLoggingConsoleFactory::android_get_QAndroidUserEventAgent() const {
    realUserEventAgent =
            (QAndroidUserEventAgent*)getConsoleAgents()->user_event;
    return &sLoggingUserEventAgent;
}
