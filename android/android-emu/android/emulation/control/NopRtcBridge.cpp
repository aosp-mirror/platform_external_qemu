
#include "android/emulation/control/RtcBridge.h"

namespace android {
namespace emulation {
namespace control {

NopRtcBridge::NopRtcBridge() = default;
NopRtcBridge::~NopRtcBridge() = default;

bool NopRtcBridge::connect(std::string identity) {
    return true;
}

void NopRtcBridge::disconnect(std::string identity) {}

bool NopRtcBridge::acceptJsepMessage(std::string identity, std::string msg) {
    return true;
}
bool NopRtcBridge::nextMessage(std::string identity,
                               std::string* nextMessage,
                               System::Duration blockAtMostMs) {
    *nextMessage =
            std::move("{ \"bye\" : \"good times, but i am not real.\" }");
    return true;
}

void NopRtcBridge::terminate() {}

bool NopRtcBridge::start() {
    return true;
}

RtcBridge::BridgeState NopRtcBridge::state() {
    return BridgeState::Connected;
}
}  // namespace control
}  // namespace emulation
}  // namespace android