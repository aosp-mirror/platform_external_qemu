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

void ModemLegacy::set_data_registration(AModem modem,
                                        ARegistrationState state) {
    amodem_set_data_registration(modem, state);
}

void ModemLegacy::set_voice_registration(AModem modem,
                                         ARegistrationState state) {
    amodem_set_voice_registration(modem, state);
}

}  // namespace modem
}  // namespace android
