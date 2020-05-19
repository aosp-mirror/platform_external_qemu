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
    cuttlefish::receive_inbound_call(std::string(args));
    return 0;
}

int ModemSimulator::disconnect_call(AModem modem, const char* args) {
    (void)modem;
    cuttlefish::disconnect_call(std::string(args));
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

void ModemSimulator::set_data_registration(AModem modem,
                                           ARegistrationState state) {
    (void)modem;
    cuttlefish::set_data_registration(state);
}

void ModemSimulator::set_voice_registration(AModem modem,
                                            ARegistrationState state) {
    (void)modem;
    cuttlefish::set_voice_registration(state);
}

}  // namespace modem
}  // namespace android
