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
    virtual int update_call(AModem, const char*, int) override;
    virtual void set_data_network_type(AModem, ADataNetworkType) override;
    virtual void set_signal_strength_profile(AModem, int) override;
    virtual void set_data_registration(AModem, ARegistrationState) override;
    virtual void set_voice_registration(AModem, ARegistrationState) override;
};

}  // namespace modem
}  // namespace android
