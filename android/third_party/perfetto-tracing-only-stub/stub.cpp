#include "../perfetto-tracing-only/perfetto-tracing-only.h"

namespace virtualdeviceperfetto {

// A stub implementation of perfetto tracing only.

static VirtualDeviceTraceConfig sTraceConfig = {
    .initialized = false,
    .tracingDisabled = true,
    .packetsWritten = 0,
    .sequenceIdWritten = 0,
    .currentInterningId = 1,
    .currentThreadId = 1,
    .hostFilename = "vmm.trace",
    .guestFilename = nullptr,
    .combinedFilename = nullptr,
    .hostStartTime = 0,
    .guestStartTime = 0,
    .guestTimeDiff = 0,
    .perThreadStorageMb = 1,
};

void setTraceConfig(std::function<void(VirtualDeviceTraceConfig&)> f) {
    f(sTraceConfig);
}

void initialize(const bool** tracingDisabledPtr) {
    *tracingDisabledPtr = &sTraceConfig.tracingDisabled;
}

VirtualDeviceTraceConfig queryTraceConfig() {
    return sTraceConfig;
}

void enableTracing() { }
void disableTracing() { }

void beginTrace(const char* eventName) { (void)eventName; }
void endTrace() { }
void traceCounter(const char* name, int64_t value) {
    (void)name;
    (void)value;
}

void setGuestTime(uint64_t t) {
    (void)t;
}

} // namespace perfetto
