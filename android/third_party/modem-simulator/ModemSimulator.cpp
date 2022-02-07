/* Copyright (C) 2020 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "ModemSimulator.h"
#include "modem_main.h"

namespace android {
namespace modem {

void ModemSimulator::receive_sms(AModem modem, SmsPDU sms) {
    (void)modem;
    cuttlefish::send_sms_msg(std::string(amodem_sms_to_string(modem, sms)));
}

int ModemSimulator::add_inbound_call(AModem modem, const char* args) {
    (void)modem;
    if (!args)
        return 0;
    int res = cuttlefish::receive_inbound_call(std::string(args));
    if (res == A_CALL_OP_OK) {
        ACallRec rec;
        snprintf(rec.number, sizeof(rec.number), "%s", args);
        mCalls[std::string(args)] = rec;
    }
    return res;
}

ACall ModemSimulator::find_call_by_number(AModem, const char* args) {
    auto iter = mCalls.find(std::string(args));
    if (iter == mCalls.end()) {
        return nullptr;
    } else {
        return &(iter->second);
    }
}

int ModemSimulator::disconnect_call(AModem modem, const char* args) {
    (void)modem;
    if (!args)
        return 0;
    cuttlefish::disconnect_call(std::string(args));
    mCalls.erase(std::string(args));
    return 0;
}

int ModemSimulator::update_call(AModem modem, const char* args, int state) {
    (void)modem;
    cuttlefish::update_call(std::string(args), state);
    return 0;
}

void ModemSimulator::set_data_network_type(AModem modem,
                                           ADataNetworkType type) {
    (void)modem;
    cuttlefish::set_data_network_type(type);
}

void ModemSimulator::set_signal_strength_profile(AModem modem, int quality) {
    (void)modem;
    cuttlefish::set_signal_strength_profile(quality);
}

void ModemSimulator::update_time(AModem modem) {
    (void)modem;
    cuttlefish::update_time();
}

void ModemSimulator::set_data_registration(AModem modem,
                                           ARegistrationState state) {
    (void)modem;
    mDataState = state;
    cuttlefish::set_data_registration(state);
}

ARegistrationState ModemSimulator::get_data_registration(AModem modem) {
    (void)modem;
    return mDataState;
}

ARegistrationState ModemSimulator::get_voice_registration(AModem modem) {
    (void)modem;
    return mVoiceState;
}

void ModemSimulator::set_voice_registration(AModem modem,
                                            ARegistrationState state) {
    (void)modem;
    mVoiceState = state;
    cuttlefish::set_voice_registration(state);
}

void ModemSimulator::save_sate(AModem modem, SysFile* file) {
    (void)modem;
    cuttlefish::save_state(file);
}

int ModemSimulator::load_sate(AModem modem, SysFile* file, int version_id) {
    (void)modem;
    int res = 0;
    res = cuttlefish::load_state(file, version_id);
    return res;
}

void ModemSimulator::set_notification_callback_vx(AModem modem,
                                                  ModemCallback callbackFunc,
                                                  void* userData) {
    (void)modem;
    cuttlefish::set_notification_callback(reinterpret_cast<void*>(callbackFunc), userData);
}

}  // namespace modem
}  // namespace android
