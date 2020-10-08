#include "perfetto.h"
#include "perfetto-tracing-only.h"

#include <fstream>

PERFETTO_DEFINE_CATEGORIES(
    ::perfetto::Category("gfx")
        .SetDescription("Events from the graphics subsystem"));
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

namespace virtualdeviceperfetto {

static bool sPerfettoInitialized = false;

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

VirtualDeviceTraceConfig queryTraceConfig() {
    return sTraceConfig;
}

void initialize(const bool** tracingDisabledPtr) {
    if (!sPerfettoInitialized) {
        ::perfetto::TracingInitArgs args;
        args.backends |= ::perfetto::kInProcessBackend;
        ::perfetto::Tracing::Initialize(args);
        ::perfetto::TrackEvent::Register();
        sPerfettoInitialized = true;
    }

    // An optimization to have faster queries of whether tracing is enabled.
    *tracingDisabledPtr = &sTraceConfig.tracingDisabled;
}

static std::unique_ptr<::perfetto::TracingSession> sTracingSession;

void enableTracing() {
    if (!sTracingSession) {
        auto desc = ::perfetto::ProcessTrack::Current().Serialize();
        desc.mutable_process()->set_process_name("VirtualMachineMonitorProcess");
        ::perfetto::TrackEvent::SetTrackDescriptor(::perfetto::ProcessTrack::Current(), desc);

        ::perfetto::TraceConfig cfg;
        ::perfetto::protos::gen::TrackEventConfig track_event_cfg;
        cfg.add_buffers()->set_size_kb(1024 * 100);  // Record up to 100 MiB.
        auto* ds_cfg = cfg.add_data_sources()->mutable_config();
        ds_cfg->set_name("track_event");
        ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());

        sTracingSession = ::perfetto::Tracing::NewTrace();
        sTracingSession->Setup(cfg);
        sTracingSession->StartBlocking();
        sTraceConfig.tracingDisabled = false;
    }
}

void disableTracing() {
    if (sTracingSession) {
        sTraceConfig.tracingDisabled = true;
        sTracingSession->StopBlocking();
        std::vector<char> trace_data(sTracingSession->ReadTraceBlocking());

        // Write the trace into a file.
        std::ofstream output;
        output.open(sTraceConfig.hostFilename, std::ios::out | std::ios::binary);
        output.write(&trace_data[0], trace_data.size());
        output.close();
        sTracingSession.reset();
    }
}

void beginTrace(const char* eventName) {
    TRACE_EVENT_BEGIN("gfx", ::perfetto::StaticString{eventName});
}

void endTrace() {
    TRACE_EVENT_END("gfx");
}

void traceCounter(const char* name, int64_t value) {
    // TODO: Why isn't this defined?
    // TRACE_COUNTER1("gfx", ::perfetto::StaticString{name}, value);
}

void setGuestTime(uint64_t t) {
    virtualdeviceperfetto::setTraceConfig([t](virtualdeviceperfetto::VirtualDeviceTraceConfig& config) {
        // can only be set before tracing
        if (!config.tracingDisabled) {
            return;
        }
        config.guestStartTime = t;
        config.hostStartTime = (uint64_t)(::perfetto::base::GetWallTimeNs().count());
        config.guestTimeDiff = config.guestStartTime - config.hostStartTime;
    });
}


} // namespace perfetto
