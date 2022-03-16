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
#include <queue>
#include <type_traits>  // for remove_...
#include <vector>       // for vector

#include "android/emulation/bluetooth/HciAsyncDataChannelAdapter.h"
#include "android/emulation/control/utils/SimpleAsyncGrpc.h"
#include "android/utils/debug.h"         // for dwarning
#include "grpc/support/time.h"           // for gpr_now
#include "net/hci_datachannel_server.h"  // for HciData...
#include "root_canal_qemu.h"             // for Rootcanal

#define DEBUG 1
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

namespace android {
namespace emulation {
namespace bluetooth {

// True if a connection over gRPC <-> /dev/vhci is active
static std::atomic_bool sHijacked{false};

class AsyncVhciForwardingServiceImpl : public AsyncVhciForwardingService {
    ::grpc::ServerBidiReactor<HCIPacket, HCIPacket>* attachVhci(
            ::grpc::CallbackServerContext* context) override {
        class HciHandler
            : public SimpleBidiStream<HCIPacket, HCIPacket> {
        public:
            HciHandler(::grpc::CallbackServerContext* context)
                : mContext(context) {
                bool expected = false;
                if (!sHijacked.compare_exchange_strong(expected, true)) {
                    // Bad news, we've already been hijacked.
                    Finish(::grpc::Status(
                            ::grpc::StatusCode::FAILED_PRECONDITION,
                            "Another client is already controlling "
                            "/dev/vhci",
                            ""));
                }

                auto qemuHciServer =
                        android::bluetooth::Rootcanal::Builder::getInstance()
                                ->qemuHciServer();

                // Let's not fire any new events to anyone, we are hijacking
                // this connection.
                qemuHciServer->StopListening();

                // Close down the HCI connection to rootcanal.
                qemuHciServer->qemuChannel()->Close();

                // Create a new transport.
                mQemuChannel = qemuHciServer->injectQemuChannel();

                mHciChannel = std::make_unique<HciAsyncDataChannelAdapter>(
                        mQemuChannel,
                        [this](auto packet) { this->Write(packet); },
                        [this](auto) {
                            // This would be a big surprise..
                            dwarning("Qemu channel unexpectedly closed down.");
                            this->Finish(::grpc::Status::CANCELLED);
                        });

                hijacked = true;

                // Serious hackers are taking control! BEWARE!
                dwarning(
                        "You have taken over /dev/vhci, rootcanal is now "
                        "disabled.");
            }

            void Read(const HCIPacket* packet) override {
                mHciChannel->send(*packet);
            }

            void OnDone() override {
                if (hijacked) {
                    auto qemuHciServer = android::bluetooth::Rootcanal::
                                                 Builder::getInstance()
                                                         ->qemuHciServer();
                    qemuHciServer->StartListening();
                    qemuHciServer->injectQemuChannel();
                    sHijacked = false;
                }
                delete this;
            }

            void OnCancel() override { Finish(grpc::Status::CANCELLED); }

        private:
            ::grpc::CallbackServerContext* mContext;
            std::unique_ptr<HciAsyncDataChannelAdapter> mHciChannel;
            std::shared_ptr<AsyncDataChannel> mQemuChannel;

            bool hijacked{false};
        };

        return new HciHandler(context);
    }
};

AsyncVhciForwardingService* getVhciForwarder() {
    return new AsyncVhciForwardingServiceImpl();
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android