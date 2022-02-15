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

#include "ModemBase.h"

#include <string>
#include <unordered_map>

namespace android {
namespace modem {

// a wrapper of the legacy modem
class ModemSimulator : public ModemBase {
public:
    ModemSimulator() = default;
    virtual ~ModemSimulator() = default;

public:
    virtual void receive_sms(AModem, SmsPDU) override;
    virtual int add_inbound_call(AModem, const char*) override;
    virtual int disconnect_call(AModem, const char*) override;
    virtual ACall find_call_by_number(AModem, const char*) override;
    virtual int update_call(AModem, const char*, int) override;
    virtual void set_data_network_type(AModem, ADataNetworkType) override;
    virtual void set_signal_strength_profile(AModem, int) override;
    virtual void update_time(AModem) override;
    virtual void set_data_registration(AModem, ARegistrationState) override;
    virtual void set_voice_registration(AModem, ARegistrationState) override;
    virtual ARegistrationState get_data_registration(AModem) override;
    virtual ARegistrationState get_voice_registration(AModem) override;
    virtual void save_sate(AModem, SysFile*) override;
    virtual int load_sate(AModem modem, SysFile* file, int version_id) override;
    virtual void set_notification_callback_vx(AModem modem,
                                              ModemCallback callbackFunc,
                                              void* userData) override;

private:
    std::unordered_map<std::string, ACallRec> mCalls;
    ARegistrationState mDataState{A_REGISTRATION_HOME};
    ARegistrationState mVoiceState{A_REGISTRATION_HOME};
};

}  // namespace modem
}  // namespace android
