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

#include <grpcpp/grpcpp.h>

#include "android/console.h"
#include "android/emulation/control/car_data_agent.h"
#include "android/emulation/control/cellular_agent.h"
#include "android/emulation/control/incubating/CarService.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/grpc/utils/EnumTranslate.h"
#include "android/utils/debug.h"
#include "car_service.grpc.pb.h"
#include "car_service.pb.h"
#include "host-common/FeatureControl.h"
#include "host-common/VmLock.h"
#include "host-common/feature_control.h"
#include "host-common/hw-config.h"

#ifdef DISABLE_ASYNC_GRPC
#include "android/grpc/utils/SyncToAsyncAdapter.h"
#else
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#endif

#define DEBUG 1

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("ModemService: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {
namespace incubating {

using CarEventStream = GenericEventStreamWriter<CarEvent>;
using google::protobuf::Empty;

class CarImpl final : public CarService::WithCallbackMethod_sendCarEvents<
                              CarService::WithCallbackMethod_receiveCarEvents<
                                      CarService::Service>> {
public:
    CarImpl(const AndroidConsoleAgents* agents)
        : mConsoleAgents(agents), mCarDataAgent(agents->car) {
        mCarDataAgent->setCarCallback(&CarImpl::car_callback, &mListeners);
    }

    virtual ::grpc::Status sendCarEvent(
            ::grpc::ServerContext* context,
            const ::android::emulation::control::incubating::CarEvent* request,
            ::google::protobuf::Empty* response) override {
        mCarDataAgent->sendCarData(request->data().data(),
                                   request->data().size());
        return grpc::Status::OK;
    }

    virtual ::grpc::ServerReadReactor<CarEvent>* sendCarEvents(
            ::grpc::CallbackServerContext* /*context*/,
            ::google::protobuf::Empty* /*response*/) override {
        return new SimpleServerLambdaReader<CarEvent>([this](auto request) {
            mCarDataAgent->sendCarData(request->data().data(),
                                       request->data().size());
        });
    }

    ::grpc::ServerWriteReactor<CarEvent>* receiveCarEvents(
            ::grpc::CallbackServerContext* /*context*/,
            const ::google::protobuf::Empty* /*request*/) override {
        return new CarEventStream(&mListeners);
    }

private:
    static void car_callback(const char* msg, int len, void* context) {
        EventChangeSupport<CarEvent>* listeners =
                reinterpret_cast<EventChangeSupport<CarEvent>*>(context);
        CarEvent event;
        event.set_data(msg, len);
        listeners->fireEvent(event);
    }

    const AndroidConsoleAgents* mConsoleAgents;
    const QCarDataAgent* mCarDataAgent;
    EventChangeSupport<CarEvent> mListeners;
};

grpc::Service* getCarService(const AndroidConsoleAgents* agents) {
    if (feature_is_enabled(kFeature_CarVHalTable)) {
        return new CarImpl(agents);
    }
    return nullptr;
}

}  // namespace incubating
}  // namespace control
}  // namespace emulation
}  // namespace android
