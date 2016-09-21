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

#pragma once

#include "android/metrics/MetricsReporter.h"

#include <functional>

namespace android {
namespace metrics {

class MockMetricsReporter final : public MetricsReporter {
public:
    using OnReportConditional = std::function<void(ConditionalCallback)>;

    MockMetricsReporter();
    MockMetricsReporter(bool enabled,
                        MetricsWriter::Ptr writer,
                        base::StringView emulatorVersion,
                        base::StringView emulatorFullVersion,
                        base::StringView qemuVersion);

    void reportConditional(ConditionalCallback callback) override;
    int mReportConditionalCallsCount = 0;
    OnReportConditional mOnReportConditional;

    // Allow UTs access to this one as well.
    using MetricsReporter::sendToWriter;
};

}  // namespace metrics
}  // namespace android
