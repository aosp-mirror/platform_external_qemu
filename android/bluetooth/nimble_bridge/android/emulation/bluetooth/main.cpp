/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <chrono>    // for oper...
#include <fstream>   // for fstream
#include <memory>    // for uniq...
#include <optional>  // for opti...
#include <string>    // for string
#include <thread>    // for slee...
#include <utility>   // for move
#include <vector>    // for vector

#include "RequestForwarder.h"          // for Requ...
#include "absl/flags/flag.h"           // for Fast...
#include "absl/flags/parse.h"          // for Pars...
#include "absl/status/status.h"        // for Status
#include "absl/strings/string_view.h"  // for stri...
#include "host-common/hw-config.h"

#include "aemu/base/files/PathUtils.h"                           // for Path...
#include "android/emulation/control/EmulatorAdvertisement.h"     // for Emul...
#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for Emul...
#include "android/utils/debug.h"                                 // for dinfo
#include "emulated_bluetooth_device.pb.h"                        // for Gatt...
// #include "emulator_controller.pb.h"                              // for
// control

ABSL_FLAG(std::string,
          with_device_proto,
          "",
          "Path to a device configuration proto file describing the emulated "
          "device.");

ABSL_FLAG(std::string,
          with_hci_transport,
          "",
          "Path to a emulator endpoint configuration proto file hosting the "
          "HCI transport.");

using namespace android::emulation::bluetooth;
using namespace android::emulation::control;
using android::base::PathUtils;

std::optional<GattDevice> loadFromProto(std::string_view pathToEndpointProto) {
    std::fstream input(
            PathUtils::asUnicodePath(pathToEndpointProto.data()).c_str(),
            std::ios::in | std::ios::binary);
    GattDevice device;
    if (!input) {
        derror("File %s not found", pathToEndpointProto.data());
        return {};
    } else if (!device.ParseFromIstream(&input)) {
        derror("File %s does not contain a valid device",
               pathToEndpointProto.data());
        return {};
    }

    return device;
}

std::unique_ptr<EmulatorGrpcClient> findFirstEmulator() {
    auto emulators = EmulatorAdvertisement({}).discoverRunningEmulators();
    for (const auto& discovered : emulators) {
        auto client = EmulatorGrpcClient::Builder()
                              .withDiscoveryFile(discovered)
                              .build();
        if (client.ok()) {
            return std::move(client.value());
        } else {
            dwarning("Failed to configure connection: %s",
                     client.status().message().data());
        }
    }
    return nullptr;
}

int main(int argc, char** argv) {
    absl::ParseCommandLine(argc, argv);
    auto definition = loadFromProto(absl::GetFlag(FLAGS_with_device_proto));

    if (!definition.has_value()) {
        dfatal("No valid protobuf configutation provided");
    }

    auto maybeClient = EmulatorGrpcClient::loadFromProto(
            absl::GetFlag(FLAGS_with_hci_transport));
    while (!maybeClient.ok()) {
        using namespace std::chrono_literals;
        dinfo("Waiting for first emulator..");
        std::this_thread::sleep_for(1s);
        maybeClient = findFirstEmulator();
    }

    auto device = RequestForwarder::registerDevice(
            std::move(definition.value()), std::move(maybeClient.value()));

    auto status = device->initialize();
    if (status.ok()) {
        dinfo("Launching device.");
        device->start();
    }

    exit(0);
}
