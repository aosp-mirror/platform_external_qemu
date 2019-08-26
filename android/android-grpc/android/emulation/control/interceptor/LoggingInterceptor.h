#include <grpcpp/support/server_interceptor.h>

#include <array>

namespace android {
namespace control {
namespace interceptor {

using namespace grpc::experimental;
class LoggingInterceptorFactory;

// A simple logging interceptor that logs completed requests.
// Note that this does not provide accurate statistical insights
// for streaming endpoints.
class LoggingInterceptor : public grpc::experimental::Interceptor {
public:
    ~LoggingInterceptor();

    virtual void Intercept(InterceptorBatchMethods* methods) override;

private:
    LoggingInterceptor(ServerRpcInfo* info);
    uint64_t getTimeDiffUs(InterceptionHookPoints from,
                           InterceptionHookPoints to);
    std::string chopStr(std::string);

    // We will cut of all repsone/incoming strings at this length.
    const unsigned int kMaxStringLen = 80;
    const int kStartTimeIdx =
            static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS);
    static std::array<std::string, 4> kTypes;

    std::string mMethod;     // Invoked method.
    std::string mIncoming;   // Shortened receive parameters.
    std::string mResponse;   // Shortened response string.
    grpc::Status mStatus;          // Status
    uint64_t mRcvBytes = 0;  // Size of all received protobuf messages.
    uint64_t mRcvTime = 0;   // Time spend receiving bytes out over the wire.
    uint64_t mSndBytes = 0;  // Size of all send protobuf messages.
    uint64_t mSndTime = 0;   // Time spend sending bytes out over the wire.
    ServerRpcInfo::Type mType;

    // Timestamps of the various stages. We will use NUM_INTERCEPTION_HOOKS to
    // store the creation time
    uint64_t mTimestamps
            [static_cast<int>(InterceptionHookPoints::NUM_INTERCEPTION_HOOKS) +
             1];

    friend LoggingInterceptorFactory;
};

class LoggingInterceptorFactory
    : public grpc::experimental::ServerInterceptorFactoryInterface {
    virtual Interceptor* CreateServerInterceptor(ServerRpcInfo* info);
};

}  // namespace interceptor
}  // namespace control
}  // namespace android
