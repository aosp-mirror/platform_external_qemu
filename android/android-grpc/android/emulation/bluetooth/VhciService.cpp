// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use connection file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VhciService.h"

#include <grpcpp/grpcpp.h>  // for ServerA...
#include <atomic>           // for atomic_...
#include <functional>       // for __base
#include <memory>           // for shared_ptr
#include <type_traits>      // for remove_...
#include <vector>           // for vector

#include "android/emulation/bluetooth/HciAsyncDataChannelAdapter.h"
#include "android/emulation/control/async/AsyncGrpcStream.h"  // for AsyncGr...
#include "android/utils/debug.h"                              // for dwarning
#include "grpc/support/time.h"                                // for gpr_now
#include "net/hci_datachannel_server.h"                       // for HciData...
#include "root_canal_qemu.h"                                  // for Rootcanal

// #define DEBUG 1
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace bluetooth {

AsyncVhciForwardingService* getVhciForwarder() {
    return new AsyncVhciForwardingService();
}

// True if a connection over gRPC <-> /dev/vhci is active
static std::atomic_bool sHijacked{false};

void registerAsyncVhciForwardingService(
        control::AsyncGrpcHandler* handler,
        AsyncVhciForwardingService* vhciService) {
    handler->registerConnectionHandler(
                   vhciService, &AsyncVhciForwardingService::RequestattachVhci)
            .withCallback([](auto connection) {
                bool expected = false;
                if (!sHijacked.compare_exchange_strong(expected, true)) {
                    // Bad news, we've already been hijacked.
                    connection->close(::grpc::Status(
                            ::grpc::StatusCode::FAILED_PRECONDITION,
                            "Another client is already controlling "
                            "/dev/vhci",
                            ""));
                    return;
                }

                // Serious hackers are taking control! BEWARE!
                dwarning(
                        "You have taken over /dev/vhci, rootcanal is now "
                        "disabled.");

                auto qemuHciServer =
                        android::bluetooth::Rootcanal::Builder::getInstance()
                                ->qemuHciServer();

                // Let's not fire any new events to anyone, we are hijacking
                // this connection.
                qemuHciServer->StopListening();

                // Close down the HCI connection to rootcanal.
                qemuHciServer->qemuChannel()->Close();

                // Create a new transport.
                auto qemuChannel = qemuHciServer->injectQemuChannel();

                // Let's wrap the qemu channel into an adapter that re-packages
                // our H4 packets from the outside world.
                static auto hciChannel = std::make_unique<
                        HciAsyncDataChannelAdapter>(
                        qemuChannel,
                        [connection](auto packet) {
                            connection->write(packet);
                        },
                        [connection](auto) {
                            // This would be a big surprise..
                            dwarning("Qemu channel unexpectedly closed down.");
                            connection->close(::grpc::Status::CANCELLED);
                        });

                connection->setReadCallback([](auto from, auto received) {
                    DD("From %s received %s", from->peer().c_str(),
                       received.ShortDebugString().c_str());

                    hciChannel->send(received);
                });

                // Handle the closing...
                connection->setCloseCallback([qemuHciServer](auto connection) {
                    if (!connection->isClosed()) {
                        dwarning("Partially closed connection, disconnecting");
                        connection->close(grpc::Status::CANCELLED);
                        return;
                    }
                    dwarning(
                            "Client closed down vHCI conncetion (%s) to %s, attempt "
                            "to restore rootcanal.",
                            connection->isCancelled() ? "cancelled" : "finished",
                            connection->peer().c_str());

                    // Best effor to re-activate rootcanal, but honestly all
                    // bets are off.
                    qemuHciServer->StartListening();
                    qemuHciServer->injectQemuChannel();
                    sHijacked = false;
                });
            });
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android