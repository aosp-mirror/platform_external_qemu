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
#include <stdio.h>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/logging/CLog.h"
#include "android/console.h"
#include "android/emulation/control/incubating/SensorService.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/physics/Physics.h"
#include "android/physics/physical_state_agent.h"
#include "android/utils/debug.h"
#include "google/protobuf/empty.pb.h"
#include "sensor_service.grpc.pb.h"
#include "sensor_service.pb.h"

#ifdef DISABLE_ASYNC_GRPC
#include "android/grpc/utils/SyncToAsyncAdapter.h"
#else
#endif

#define DEBUG 1

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("SensorImpl: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {
namespace incubating {

using SensorEventStream = GenericEventStreamWriter<SensorValue>;
using PhysicalModelValueEventStream =
        GenericEventStreamWriter<PhysicalModelValue>;

using SensorEventSupport = EventChangeSupport<SensorValue>;
using PhysicalModelEventSupport = EventChangeSupport<PhysicalModelValue>;

using google::protobuf::Empty;
using ::grpc::CallbackServerContext;
using ::grpc::ServerContext;
using ::grpc::Status;

class SensorImpl final
    : public SensorService::WithCallbackMethod_receivePhysicalModelEvents<
              SensorService::WithCallbackMethod_receiveSensorEvents<
                      SensorService::Service>> {
public:
    SensorImpl(const AndroidConsoleAgents* agents)
        : mConsoleAgents(agents), mSensorAgent(agents->sensors) {
        mQAndroidPhysicalStateAgent.context = this;
    }

    virtual ::grpc::ServerWriteReactor<PhysicalModelValue>*
    receivePhysicalModelEvents(::grpc::CallbackServerContext* context,
                               const PhysicalModelValue* request) override {
        dwarning(
                "Hijacking the physical state agent. You will likely see "
                "reduced functionality in the virtual sensors UI");
        mSensorAgent->setPhysicalStateAgent(&mQAndroidPhysicalStateAgent);

        // Record the last known state.
        PhysicalModelValue latest;
        getPhysicalModel(nullptr, request, &latest);

        std::lock_guard guard(mStateAccess);

        // Do not update the last know state if it already exists.
        // If it does we might drop an event.
        // Note: it is possible a client might get the same event 2x
        if (!mLastKnownPhysicalState.count(request->target())) {
            mLastKnownPhysicalState[request->target()] = latest.value();
        }

        // Create an async event forwarder
        auto event = new PhysicalModelValueEventStream(
                &mPhysicalListeners[request->target()]);

        // Send out current state.
        event->eventArrived(latest);
        return event;
    }

    virtual ::grpc::ServerWriteReactor<SensorValue>* receiveSensorEvents(
            CallbackServerContext* /*context*/,
            const SensorValue* request) override {
        dwarning(
                "Hijacking the physical state agent. You will likely see "
                "reduced functionality in the virtual sensors UI");
        mSensorAgent->setPhysicalStateAgent(&mQAndroidPhysicalStateAgent);

        // Record the last known state.
        SensorValue latest;
        getSensor(nullptr, request, &latest);

        std::lock_guard guard(mStateAccess);

        // Do not update the last know state if it already exists.
        // If it does we might drop an event.
        // Note: it is possible a client might get the same event 2x
        if (!mLastKnownSensorState.count(request->target())) {
            mLastKnownSensorState[request->target()] = latest.value();
        }

        // Create an async event forwarder
        auto event =
                new SensorEventStream(&mSensorListeners[request->target()]);

        // Send out current state.
        event->eventArrived(latest);
        return event;
    }

    Status getSensor(ServerContext* context,
                     const SensorValue* request,
                     SensorValue* reply) override {
        size_t size;
        mSensorAgent->getSensorSize((int)request->target() - 1, &size);
        std::vector<float> val(size, 0);
        std::vector<float*> out;
        for (size_t i = 0; i < val.size(); i++) {
            out.push_back(&val[i]);
        }

        int state = mSensorAgent->getSensor((int)request->target() - 1,
                                            out.data(), out.size());

        auto value = reply->mutable_value();
        for (size_t i = 0; i < size; i++) {
            value->add_data(val[i]);
        }
        reply->set_status(static_cast<SensorValue::SensorState>(state + 1));
        reply->set_target(request->target());
        return ::grpc::Status::OK;
    }

    Status setSensor(ServerContext* context,
                     const SensorValue* request,
                     ::google::protobuf::Empty* reply) override {
        android::base::ThreadLooper::runOnMainLooper(
                [agent = mSensorAgent, sensorValue = *request]() {
                    agent->setSensorOverride((int)sensorValue.target() - 1,
                                             sensorValue.value().data().data(),
                                             sensorValue.value().data().size());
                });
        return Status::OK;
    }

    Status setPhysicalModel(ServerContext* context,
                            const PhysicalModelValue* request,
                            Empty* reply) override {
        android::base::ThreadLooper::runOnMainLooper(
                [agent = mSensorAgent, physicalValue = *request]() {
                    agent->setPhysicalParameterTarget(
                            (int)physicalValue.target() - 1,
                            physicalValue.value().data().data(),
                            physicalValue.value().data().size(),
                            PhysicalInterpolation::PHYSICAL_INTERPOLATION_STEP);
                });
        return Status::OK;
    }

    Status getPhysicalModel(ServerContext* context,
                            const PhysicalModelValue* request,
                            PhysicalModelValue* reply) override {
        size_t size;
        mSensorAgent->getPhysicalParameterSize((int)request->target() - 1,
                                               &size);
        std::vector<float> val(size, 0);
        std::vector<float*> out;
        for (size_t i = 0; i < val.size(); i++) {
            out.push_back(&val[i]);
        }

        int state = mSensorAgent->getPhysicalParameter(
                (int)request->target() - 1, out.data(), out.size(),
                PARAMETER_VALUE_TYPE_CURRENT);

        auto value = reply->mutable_value();
        for (size_t i = 0; i < val.size(); i++) {
            value->add_data(val[i]);
        }
        reply->set_status(
                static_cast<PhysicalModelValue::PhysicalState>(state + 1));
        reply->set_target(request->target());
        return Status::OK;
    }

private:
    bool isEqual(const ParameterValue& lhs, const ParameterValue& rhs) {
        if (lhs.data_size() != rhs.data_size()) {
            return false;
        }

        for (int i = 0; i < lhs.data_size(); i++) {
            if (lhs.data(i) != rhs.data(i)) {
                return false;
            }
        }

        return true;
    }

    void broadcastUpdates() {
        std::lock_guard guard(mStateAccess);

        // We will only send out an event if a sensor
        // value has changed.

        // Let's start with the sensors..
        SensorValue sensorRequest;
        SensorValue sensorLatest;
        for (const auto& [key, val] : mLastKnownSensorState) {
            sensorRequest.set_target(key);
            getSensor(nullptr, &sensorRequest, &sensorLatest);
            if (!isEqual(val, sensorLatest.value())) {
                VERBOSE_INFO(grpc, "Broadcasting: %s",
                             sensorLatest.ShortDebugString().c_str());
                mSensorListeners[key].fireEvent(sensorLatest);
            }
        }

        // Now let's do the physical ones
        PhysicalModelValue physicalRequest;
        PhysicalModelValue physicalLatest;
        for (const auto& [key, val] : mLastKnownPhysicalState) {
            physicalRequest.set_target(key);
            getPhysicalModel(nullptr, &physicalRequest, &physicalLatest);
            if (!isEqual(val, physicalLatest.value())) {
                VERBOSE_INFO(grpc, "Broadcasting: %s",
                             physicalLatest.ShortDebugString().c_str());
                mPhysicalListeners[key].fireEvent(physicalLatest);
            }
        }
    }

    void static onTargetStateChanged(void* context) {
        // Ignored for now..
        DD("onTargetStateChanged.");
    }

    void static onPhysicalStateChanging(void* context) {
        // Ignored for now..
        DD("onPhysicalStateChanging.");
    }

    void static onPhysicalStateStabilized(void* context) {
        if (context != nullptr) {
            SensorImpl* service = reinterpret_cast<SensorImpl*>(context);
            service->broadcastUpdates();
        }
    }

    const AndroidConsoleAgents* mConsoleAgents;
    const QAndroidSensorsAgent* mSensorAgent;

    QAndroidPhysicalStateAgent mQAndroidPhysicalStateAgent = {
            .onTargetStateChanged = onTargetStateChanged,
            .onPhysicalStateChanging = onPhysicalStateChanging,
            .onPhysicalStateStabilized = onPhysicalStateStabilized,
    };

    std::unordered_map<SensorValue::SensorType, SensorEventSupport>
            mSensorListeners;
    std::unordered_map<PhysicalModelValue::PhysicalType,
                       PhysicalModelEventSupport>
            mPhysicalListeners;

    std::unordered_map<SensorValue::SensorType, ParameterValue>
            mLastKnownSensorState;
    std::unordered_map<PhysicalModelValue::PhysicalType, ParameterValue>
            mLastKnownPhysicalState;

    std::mutex mStateAccess;
};

grpc::Service* getSensorService(const AndroidConsoleAgents* agents) {
    return new SensorImpl(agents);
}

}  // namespace incubating
}  // namespace control
}  // namespace emulation
}  // namespace android
