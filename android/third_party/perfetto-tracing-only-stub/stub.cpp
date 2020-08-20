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
    .filename = "stubtrace.trace",
    .guestStartTime = 0,
    .hostStartTime = 0,
    .useGuestStartTime = false,
};

static TraceWriterOperations sDefaultOperations = {
    .openSink = []() { },
    .closeSink = []() { },
    .writeToSink = [](uint8_t*, size_t) { },
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

} // namespace perfetto
