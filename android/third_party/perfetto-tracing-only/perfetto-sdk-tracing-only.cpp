#include "perfetto.h"
#include "perfetto-tracing-only.h"
#include "perfetto_trace.pb.h"

#include <string>
#include <thread>
#include <fstream>

PERFETTO_DEFINE_CATEGORIES(
    ::perfetto::Category("gfx")
        .SetDescription("Events from the graphics subsystem"));
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

#define TRACE_COUNTER(category, name, value)  \
    PERFETTO_INTERNAL_TRACK_EVENT(                  \
            category, name, \
            ::perfetto::protos::pbzero::TrackEvent::TYPE_COUNTER, [&](::perfetto::EventContext ctx){ \
            ctx.event()->set_counter_value(value); \
            })

#ifdef __cplusplus
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#else
#   define CC_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#   define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#endif

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

PERFETTO_TRACING_ONLY_EXPORT void setTraceConfig(std::function<void(VirtualDeviceTraceConfig&)> f) {
    f(sTraceConfig);
}

PERFETTO_TRACING_ONLY_EXPORT VirtualDeviceTraceConfig queryTraceConfig() {
    return sTraceConfig;
}

PERFETTO_TRACING_ONLY_EXPORT void initialize(const bool** tracingDisabledPtr) {
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

bool useFilenameByEnv(const char* s) {
    return s && ("" != std::string(s));
}

PERFETTO_TRACING_ONLY_EXPORT void enableTracing() {
    const char* hostFilenameByEnv = std::getenv("VPERFETTO_HOST_FILE");
    const char* guestFilenameByEnv = std::getenv("VPERFETTO_GUEST_FILE");
    const char* combinedFilenameByEnv = std::getenv("VPERFETTO_COMBINED_FILE");

    if (useFilenameByEnv(hostFilenameByEnv)) {
        sTraceConfig.hostFilename = hostFilenameByEnv;
    }

    if (useFilenameByEnv(guestFilenameByEnv)) {
        sTraceConfig.guestFilename = guestFilenameByEnv;
    }

    if (useFilenameByEnv(combinedFilenameByEnv)) {
        sTraceConfig.combinedFilename = combinedFilenameByEnv;
    }

    // Don't enable tracing if host filename is null
    if (!sTraceConfig.hostFilename) return;

    // Don't enable it twice
    if (!sTraceConfig.tracingDisabled) return;

    if (!sTracingSession) {
        fprintf(stderr, "%s: Tracing begins================================================================================\n", __func__);
        fprintf(stderr, "%s: Configuration:\n", __func__);
        fprintf(stderr, "%s: host filename: %s (possibly set via $VPERFETTO_HOST_FILE)\n", __func__, sTraceConfig.hostFilename);
        fprintf(stderr, "%s: guest filename: %s (possibly set via $VPERFETTO_GUEST_FILE)\n", __func__, sTraceConfig.guestFilename);
        fprintf(stderr, "%s: combined filename: %s (possibly set via $VPERFETTO_COMBINED_FILE)\n", __func__, sTraceConfig.combinedFilename);
        fprintf(stderr, "%s: guest time diff to add to host time: %llu\n", __func__, (unsigned long long)sTraceConfig.guestTimeDiff);

        auto desc = ::perfetto::ProcessTrack::Current().Serialize();
        desc.mutable_process()->set_process_name("VirtualMachineMonitorProcess");
        ::perfetto::TrackEvent::SetTrackDescriptor(::perfetto::ProcessTrack::Current(), desc);

        ::perfetto::TraceConfig cfg;
        ::perfetto::protos::gen::TrackEventConfig track_event_cfg;
        cfg.add_buffers()->set_size_kb(1024 * 100);  // Record up to 100 MiB.
        auto* ds_cfg = cfg.add_data_sources()->mutable_config();
        ds_cfg->set_name("track_event");
        ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());

        // Disable service events in the host trace, because they interfere
        // with the guest's and we end up dropping packets on one side or the other.
        auto* builtin_ds_cfg = cfg.mutable_builtin_data_sources();
        builtin_ds_cfg->set_disable_service_events(true);

        sTracingSession = ::perfetto::Tracing::NewTrace();
        sTracingSession->Setup(cfg);
        sTracingSession->StartBlocking();
        sTraceConfig.tracingDisabled = false;
    }
}

