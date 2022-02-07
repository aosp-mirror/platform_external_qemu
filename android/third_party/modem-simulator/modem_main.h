/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/telephony/modem.h"

#include <string>
#include <vector>

namespace cuttlefish {

enum ModemMessageType {
    MODEM_MSG_QUIT = -1,
    MODEM_MSG_SMS = 0,
    MODEM_MSG_CALL,
    MODEM_MSG_UPDATECALL,
    MODEM_MSG_ENDCALL,
    MODEM_MSG_CTEC,
    MODEM_MSG_SIGNAL,
    MODEM_MSG_DATA_REG,
    MODEM_MSG_VOICE_REG,
    MODEM_MSG_RADIO_STATE,
    MODEM_MSG_TIME_UPDATE,
};

struct ModemMessage {
    int type;
    std::vector<uint8_t> data;
    std::string sdata;
};

void send_sms_msg(std::string msg);

int receive_inbound_call(std::string number);

void disconnect_call(std::string number);

void update_call(std::string number, int state);

void set_data_network_type(int type);

void set_signal_strength_profile(int quality);

void update_time();

void set_data_registration(int state);

void set_voice_registration(int state);

void save_state(SysFile* file);

int load_state(SysFile* file, int version_id);

void set_notification_callback(void* callbackFunc, void* userData);

void queue_modem_message(ModemMessage msg);

// listening for guest RIL connection
// returns the actual port number
int start_android_modem_simulator_detached(int modem_simulator_port, bool& isIpv4);
int stop_android_modem_simulator();
}  // namespace cuttlefish
