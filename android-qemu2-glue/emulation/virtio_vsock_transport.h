// Copyright 2021 The Android Open Source Project
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

#pragma once
#include <memory>
#include <android-qemu2-glue/base/files/QemuFileStream.h>

namespace android {
namespace emulation {

struct IVsockNewTransport : std::enable_shared_from_this<IVsockNewTransport> {
    virtual ~IVsockNewTransport() {}
    virtual void save() const {}
    virtual int writeGuestToHost(const void *data, size_t size,
                                 std::shared_ptr<IVsockNewTransport> *nextStage) = 0;
};

std::shared_ptr<IVsockNewTransport> vsock_create_transport_connector(uint64_t key);

std::shared_ptr<IVsockNewTransport> vsock_load_transport(android::base::Stream *stream);

void virtio_vsock_new_transport_init();

}  // namespace emulation
}  // namespace android
