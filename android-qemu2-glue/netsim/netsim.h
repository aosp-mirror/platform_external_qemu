// Copyright 2023 The Android Open Source Project
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
#include <string>

#include "backend/packet_streamer_client.h"
#include "netsim/startup.pb.h"

using netsim::startup::ChipInfo;
using netsim::startup::DeviceInfo;

void register_netsim(const std::string address,
                     const std::string host_id,
                     const std::string dns_server,
                     const std::string netsim_args);

netsim::packet::NetsimdOptions get_netsim_options();

std::shared_ptr<DeviceInfo> get_netsim_device_info();

void close_netsim();