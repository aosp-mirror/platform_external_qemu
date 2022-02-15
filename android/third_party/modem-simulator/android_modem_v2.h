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

#pragma once

#include "android/telephony/modem.h"
#include "android/telephony/sms.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

extern void init_modem_simulator();

extern int android_modem_version;

extern void amodem_receive_sms_vx(AModem modem, SmsPDU sms);

extern int amodem_add_inbound_call_vx(AModem modem, const char* args);

extern int amodem_disconnect_call_vx(AModem modem, const char* args);

extern int amodem_update_call_vx(AModem modem, const char* args, int state);

extern ACall amodem_find_call_by_number_vx(AModem modem, const char* args);

extern void amodem_set_data_network_type_vx(AModem modem,
                                            ADataNetworkType type);

extern void amodem_set_signal_strength_profile_vx(AModem modem, int quality);

extern void amodem_update_time(AModem modem);

extern void amodem_set_data_registration_vx(AModem modem,
                                            ARegistrationState state);
extern void amodem_set_voice_registration_vx(AModem modem,
                                             ARegistrationState state);

extern ARegistrationState amodem_get_data_registration_vx(AModem modem);

extern ARegistrationState amodem_get_voice_registration_vx(AModem modem);

extern void amodem_state_save_vx(AModem modem, SysFile* file);

extern int amodem_state_load_vx(AModem modem, SysFile* file, int version_id);

extern void amodem_set_notification_callback_vx(AModem modem,
                                                ModemCallback callbackFunc,
                                                void* userData);

ANDROID_END_HEADER
