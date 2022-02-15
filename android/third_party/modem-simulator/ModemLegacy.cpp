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

#include "ModemLegacy.h"

namespace android {
namespace modem {

void ModemLegacy::receive_sms(AModem modem, SmsPDU sms) {
    amodem_receive_sms(modem, sms);
}

int ModemLegacy::add_inbound_call(AModem modem, const char* args) {
    return amodem_add_inbound_call(modem, args);
}

ACall ModemLegacy::find_call_by_number(AModem modem, const char* args) {
    return amodem_find_call_by_number(modem, args);
}

int ModemLegacy::disconnect_call(AModem modem, const char* args) {
    return amodem_disconnect_call(modem, args);
}

int ModemLegacy::update_call(AModem modem, const char* args, int state) {
    return amodem_update_call(modem, args, (ACallState)state);
}

void ModemLegacy::set_data_network_type(AModem modem, ADataNetworkType type) {
    amodem_set_data_network_type(modem, type);
}

void ModemLegacy::set_signal_strength_profile(AModem modem, int quality) {
    amodem_set_signal_strength_profile(modem, quality);
}

void ModemLegacy::update_time(AModem modem) {
    amodem_addTimeUpdate(modem);
}

void ModemLegacy::set_data_registration(AModem modem,
                                        ARegistrationState state) {
    amodem_set_data_registration(modem, state);
}

ARegistrationState ModemLegacy::get_data_registration(AModem modem) {
    return amodem_get_data_registration(modem);
}

ARegistrationState ModemLegacy::get_voice_registration(AModem modem) {
    return amodem_get_voice_registration(modem);
}

void ModemLegacy::set_voice_registration(AModem modem,
                                         ARegistrationState state) {
    amodem_set_voice_registration(modem, state);
}

void ModemLegacy::save_sate(AModem modem, SysFile* file) {
    amodem_state_save(modem, file);
}

int ModemLegacy::load_sate(AModem modem, SysFile* file, int version_id) {
    return amodem_state_load(modem, file, version_id);
}

void ModemLegacy::set_notification_callback_vx(AModem modem,
                                               ModemCallback callbackFunc,
                                               void* userData) {
    amodem_set_notification_callback(modem, callbackFunc, userData);
}

}  // namespace modem
}  // namespace android
