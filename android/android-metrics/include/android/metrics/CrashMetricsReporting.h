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
#include "android/metrics/MetricsWriter.h"
#include "android/metrics/export.h"

namespace android {
namespace metrics {

//
// This function sends a bunch of metrics messages to report crashed sessions
// passed in |sessions| parameter using |reporter|.
//
AEMU_METRICS_API void reportCrashMetrics(
        MetricsReporter& reporter,
        const MetricsWriter::AbandonedSessions& sessions);

}  // namespace metrics
}  // namespace android
