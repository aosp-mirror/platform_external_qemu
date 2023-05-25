// Copyright (C) 2022 The Android Open Source Project
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

#include "android-qemu2-glue/emulation/VirtioWifiForwarder.h"
#include "android-qemu2-glue/emulation/WifiService.h"

namespace android {
namespace qemu2 {

using slirp_rx_callback = std::function<ssize_t(const uint8_t*, size_t)>;

void libslirp_main_loop_wait(bool nonblocking);

Slirp* libslirp_init(slirp_rx_callback callback,
                     int restricted,
                     bool ipv4,
                     const char* vnetwork,
                     const char* vhost,
                     bool ipv6,
                     const char* vprefix6,
                     int vprefix6_len,
                     const char* vhost6,
                     const char* vhostname,
                     const char* tftp_export,
                     const char* bootfile,
                     const char* vdhcp_start,
                     const char* vnameserver,
                     const char* vnameserver6,
                     const char** dnssearch,
                     const char* vdomainname,
                     const char* tftp_server_name);

}  // namespace qemu2
}  // namespace android