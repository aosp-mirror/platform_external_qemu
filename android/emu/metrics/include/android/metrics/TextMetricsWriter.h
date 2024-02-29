// Copyright 2017 The Android Open Source Project
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

#include "aemu/base/files/StdioStream.h"
#include "android/metrics/MetricsWriter.h"
#include "android/metrics/export.h"

#include <memory>

namespace android {
namespace metrics {

//
// TextMetricsWriter - a MetricsWriter implementation that serializes
// incoming messages as text and prints them to FILE* supplied at creation.
//

class AEMU_METRICS_API TextMetricsWriter final
    : public MetricsWriter,
      public std::enable_shared_from_this<TextMetricsWriter> {
public:
    using Ptr = std::shared_ptr<TextMetricsWriter>;
    using WPtr = std::weak_ptr<TextMetricsWriter>;

    static Ptr create(const std::string& sessionId,
                      base::StdioStream&& outStream);

    void write(const android_studio::AndroidStudioEvent& asEvent,
               wireless_android_play_playlog::LogEvent* logEvent) override;

private:
    TextMetricsWriter(const std::string& sessionId,
                      base::StdioStream&& outStream);

    base::StdioStream mOutStream;
};

}  // namespace metrics
}  // namespace android
