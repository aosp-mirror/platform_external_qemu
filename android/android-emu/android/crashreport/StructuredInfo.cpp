// Copyright 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/crashreport/StructuredInfo.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/crashreport/proto/crash_info.pb.h"

#include "android/featurecontrol/proto/emulator_feature_patterns.pb.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

namespace android {
namespace crashreport {

using namespace android_emulator;
using base::LazyInstance;
using base::Lock;
using base::AutoLock;

static LazyInstance<StructuredInfo> sStructuredInfo =
    LAZY_INSTANCE_INIT;

static LazyInstance<EmulatorCrashInfo> sCrashProto =
    LAZY_INSTANCE_INIT;

// static
StructuredInfo* StructuredInfo::get() {
    return sStructuredInfo.ptr();
}

void StructuredInfo::clear() {
    AutoLock lock(mLock);
    clearLocked();
}

void StructuredInfo::clearLocked() {
    sCrashProto.get().Clear();
}

void StructuredInfo::addHostHwInfo(const HostHwInfo::Info& info) {
    AutoLock lock(mLock);

    EmulatorHost* hostinfo = sCrashProto.get().mutable_emulator_host_info();

    hostinfo->set_virt_support(info.virt_support);
    hostinfo->set_running_in_vm(info.running_in_vm);

    for (const auto& hostgpu : info.gpuinfolist->infos) {
        EmulatorGpuInfo* outGpu = hostinfo->add_host_gpu();
        outGpu->set_make(hostgpu.make);
        outGpu->set_model(hostgpu.model);
        outGpu->set_device_id(hostgpu.device_id);
        outGpu->set_revision_id(hostgpu.revision_id);
        outGpu->set_version(hostgpu.version);
        outGpu->set_renderer(hostgpu.renderer);
    }

    fprintf(stderr, "%s: %s\n", __func__, hostinfo->DebugString().c_str());
}

void StructuredInfo::toBinaryString(std::string* res) const {
    AutoLock lock(mLock);
    sCrashProto.get().SerializeToString(res);
}

void StructuredInfo::reloadFromBinaryString(const std::string buf) {
    AutoLock lock(mLock);
    clearLocked();
    sCrashProto.get().ParseFromString(buf);
}

}  // crashreport
}  // android
