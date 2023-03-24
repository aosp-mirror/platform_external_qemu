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

#include "ModemBase.h"
#include "android/avd/info.h"
#include "android/console.h"
#include "android/emulation/control/cellular_agent.h"
#include "android/emulation/control/incubating/ModemService.h"
#include "android/emulation/control/telephony_agent.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/grpc/utils/EnumTranslate.h"
#include "android/telephony/modem.h"
#include "android/utils/debug.h"
#include "android_modem_v2.h"
#include "host-common/FeatureControl.h"
#include "host-common/VmLock.h"
#include "host-common/feature_control.h"
#include "host-common/hw-config.h"
#include "modem_service.grpc.pb.h"
#include "modem_service.pb.h"

#ifdef DISABLE_ASYNC_GRPC
#include "android/grpc/utils/SyncToAsyncAdapter.h"
#else
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#endif

// #define DEBUG 1
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

EventChangeSupport<PhoneEvent> s_activeCallListeners;

extern "C" {
static void telephony_callback(void* userData, int numActiveCalls);
}

using PhoneEventStream = GenericEventStreamWriter<PhoneEvent>;

static bool isValidPhoneNr(std::string number) {
    SmsAddressRec sender;
    bool invalid =
            sms_address_from_str(&sender, number.c_str(), number.size()) < 0;
    return !invalid;
}
class ModemImpl final
    : public Modem::WithCallbackMethod_receivePhoneEvents<Modem::Service>

