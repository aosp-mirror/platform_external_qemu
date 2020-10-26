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

static inline int amodem_add_inbound_call_vx(AModem modem, const char* args) { return 0; }
static inline void amodem_set_signal_strength_profile_vx(AModem modem, int quality) { }
static inline void amodem_receive_sms_vx(AModem modem, SmsPDU sms) { }
static inline int amodem_disconnect_call_vx(AModem modem, const char* args) { return 0; }
static inline int amodem_update_call_vx(AModem modem, const char* args, int state) { return 0; }
static inline void amodem_set_data_registration_vx(AModem modem, ARegistrationState state) { }
static inline void amodem_set_voice_registration_vx(AModem modem, ARegistrationState state) { }
static inline void amodem_set_data_network_type_vx(AModem modem, ADataNetworkType type) { }
