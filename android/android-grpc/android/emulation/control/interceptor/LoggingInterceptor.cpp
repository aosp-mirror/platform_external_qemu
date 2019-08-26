#include "android/emulation/control/interceptor/LoggingInterceptor.h"

#include <android/base/Log.h>
#include <google/protobuf/message.h>

#include <algorithm>

#include "android/base/system/System.h"
namespace android {
namespace control {
namespace interceptor {

using namespace grpc::experimental;

std::array<std::string, 4> LoggingInterceptor::kTypes{
        "UNARY", "CLIENT_STREAMING", "SERVER_STREAMING", "BIDI_STREAMING"};

std::array<std::string, 17> kStatus{"OK",
                                    "CANCELLED",
                                    "UNKNOWN",
                                    "INVALID_ARGUMENT",
                                    "DEADLINE_EXCEEDED",
                                    "NOT_FOUND",
                                    "ALREADY_EXISTS",
                                    "PERMISSION_DENIED",
                                    "UNAUTHENTICATED",
                                    "RESOURCE_EXHAUSTED",
                                    "FAILED_PRECONDITION",
                                    "ABORTED",
                                    "OUT_OF_RANGE",
                                    "UNIMPLEMENTED",
                                    "INTERNAL",
                                    "UNAVAILABLE",
                                    "DATA_LOSS"};

static std::string statusToString(grpc::Status status) {
    if (status.error_code() == grpc::StatusCode::OK)
        return "OK";
    return kStatus[std::min<int>(static_cast<int>(status.error_code()), 16)]
            .append(":")
            .append(status.error_message());
}

uint64_t LoggingInterceptor::getTimeDiffUs(InterceptionHookPoints from,
                                           InterceptionHookPoints to) {
    assert(mTimestamps[static_cast<int>(to)] >
           mTimestamps[static_cast<int>(from)]);

    return mTimestamps[static_cast<int>(to)] -
           mTimestamps[static_cast<int>(from)];
}

LoggingInterceptor::LoggingInterceptor(ServerRpcInfo* info) {
    mMethod = std::string(info->method()).substr(0, kMaxStringLen);
    mTimestamps[kStartTimeIdx] = base::System::get()->getUnixTimeUs();
    mType = info->type();
}

LoggingInterceptor::~LoggingInterceptor() {
    LOG(INFO) << mTimestamps[kStartTimeIdx] << ", rcvTime: " << mRcvTime
              << ", sndTime: " << mSndTime << ", "
              << kTypes[static_cast<int>(mType)] << ", rcv: " << mRcvBytes
              << ", snd: " << mSndBytes << ", " << mMethod << "(" << mIncoming
              << ") -> [" << mResponse << "], " << statusToString(mStatus);
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
        int selector = kStartTimeIdx;
        if (mType == ServerRpcInfo::Type::CLIENT_STREAMING ||
            mType == ServerRpcInfo::Type::BIDI_STREAMING)
            selector = static_cast<int>(
                    mRcvTime == 0
                            ? InterceptionHookPoints::POST_RECV_INITIAL_METADATA
                            : InterceptionHookPoints::POST_RECV_MESSAGE);

        mRcvTime += (ts - mTimestamps[selector]);
    }

    // Note you can get many pre/post send in case of server streaming/bidi
    // Note you can get many pre/post recv in case of client streaming/bidi
    for (int i = 0;
         i < static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS);
         i++) {
        if (methods->QueryInterceptionHookPoint(
                    static_cast<InterceptionHookPoints>(i))) {
            mTimestamps[i] = ts;
        }
    }

    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::POST_RECV_MESSAGE)) {
        // We just received a message from the client
        auto msg = reinterpret_cast<::google::protobuf::Message*>(
                methods->GetRecvMessage());
        if (msg) {
            mIncoming = chopStr(msg->ShortDebugString());
            mRcvBytes += msg->SpaceUsed();
        }
    }

    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::PRE_SEND_MESSAGE)) {
        // We are ready to ship this message overseas!
        auto msg = reinterpret_cast<const ::google::protobuf::Message*>(
                methods->GetSendMessage());
        if (msg) {
            mResponse = chopStr(msg->ShortDebugString());
            mSndBytes += msg->SpaceUsed();
        }
    }
    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::PRE_SEND_STATUS)) {
        mStatus = methods->GetSendStatus();
    }

    if (methods->QueryInterceptionHookPoint(
                InterceptionHookPoints::POST_SEND_MESSAGE)) {
        // Increment the time spend on sending bytes..
        mSndTime += getTimeDiffUs(InterceptionHookPoints::PRE_SEND_MESSAGE,
                                  InterceptionHookPoints::POST_SEND_MESSAGE);
    }

    methods->Proceed();
}  // namespace interceptor

Interceptor* LoggingInterceptorFactory::CreateServerInterceptor(
        ServerRpcInfo* info) {
    return new LoggingInterceptor(info);
}
}  // namespace interceptor
}  // namespace control
}  // namespace android
