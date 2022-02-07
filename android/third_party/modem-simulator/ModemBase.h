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

namespace android {
namespace modem {

class ModemBase {
public:
    ModemBase() = default;
    virtual ~ModemBase() = default;

public:
    virtual void receive_sms(AModem, SmsPDU) = 0;
    virtual int add_inbound_call(AModem, const char*) = 0;
    virtual ACall find_call_by_number(AModem, const char*) = 0;
    virtual int disconnect_call(AModem, const char*) = 0;
    virtual int update_call(AModem, const char*, int) = 0;
    virtual void set_data_network_type(AModem, ADataNetworkType) = 0;
    virtual void set_signal_strength_profile(AModem, int) = 0;
    virtual void update_time(AModem) = 0;
    virtual void set_data_registration(AModem, ARegistrationState) = 0;
    virtual void set_voice_registration(AModem, ARegistrationState) = 0;
    virtual ARegistrationState get_data_registration(AModem) = 0;
    virtual ARegistrationState get_voice_registration(AModem) = 0;
    virtual void save_sate(AModem, SysFile*) = 0;
    virtual int load_sate(AModem modem, SysFile* file, int version_id) = 0;
    virtual void set_notification_callback_vx(AModem modem,
                                              ModemCallback callbackFunc,
                                              void* userData) = 0;
};

}  // namespace modem
}  // namespace android
