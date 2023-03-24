// Copyright (C) 2019 The Android Open Source Project
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
#include "android/emulation/control/interceptor/LoggingInterceptor.h"

#include <assert.h>                       // for assert
#include <google/protobuf/message.h>      // for Message
#include <google/protobuf/text_format.h>  // for TextFormat::Printer, TextFo...
#include <inttypes.h>                     // for PRIu64
#include <algorithm>                      // for min
#include <utility>                        // for move

#include "android/base/system/System.h"  // for System
#include "android/utils/debug.h"         // for VERBOSE_PRINT, VERBOSE_grpc

namespace android {
namespace control {
namespace interceptor {

using namespace grpc::experimental;

const std::array<std::string, 4> InvocationRecord::kTypes{
        "UNARY", "CLIENT_STREAMING", "SERVER_STREAMING", "BIDI_STREAMING"};

std::array<std::string, 17> kStatus{"OK",
                                    "CANCELLED",
                                    "UNKNOWN",
                                    "INVALID_ARGUMENT",
                                    "DEADLINE_EXCEEDED",
                                    "NOT_FOUND",
                                    "ALREADY_EXISTS",
                                    "PERMISSION_DENIED",
                                    "RESOURCE_EXHAUSTED",
                                    "FAILED_PRECONDITION",
                                    "ABORTED",
                                    "OUT_OF_RANGE",
                                    "UNIMPLEMENTED",
                                    "INTERNAL",
                                    "UNAVAILABLE",
                                    "DATA_LOSS",
                                    "UNAUTHENTICATED"};

static uint64_t getTimeDiffUs(InvocationRecord loginfo,
                              InterceptionHookPoints from,
                              InterceptionHookPoints to) {
    assert(loginfo.mTimestamps[static_cast<int>(to)] >=
           loginfo.mTimestamps[static_cast<int>(from)]);

    return loginfo.mTimestamps[static_cast<int>(to)] -
           loginfo.mTimestamps[static_cast<int>(from)];
}

static void printLog(const InvocationRecord& loginfo) {
    auto status_msg = kStatus[std::min<int>(
            static_cast<int>(loginfo.status.error_code()), 16)];
    VERBOSE_PRINT(grpc,
                  "start: %" PRIu64 ", rcvTime: %" PRIu64 ", sndTime: %" PRIu64
                  ", rcv: %" PRIu64 ", snd: %" PRIu64 ", %s %s, %s(%s) -> [%s]",
                  loginfo.mTimestamps[InvocationRecord::kStartTimeIdx],
                  loginfo.rcvTime, loginfo.sndTime, loginfo.rcvBytes,
                  loginfo.sndBytes, status_msg.c_str(),
                  loginfo.status.error_message().c_str(),
                  loginfo.method.c_str(), loginfo.incoming.c_str(),
                  loginfo.response.c_str());
};

LoggingInterceptor::LoggingInterceptor(ServerRpcInfo* info,
                                       ReportingFunction reporter)
    : mReporter(reporter) {
    if (info) {
        mLoginfo.method = std::string(info->method()).substr(0, kMaxStringLen);
        mLoginfo.type = info->type();
    }
    mLoginfo.mTimestamps[InvocationRecord::kStartTimeIdx] =
            base::System::get()->getUnixTimeUs();
}

LoggingInterceptor::~LoggingInterceptor() {
    auto ts = base::System::get()->getUnixTimeUs();
    mLoginfo.duration =
            ts - mLoginfo.mTimestamps[InvocationRecord::kStartTimeIdx];
    mReporter(mLoginfo);
}

std::string LoggingInterceptor::formatProtobufMessage(
        const ::google::protobuf::Message* msg) {
    std::string debug_string;

    ::google::protobuf::TextFormat::Printer printer;
    printer.SetSingleLineMode(true);
    printer.SetExpandAny(true);
    printer.SetTruncateStringFieldLongerThan(kMaxProtbufStrlen);
    printer.PrintToString(*msg, &debug_string);

    // Single line mode currently might have an extra space at the end.
    if (debug_string.size() > 0 &&
        debug_string[debug_string.size() - 1] == ' ') {
        debug_string.resize(debug_string.size() - 1);
    }

    return debug_string;
}

std::string LoggingInterceptor::chopStr(std::string str) {
    if (str.size() <= kMaxStringLen)
        return str;
    return str.substr(0, kMaxStringLen - 3) + "...";
}

/*
Creation of interceptor object
Phase: [POST_RECV_INITIAL_METADATA, POST_RECV_MESSAGE]
Method invocation inside server.
Phase: [PRE_SEND_INITIAL_METADATA, PRE_SEND_MESSAGE,PRE_SEND_STATUS]
Phase: [POST_SEND_MESSAGE]
Phase: [POST_RECV_CLOSE]
 */
void LoggingInterceptor::Intercept(InterceptorBatchMethods* methods) {
    auto ts = base::System::get()->getUnixTimeUs();

    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::POST_RECV_MESSAGE)) {
        // Special case for streaming.. This is really just an approximation
        // of what is happening.. Increment the time spend on receiving
        // bytes..
        int selector = InvocationRecord::kStartTimeIdx;
        if (mLoginfo.type == ServerRpcInfo::Type::CLIENT_STREAMING ||
            mLoginfo.type == ServerRpcInfo::Type::BIDI_STREAMING)
            selector = static_cast<int>(
                    mLoginfo.rcvTime == 0
                            ? InterceptionHookPoints::POST_RECV_INITIAL_METADATA
                            : InterceptionHookPoints::POST_RECV_MESSAGE);

        mLoginfo.rcvTime += (ts - mLoginfo.mTimestamps[selector]);
    }