void asyncTraceSaveFunc() {
    fprintf(stderr, "%s: Saving combined trace async...\n", __func__);

    static const int kWaitSecondsPerIteration = 1;
    static const int kMaxIters = 20;
    static const int kMinItersForGuestFileSize = 2;

    const char* hostFilename = sTraceConfig.hostFilename;
    const char* guestFilename = sTraceConfig.guestFilename;
    const char* combinedFilename = sTraceConfig.combinedFilename;

    std::streampos currGuestSize = 0;
    int numGoodGuestFileSizeIters = 0;
    bool good = false;

    for (int i = 0; i < kMaxIters; ++i) {
        fprintf(stderr, "%s: Waiting for 1 second...\n", __func__);
        std::this_thread::sleep_for(std::chrono::seconds(kWaitSecondsPerIteration));
        fprintf(stderr, "%s: Querying file size of guest trace...\n", __func__);
        std::ifstream guestFile(guestFilename, std::ios::in | std::ios::binary | std::ios::ate);
        std::streampos size = guestFile.tellg();

        if (!size) {
            fprintf(stderr, "%s: No size, try again\n", __func__);
            continue;
        }

        if (size != currGuestSize) {
            fprintf(stderr, "%s: Sized changed (%llu to %llu), try again\n", __func__,
                    (unsigned long long)currGuestSize, (unsigned long long)size);
            currGuestSize = size;
            continue;
        }

        ++numGoodGuestFileSizeIters;

        if (numGoodGuestFileSizeIters == kMinItersForGuestFileSize) {
            fprintf(stderr, "%s: size is stable, continue saving\n", __func__);
            good = true;
            break;
        }
    }

    if (!good) {
        fprintf(stderr, "%s: Timed out when waiting for guest file to stabilize, skipping combined trace saving.\n", __func__);
        return;
    }

    std::ifstream hostFile(hostFilename, std::ios_base::binary);
    std::ifstream guestFile(guestFilename, std::ios_base::binary);
    std::ofstream combinedFile(combinedFilename, std::ios::out | std::ios_base::binary);

    combinedFile << guestFile.rdbuf() << hostFile.rdbuf();

    fprintf(stderr, "%s: Wrote combined trace (%s)\n", __func__, combinedFilename);
}

template <typename T>
static inline size_t varIntEncodingSize(T value) {
    // If value is <= 0 we must first sign extend to int64_t (see [1]).
    // Finally we always cast to an unsigned value to to avoid arithmetic
    // (sign expanding) shifts in the while loop.
    // [1]: "If you use int32 or int64 as the type for a negative number, the
    // resulting varint is always ten bytes long".
    // - developers.google.com/protocol-buffers/docs/encoding
    // So for each input type we do the following casts:
    // uintX_t -> uintX_t -> uintX_t
    // int8_t  -> int64_t -> uint64_t
    // int16_t -> int64_t -> uint64_t
    // int32_t -> int64_t -> uint64_t
    // int64_t -> int64_t -> uint64_t
    using MaybeExtendedType =
        typename std::conditional<std::is_unsigned<T>::value, T, int64_t>::type;
    using UnsignedType = typename std::make_unsigned<MaybeExtendedType>::type;

    MaybeExtendedType extended_value = static_cast<MaybeExtendedType>(value);
    UnsignedType unsigned_value = static_cast<UnsignedType>(extended_value);

    size_t bytes = 0;

    while (unsigned_value >= 0x80) {
        ++bytes;
        unsigned_value >>= 7;
    }

    return bytes + 1;
}

template <typename T>
static inline uint8_t* writeVarInt(T value, uint8_t* target) {
    // If value is <= 0 we must first sign extend to int64_t (see [1]).
    // Finally we always cast to an unsigned value to to avoid arithmetic
    // (sign expanding) shifts in the while loop.
    // [1]: "If you use int32 or int64 as the type for a negative number, the
    // resulting varint is always ten bytes long".
    // - developers.google.com/protocol-buffers/docs/encoding
    // So for each input type we do the following casts:
    // uintX_t -> uintX_t -> uintX_t
    // int8_t  -> int64_t -> uint64_t
    // int16_t -> int64_t -> uint64_t
    // int32_t -> int64_t -> uint64_t
    // int64_t -> int64_t -> uint64_t
    using MaybeExtendedType =
        typename std::conditional<std::is_unsigned<T>::value, T, int64_t>::type;
    using UnsignedType = typename std::make_unsigned<MaybeExtendedType>::type;

    MaybeExtendedType extended_value = static_cast<MaybeExtendedType>(value);
    UnsignedType unsigned_value = static_cast<UnsignedType>(extended_value);

    while (unsigned_value >= 0x80) {
        *target++ = static_cast<uint8_t>(unsigned_value) | 0x80;
        unsigned_value >>= 7;
    }
    *target = static_cast<uint8_t>(unsigned_value);
    return target + 1;
}

