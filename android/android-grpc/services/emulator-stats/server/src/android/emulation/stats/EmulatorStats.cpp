// Copyright (C) 2023 The Android Open Source Project
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
#include "android/emulation/stats/EmulatorStats.h"

#include <grpcpp/grpcpp.h>
#include "android/base/system/System.h"
#include "emulator_stats.grpc.pb.h"
#include "emulator_stats.pb.h"

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using namespace android::base;

namespace android {
namespace emulation {
namespace stats {

class EmulatorStatsImpl final : public EmulatorStats::Service {
public:
    EmulatorStatsImpl() = default;
    Status getMemoryUsage(ServerContext* context,
                          ::google::protobuf::Empty* request,
                          MemoryUsage* reply) {
        base::MemUsage usage = android::base::System::get()->getMemUsage();
        reply->set_resident_memory(usage.resident);
        reply->set_resident_memory_max(usage.resident_max);
        reply->set_virtual_memory(usage.virt);
        reply->set_virtual_memory_max(usage.virt_max);
        reply->set_total_phys_memory(usage.total_phys_memory);
        reply->set_total_page_file(usage.total_page_file);
        return Status::OK;
    }
};

grpc::Service* getStatsService() {
    return new EmulatorStatsImpl();
}

}  // namespace stats
}  // namespace emulation
}  // namespace android