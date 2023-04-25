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

#include "android/metrics/MetricsEngine.h"

#include "android/CommonReportedInfo.h"
#include "aemu/base/Optional.h"
#include "aemu/base/StringFormat.h"

#include "aemu/base/Uuid.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/files/IniFile.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/memory/ScopedPtr.h"
#include "android/base/system/System.h"
#include "android/cmdline-option.h"
#include "android/emulation/CpuAccelerator.h"
#include "host-common/FeatureControl.h"
#include "android/console.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/PerfStatReporter.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/StudioConfig.h"
#include "host-common/opengl/emugl_config.h"
#include "android/opengl/gpuinfo.h"
#include "host-common/opengles.h"
#include "android/utils/debug.h"
#include "android/utils/file_io.h"
#include "android/utils/x86_cpuid.h"

#include "studio_stats.pb.h"

#include <utility>

namespace android {
namespace metrics {

class MetricsEngineImpl : public MetricsEngine {
public:
    void registerReporter(std::shared_ptr<Reporter> reporter) override {
        mReporters.push_back(reporter);
    }

    void stop() override {
        for (auto& reporter : mReporters) {
            reporter->stop();
        }
    }

    void setEmulatorName(std::string name) override { mName = name; };
    std::string emulatorName() override { return mName; };

private:
    std::vector<std::shared_ptr<Reporter>> mReporters;
    std::string mName;
};
static android::base::LazyInstance<MetricsEngineImpl> sEngine = {};

MetricsEngine* MetricsEngine::get() {
    return sEngine.ptr();
}

}  // namespace metrics
}  // namespace android