static std::vector<char> sProcessTrace(const std::vector<char>& trace) {
    std::vector<char> res;

    ::perfetto::protos::Trace pbtrace;
    std::string traceStr(trace.begin(), trace.end());

    if (pbtrace.ParseFromString(traceStr)) {
    } else {
        fprintf(stderr, "%s: Failed to parse protobuf as a string\n", __func__);
        return res;
    }

    fprintf(stderr, "%s: postprocessing trace with guest time diff of %llu\n", __func__,
            (unsigned long long)sTraceConfig.guestTimeDiff);

    for (int i = 0; i < pbtrace.packet_size(); ++i) {
        // Process one trace packet.
        auto* packet = pbtrace.mutable_packet(i);
        if (packet->has_timestamp()) {
            packet->set_timestamp(packet->timestamp() + sTraceConfig.guestTimeDiff);
        }
    }

    std::string traceAfter;
    pbtrace.SerializeToString(&traceAfter);

    std::vector<char> res2(traceAfter.begin(), traceAfter.end());
    return res2;
}

PERFETTO_TRACING_ONLY_EXPORT void disableTracing() {
    if (sTracingSession) {
        sTraceConfig.tracingDisabled = true;
        sTracingSession->StopBlocking();
        std::vector<char> trace_data(sTracingSession->ReadTraceBlocking());

        std::vector<char> processed =
            sProcessTrace(trace_data);

        fprintf(stderr, "%s: Tracing ended================================================================================\n", __func__);
        fprintf(stderr, "%s: Saving trace to disk. Configuration:\n", __func__);
        fprintf(stderr, "%s: host filename: %s\n", __func__, sTraceConfig.hostFilename);
        fprintf(stderr, "%s: guest filename: %s\n", __func__, sTraceConfig.guestFilename);
        fprintf(stderr, "%s: combined filename: %s\n", __func__, sTraceConfig.combinedFilename);

        fprintf(stderr, "%s: Saving host trace first...\n", __func__);

        // Write the trace into a file.
        std::ofstream output;
        output.open(sTraceConfig.hostFilename, std::ios::out | std::ios::binary);
        output.write(&processed[0], processed.size());
        output.close();
        sTracingSession.reset();

        fprintf(stderr, "%s: Saving host trace first...(done)\n", __func__);

        if (!sTraceConfig.guestFilename || !sTraceConfig.combinedFilename) {
            fprintf(stderr, "%s: skipping guest combined trace, "
                            "either guest file name (%p) not specified or "
                            "combined file name (%p) not specified\n", __func__,
                    sTraceConfig.guestFilename,
                    sTraceConfig.combinedFilename);
            return;
        }

        std::thread saveThread(asyncTraceSaveFunc);
        saveThread.detach();
    }
}

PERFETTO_TRACING_ONLY_EXPORT void beginTrace(const char* eventName) {
    TRACE_EVENT_BEGIN("gfx", ::perfetto::StaticString{eventName});
}

PERFETTO_TRACING_ONLY_EXPORT void endTrace() {
    TRACE_EVENT_END("gfx");
}

PERFETTO_TRACING_ONLY_EXPORT void traceCounter(const char* name, int64_t value) {
    // TODO: What this really needs until its supported in the official sdk:
    // a. a static global to track uuids and names for counters
    // b. track objects generated dynamically
    // c. setting the descriptor of these track objects
    // if (CC_LIKELY(sTraceConfig.tracingDisabled)) return;
    // TRACE_COUNTER("gfx", ::perfetto::StaticString{name}, value);
}

PERFETTO_TRACING_ONLY_EXPORT void setGuestTime(uint64_t t) {
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
