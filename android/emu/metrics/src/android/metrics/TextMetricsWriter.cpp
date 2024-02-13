// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/metrics/TextMetricsWriter.h"

#include "android/metrics/MetricsLogging.h"

#include "google_logs_publishing.pb.h"
#include "studio_stats.pb.h"

#include <utility>

namespace android {
namespace metrics {

TextMetricsWriter::Ptr TextMetricsWriter::create(
        const std::string& sessionId,
        base::StdioStream&& outStream) {
    return Ptr(new TextMetricsWriter(sessionId, std::move(outStream)));
}

TextMetricsWriter::TextMetricsWriter(
        const std::string& sessionId,
        base::StdioStream&& outStream)
    : MetricsWriter(sessionId), mOutStream(std::move(outStream)) {}

void TextMetricsWriter::write(
        const android_studio::AndroidStudioEvent& asEvent,
        wireless_android_play_playlog::LogEvent* logEvent) {
    fprintf(mOutStream.get(), "event time %" PRIi64 " ms\n",
            logEvent->event_time_ms());
    fprintf(mOutStream.get(), "{ %s }\n", asEvent.ShortDebugString().c_str());
    fflush(mOutStream.get());
}

}  // namespace metrics
}  // namespace android