{
public:
    ModemImpl(const AndroidConsoleAgents* agents)
        : mConsoleAgents(agents),
          mModem(agents->telephony->getModem()),
          mTelephonyAgent(agents->telephony) {
        // android::RecursiveScopedVmLock vmlock;
        mTelephonyAgent->setNotifyCallback(telephony_callback, (void*)this);
    }

    ::grpc::Status setCellInfo(::grpc::ServerContext* context,
                               const CellInfo* request,
                               CellInfo* response) override {
        android::RecursiveScopedVmLock vmlock;
        mConsoleAgents->cellular->setStandard(EnumTranslate::translate(
                mCellStandardMap, request->cell_standard()));

        if (request->cell_signal_strength().has_level()) {
            mConsoleAgents->cellular->setSignalStrengthProfile(
                    EnumTranslate::translate(
                            mCellSignal,
                            request->cell_signal_strength().level()));
        }
        if (request->cell_signal_strength().has_rssi()) {
            mConsoleAgents->cellular->setSignalStrength(
                    request->cell_signal_strength().rssi());
        }

        if (request->cell_identity().has_operatoralphalong()) {
            amodem_set_operator_name(mModem, A_NAME_LONG,
                                     request->cell_identity()
                                             .operatoralphalong()
                                             .value()
                                             .c_str(),
                                     request->cell_identity()
                                             .operatoralphalong()
                                             .value()
                                             .length());
        }

        if (request->cell_identity().has_operatoralphashort()) {
            amodem_set_operator_name(mModem, A_NAME_SHORT,
                                     request->cell_identity()
                                             .operatoralphashort()
                                             .value()
                                             .c_str(),
                                     request->cell_identity()
                                             .operatoralphashort()
                                             .value()
                                             .length());
        }

        mConsoleAgents->cellular->setSimPresent(request->sim_present());

        return ::grpc::Status::OK;
    }
    // Gets the current cell info.
    ::grpc::Status getCellInfo(::grpc::ServerContext* context,
                               const ::google::protobuf::Empty* request,
                               CellInfo* response) override {
        android::RecursiveScopedVmLock vmlock;
        constexpr std::size_t MAX_OPERATOR_NAME = 32;
        std::string long_name(MAX_OPERATOR_NAME, ' ');
        amodem_get_operator_name(mModem, A_NAME_LONG, long_name.data(),
                                 long_name.size());

        response->mutable_cell_identity()
                ->mutable_operatoralphalong()
                ->set_value(long_name);

        std::string short_name(MAX_OPERATOR_NAME, ' ');
        amodem_get_operator_name(mModem, A_NAME_SHORT, short_name.data(),
                                 short_name.size());
        response->mutable_cell_identity()
                ->mutable_operatoralphashort()
                ->set_value(short_name);

        if (feature_is_enabled(kFeature_ModemSimulator)) {
            auto state = amodem_get_voice_registration_vx(mModem);
            response->set_cell_status(EnumTranslate::translate(
                    mCellStatusRegistrationMap, state));
        }
        response->set_sim_present(amodem_get_sim(mModem));

        return ::grpc::Status::OK;
    }
    // Creates a new call object, this usually results in
    // the emulator receiving a new incoming call.
    ::grpc::Status createCall(::grpc::ServerContext* context,
                              const Call* request,
                              Call* response) override {
        android::RecursiveScopedVmLock vmlock;
        // Validate number
        if (!isValidPhoneNr(request->number())) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INVALID_ARGUMENT,
                    "The number: " + request->number() + " is invalid.");
        }

        // We sort of assume the call has status active..
        int result =
                amodem_add_inbound_call_vx(mModem, request->number().c_str());

        DD("Created: %d (%s)", result, request->number().c_str());
        if (result == A_CALL_RADIO_OFF) {
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION,
                                  "The radio is turned off.");
        }
        if (result == A_CALL_EXCEED_MAX_NUM) {
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION,
                                  "There are too many active calls.");
        }

        DD("Finding call");
        auto call = amodem_find_call_by_number_vx(mModem,
                                                  request->number().c_str());

        DD("Found: %p", call);
        if (call == nullptr) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INTERNAL,
                    "The call was created, but unable to report "
                    "on it's status (It might have been deleted?).");
        }

        translateCall(call, response);
        return ::grpc::Status::OK;
    }

    // Updates the state of the given call.
    // The unique number indentifying the call must exist.
    ::grpc::Status updateCall(::grpc::ServerContext* context,
                              const Call* request,
                              Call* response) override {
        android::RecursiveScopedVmLock vmlock;
        // Validate number
        if (!isValidPhoneNr(request->number())) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INVALID_ARGUMENT,
                    "The number: " + request->number() + " is invalid.");
        }

        auto call = amodem_find_call_by_number_vx(mModem,
                                                  request->number().c_str());

        if (call == nullptr) {
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND,
                                  "Unable to find call " + request->number());
        }

        ACallState updated =
                static_cast<ACallState>(static_cast<int>(request->state()) - 1);
        DD("Update call from %d->%d", call->state, updated);
        auto res = amodem_update_call_vx(mModem, request->number().c_str(),
                                         updated);
        if (res == A_CALL_NUMBER_NOT_FOUND) {
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND,
                                  "No call with the number: " +
                                          request->number() + ", exists.");
        }
        if (res < 0) {
            return ::grpc::Status(::grpc::StatusCode::INTERNAL,
                                  "Failed to update call");
        }

        call = amodem_find_call_by_number_vx(mModem, request->number().c_str());
        if (call == nullptr) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INTERNAL,
                    "The call was updated, but unable to report "
                    "on it's status (It might have been deleted?).");
        }

        DD("Call state is now %d", call->state);
        translateCall(call, response);
        return ::grpc::Status::OK;
    }

    // Removes the active call, this will disconnect
    // the call.
    ::grpc::Status deleteCall(::grpc::ServerContext* context,
                              const Call* request,
                              ::google::protobuf::Empty* response) override {
        android::RecursiveScopedVmLock vmlock;
        amodem_disconnect_call_vx(mModem, request->number().c_str());
        return ::grpc::Status::OK;
    }

    // Lists all the calls that are currently active in the modem
    ::grpc::Status listCalls(::grpc::ServerContext* context,
                             const ::google::protobuf::Empty* request,
                             ActiveCalls* response) override {
        android::RecursiveScopedVmLock vmlock;
        int activeCalls = amodem_number_of_calls_vx(mModem);
        DD("Found %d active calls", activeCalls);

        for (int i = 0; i < activeCalls; i++) {
            DD("Retrieving call %d", i);
            auto call = amodem_call_by_idx_vx(mModem, i);
            if (call == nullptr)
                continue;

            Call* active = response->add_calls();
            translateCall(call, active);
        }

        return ::grpc::Status::OK;
    }

    ::grpc::Status updateClock(::grpc::ServerContext* context,
                               const ::google::protobuf::Empty* request,
                               ::google::protobuf::Empty* response) override {
        android::RecursiveScopedVmLock vmlock;
        amodem_update_time(mModem);
        return ::grpc::Status::OK;
    }

    // The emulator will receive an sms message.
    ::grpc::Status receiveSms(::grpc::ServerContext* context,
                              const SmsMessage* request,
                              ::google::protobuf::Empty* response) override {
        if (request->text().length() > 1024) {
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                                  "A text message cannot contain more "
                                  "than 1024 characters");
        }
        if (!request->has_text() && !request->has_encodedmessage()) {
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                                  "No (encoded) message to deliver");
        }

        SmsAddressRec sender;
        if (sms_address_from_str(&sender, request->number().c_str(),
                                 request->number().length()) != 0) {
            return ::grpc::Status(
                    ::grpc::StatusCode::INVALID_ARGUMENT,
                    "The phone number:" + request->number() + " is invalid.");
        };

        if (request->has_text()) {
            SmsPDU* pdus = smspdu_create_deliver_utf8(
                    (uint8_t*)request->text().c_str(), request->text().size(),
                    &sender, NULL);
            if (pdus == nullptr) {
                return ::grpc::Status(
                        ::grpc::StatusCode::INVALID_ARGUMENT,
                        "The message text contains invalid characters.");
            }

            android::RecursiveScopedVmLock vmlock;
            for (int nn = 0; pdus[nn] != NULL; nn++) {
                amodem_receive_sms_vx(mModem, pdus[nn]);
            }

            smspdu_free_list(pdus);
        }
        if (request->has_encodedmessage()) {
            SmsPDU pdu =
                    smspdu_create_from_hex(request->encodedmessage().c_str(),
                                           request->encodedmessage().size());
            if (pdu == nullptr) {
                return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                                      "The encoded message contains an "
                                      "invalid hex string.");
            }

            android::RecursiveScopedVmLock vmlock;
            amodem_receive_sms_vx(mModem, pdu);
        }

        return ::grpc::Status::OK;
    }

    ::grpc::ServerWriteReactor<
            ::android::emulation::control::incubating::PhoneEvent>*
    receivePhoneEvents(::grpc::CallbackServerContext* /*context*/,
                       const ::google::protobuf::Empty* /*request*/) override {
        return new PhoneEventStream(&s_activeCallListeners);
    }

