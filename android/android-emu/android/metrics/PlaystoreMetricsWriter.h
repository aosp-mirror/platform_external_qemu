// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include <stdint.h>                         // for uint32_t
#include <chrono>                           // for milliseconds
#include <memory>                           // for shared_ptr
#include <queue>                            // for queue
#include <string>                           // for string

#include "android/metrics/MetricsWriter.h"  // for MetricsWriter
#include "google_logs_publishing.pb.h" // IWYU pragma: keep

#ifdef NDEBUG
#define PLAYURL "https://play.googleapis.com/log"
#else  // !NDEBUG
#define PLAYURL "https://play.googleapis.com/staging/log"
#endif  // !NDEBUG

namespace android {
namespace metrics {

// A Metrics writer that reports events to Google when commit is called.
// The metrics writer will read in a cookie that contains the backoff time as
// not to violate backoff requirements.
//
// This metrics writer is expected to run in an ephemeral container where
// multiple runs of the emulator are very unlikely.
class PlaystoreMetricsWriter final : public MetricsWriter {
public:
    using Ptr = std::shared_ptr<PlaystoreMetricsWriter>;

    PlaystoreMetricsWriter(const std::string& sessionId,
                           const std::string& cookieFile,
                           std::string url = PLAYURL "?format=raw");
    ~PlaystoreMetricsWriter();

    void write(const android_studio::AndroidStudioEvent& asEvent,
               wireless_android_play_playlog::LogEvent* logEvent) override;
    void commit();

    // Creates a new instance of a PlaystoreMetricsWriter.
    //  |sessionId| - emulator session (run) ID for metrics logging.
    //  |cookieFile| - Cookie file used to store/retrieve mSendAfterMs.
    static Ptr create(const std::string& sessionId,
                      const std::string& cookieFile);

private:
    void writeCookie(std::string response);

    // Queue that stores events not using more than kMaxStorage of bytes
    std::queue<wireless_android_play_playlog::LogEvent> mEvents;
    const std::string mUrl;
    const std::string mCookieFile;
    uint32_t mBytes{0};       // # of bytes in our event queue, <= kMaxStorage.
    std::chrono::milliseconds mSendAfterMs{0}; // Earliest timestamp after which we send metrics.
    const uint32_t kMaxStorage{1024 * 128};  // 128 Kb.
};

}  // namespace metrics
}  // namespace android
