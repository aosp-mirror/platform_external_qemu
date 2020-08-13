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
#include "android/globals.h"
#include "modem_main.h"

//extern int android_modem_version;
void amodem_receive_sms_vx(AModem modem, SmsPDU sms) {
    if (android_modem_version == 1) {
        amodem_receive_sms(modem, sms);
    } else if (android_modem_version == 2) {
        cuttlefish::send_sms_msg(std::string(amodem_sms_to_string(modem, sms)));
    }
}

int amodem_add_inbound_call_vx(AModem modem, const char* args) {
    if (android_modem_version == 1) {
        return amodem_add_inbound_call(modem, args);
    } else if (android_modem_version == 2) {
        cuttlefish::receive_inbound_call(std::string(args));
    }
    return 0;
}

int amodem_disconnect_call_vx(AModem modem, const char* args) {
    if (android_modem_version == 1) {
        return amodem_disconnect_call(modem, args);
    } else if (android_modem_version == 2) {
        cuttlefish::disconnect_call(std::string(args));
    }
    return 0;
}

int amodem_update_call_vx(AModem modem, const char* args, int state) {
    if (android_modem_version == 1) {
        return amodem_update_call(modem, args, (ACallState)state);
    } else if (android_modem_version == 2) {
        cuttlefish::update_call(std::string(args), state);
    }
    return 0;
}

void amodem_set_data_network_type_vx(AModem modem, ADataNetworkType type) {
    if (android_modem_version == 1) {
        amodem_set_data_network_type(modem, type);
    } else if (android_modem_version == 2) {
        cuttlefish::set_data_network_type(type);
    }
}

void amodem_set_signal_strength_profile_vx(AModem modem, int quality) {
    if (android_modem_version == 1) {
        amodem_set_signal_strength_profile(modem, quality);
    } else if (android_modem_version == 2) {
        cuttlefish::set_signal_strength_profile(quality);
    }
}

void amodem_set_data_registration_vx(AModem modem, ARegistrationState state) {
    if (android_modem_version == 1) {
        amodem_set_data_registration(modem, state);
    } else if (android_modem_version == 2) {
        cuttlefish::set_data_registration(state);
    }
}

void amodem_set_voice_registration_vx(AModem modem, ARegistrationState state) {
    if (android_modem_version == 1) {
        amodem_set_voice_registration(modem, state);
    } else if (android_modem_version == 2) {
        cuttlefish::set_voice_registration(state);
    }
}