private:
    void translateCall(ACall call, Call* translated) {
        DD("State, direction: %d, state: %d, number: %s", call->dir,
           call->state, call->number);
        translated->set_direction(static_cast<Call_CallDirection>(
                static_cast<int>(call->dir) + 1));
        translated->set_state(
                static_cast<Call_CallState>(static_cast<int>(call->state) + 1));
        translated->set_number(call->number);
    }

    AModem mModem;
    const QAndroidTelephonyAgent* mTelephonyAgent;
    const AndroidConsoleAgents* mConsoleAgents;

    // internal <--> external proto mappings.
    static constexpr const std::tuple<CellInfo_CellStandard, CellularStandard>
            mCellStandardMap[] = {
                    {CellInfo::CELL_STANDARD_GSM, Cellular_Std_GSM},
                    {CellInfo::CELL_STANDARD_HSCSD, Cellular_Std_HSCSD},
                    {CellInfo::CELL_STANDARD_GPRS, Cellular_Std_GPRS},
                    {CellInfo::CELL_STANDARD_EDGE, Cellular_Std_EDGE},
                    {CellInfo::CELL_STANDARD_UMTS, Cellular_Std_UMTS},
                    {CellInfo::CELL_STANDARD_HSDPA, Cellular_Std_HSDPA},
                    {CellInfo::CELL_STANDARD_LTE, Cellular_Std_LTE},
                    {CellInfo::CELL_STANDARD_FULL, Cellular_Std_full},
                    {CellInfo::CELL_STANDARD_5G, Cellular_Std_5G},
                    {CellInfo::CELL_STANDARD_UNKNOWN, Cellular_Std_5G}};

    static constexpr const std::tuple<CellInfo_CellStatus, CellularStatus>
            mCellStatusMap[] = {
                    {CellInfo::CELL_STATUS_HOME, Cellular_Stat_Home},
                    {CellInfo::CELL_STATUS_ROAMING, Cellular_Stat_Roaming},
                    {CellInfo::CELL_STATUS_SEARCHING, Cellular_Stat_Searching},
                    {CellInfo::CELL_STATUS_DENIED, Cellular_Stat_Denied},
                    {CellInfo::CELL_STATUS_UNREGISTERED,
                     Cellular_Stat_Unregistered},
                    {CellInfo::CELL_STATUS_UNKNOWN,
                     Cellular_Stat_Unregistered}};

    static constexpr const std::tuple<CellInfo_CellStatus, ARegistrationState>
            mCellStatusRegistrationMap[] = {
                    {CellInfo::CELL_STATUS_HOME, A_REGISTRATION_HOME},
                    {CellInfo::CELL_STATUS_ROAMING, A_REGISTRATION_ROAMING},
                    {CellInfo::CELL_STATUS_SEARCHING, A_REGISTRATION_SEARCHING},
                    {CellInfo::CELL_STATUS_DENIED, A_REGISTRATION_DENIED},
                    {CellInfo::CELL_STATUS_UNREGISTERED,
                     A_REGISTRATION_UNREGISTERED},
                    {CellInfo::CELL_STATUS_UNKNOWN, A_REGISTRATION_UNKNOWN}};

    static constexpr const std::tuple<CellSignalStrength_CellSignalLevel,
                                      CellularSignal>
            mCellSignal[] = {
                    {CellSignalStrength::SIGNAL_STRENGTH_NONE_OR_UNKNOWN,
                     Cellular_Signal_None},
                    {CellSignalStrength::SIGNAL_STRENGTH_POOR,
                     Cellular_Signal_Poor},
                    {CellSignalStrength::SIGNAL_STRENGTH_MODERATE,
                     Cellular_Signal_Moderate},
                    {CellSignalStrength::SIGNAL_STRENGTH_GOOD,
                     Cellular_Signal_Good},
                    {CellSignalStrength::SIGNAL_STRENGTH_GREAT,
                     Cellular_Signal_Great}};
};

extern "C" {
static void telephony_callback(void* userData, int numActiveCalls) {
    if (s_activeCallListeners.size() == 0) {
        // No listeners, so no need to fire events.
        return;
    }

    DD("telephony_callback is invoked.. Listing calls");
    // So we updated our call count, let's fetch the active calls
    // and notify all the listeners.
    ModemImpl* modem = reinterpret_cast<ModemImpl*>(userData);
    PhoneEvent event;

    // ActiveCalls active;
    modem->listCalls(nullptr, nullptr, event.mutable_active());
    event.set_type(PhoneEvent::PHONE_EVENT_TYPE_ACTIVE);

    DD("Broadcast %s", event.ShortDebugString().c_str());
    s_activeCallListeners.fireEvent(event);
}
}

grpc::Service* getModemService(const AndroidConsoleAgents* telephone) {
    return new ModemImpl(telephone);
}

}  // namespace incubating
}  // namespace control
}  // namespace emulation
}  // namespace android