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

#include "android_modem_v2.h"

#include <functional>
#include <mutex>
#include <thread>
#include "ModemLegacy.h"
#include "ModemSimulator.h"

static std::unique_ptr<android::modem::ModemBase> s_modem{
        new android::modem::ModemLegacy()};

extern "C" {
void init_modem_simulator() {
    static std::once_flag just_once;
    std::call_once(just_once, [&]() {
            s_modem.reset(new android::modem::ModemSimulator());
    });
}
}

void amodem_receive_sms_vx(AModem modem, SmsPDU sms) {
    s_modem->receive_sms(modem, sms);
}

int amodem_add_inbound_call_vx(AModem modem, const char* args) {
    return s_modem->add_inbound_call(modem, args);
}

int amodem_disconnect_call_vx(AModem modem, const char* args) {
    return s_modem->disconnect_call(modem, args);
}

int amodem_update_call_vx(AModem modem, const char* args, int state) {
    return s_modem->update_call(modem, args, state);
}

void amodem_set_data_network_type_vx(AModem modem, ADataNetworkType type) {
    s_modem->set_data_network_type(modem, type);
}

void amodem_set_signal_strength_profile_vx(AModem modem, int quality) {
    s_modem->set_signal_strength_profile(modem, quality);
}

void amodem_set_data_registration_vx(AModem modem, ARegistrationState state) {
    s_modem->set_data_registration(modem, state);
}

void amodem_set_voice_registration_vx(AModem modem, ARegistrationState state) {
    s_modem->set_voice_registration(modem, state);
}
