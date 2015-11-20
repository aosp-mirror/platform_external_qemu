// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <gtest/gtest.h>
#include "android/telephony/modem.h"

namespace modem {

TEST(Modem, SignalStrength) {

    AModem modem = amodem_create(5554, NULL, NULL);

    int rssi = 15; // 0 .. 31
    int ber  = 99; // Always

    amodem_set_signal_strength(modem, rssi, ber);

    int retRssi = amodem_get_signal_strength(modem);

    EXPECT_EQ(retRssi, rssi);
}

TEST(Modem, VoiceDataRegistration) {

    AModem modem = amodem_create(5554, NULL, NULL);

    ARegistrationState voiceState = A_REGISTRATION_ROAMING;
    ARegistrationState dataState  = A_REGISTRATION_HOME;

    amodem_set_voice_registration(modem, voiceState);
    amodem_set_data_registration (modem, dataState);

    ARegistrationState retVoiceState = amodem_get_voice_registration(modem);
    ARegistrationState retDataState  = amodem_get_data_registration(modem);

    EXPECT_EQ(retVoiceState, voiceState);
    EXPECT_EQ(retDataState,  dataState);
}

}  // namespace modem