    // Note you can get many pre/post send in case of server streaming/bidi
    // Note you can get many pre/post recv in case of client streaming/bidi
    for (int i = 0;
         i < static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS);
         i++) {
        if (methods->QueryInterceptionHookPoint(
                    static_cast<InterceptionHookPoints>(i))) {
            mLoginfo.mTimestamps[i] = ts;
        }
    }

    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::POST_RECV_MESSAGE)) {
        // We just received a message from the client
        auto msg = reinterpret_cast<::google::protobuf::Message*>(
                methods->GetRecvMessage());
        if (msg) {
            auto size = msg->SpaceUsed();
            if (size < kMaxProtobufMsgLogSize && mLoginfo.rcvBytes == 0) {
                mLoginfo.incoming = formatProtobufMessage(msg);
            }
            mLoginfo.rcvBytes += size;
        }
    }

    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::PRE_SEND_MESSAGE)) {
        // We are ready to ship this message overseas!
        auto msg = reinterpret_cast<const ::google::protobuf::Message*>(
                methods->GetSendMessage());
        if (msg) {
            auto size = msg->SpaceUsed();
            if (size < kMaxProtobufMsgLogSize && mLoginfo.sndBytes == 0) {
                mLoginfo.response = formatProtobufMessage(msg);
            }
            mLoginfo.sndBytes += size;
        }
    }
    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::PRE_SEND_STATUS)) {
        mLoginfo.status = methods->GetSendStatus();
    }

    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::POST_SEND_MESSAGE)) {
        // Increment the time spend on sending bytes..
        mLoginfo.sndTime += getTimeDiffUs(
                mLoginfo, InterceptionHookPoints::PRE_SEND_MESSAGE,
                InterceptionHookPoints::POST_SEND_MESSAGE);
    }

    methods->Proceed();
}

LoggingInterceptorFactory::LoggingInterceptorFactory(ReportingFunction reporter)
    : mReporter(std::move(reporter)) {}

Interceptor* LoggingInterceptorFactory::CreateServerInterceptor(
        ServerRpcInfo* info) {
    return new LoggingInterceptor(info, mReporter);
};

StdOutLoggingInterceptorFactory::StdOutLoggingInterceptorFactory()
    : LoggingInterceptorFactory(printLog){};
}  // namespace interceptor
}  // namespace control
}  // namespace android
