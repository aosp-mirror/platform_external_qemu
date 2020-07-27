/* Copyright (C) 2015 The Android Open Source Project
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

#include "android-qemu2-glue/qemu-control-impl.h"

#include "android/android.h"
#include "android/emulation/control/cellular_agent.h"
#include "android/network/control.h"
#include "android/network/globals.h"
#include "android/shaper.h"
#include "android/telephony/modem_driver.h"

static void cellular_setSimPresent(bool isPresent)
{
    amodem_set_sim_present(android_modem, isPresent);
}

static void cellular_setSignalStrength(int zeroTo31)
{
    // (See do_gsm_signal() in android-qemu2-glue/console.c)

    if (android_modem) {
        if (zeroTo31 <  0) zeroTo31 =  0;
        if (zeroTo31 > 31) zeroTo31 = 31;

        amodem_set_signal_strength(android_modem, zeroTo31, 99);
    }
}

static void cellular_setSignalStrengthProfile(enum CellularSignal signal)
{
    // (See do_gsm_signal_profile() in android-qemu2-glue/console.c)
    if (android_modem) {
        int signalQuality = -1;
        switch (signal) {
            case Cellular_Signal_None: signalQuality = 0; break;
            case Cellular_Signal_Poor: signalQuality = 1; break;
            case Cellular_Signal_Moderate: signalQuality = 2; break;
            case Cellular_Signal_Good: signalQuality = 3; break;
            case Cellular_Signal_Great: signalQuality = 4; break;
        }
        if (signalQuality >= 0) {
            amodem_set_signal_strength_profile(android_modem, signalQuality);
        }
    }
}

static void cellular_setVoiceStatus(enum CellularStatus voiceStatus)
{
    // (See do_gsm_voice() in android-qemu2-glue/console.c)
    ARegistrationState  state = A_REGISTRATION_UNKNOWN;

    if (android_modem) {
        switch (voiceStatus) {
            case Cellular_Stat_Home:          state = A_REGISTRATION_HOME;          break;
            case Cellular_Stat_Roaming:       state = A_REGISTRATION_ROAMING;       break;
            case Cellular_Stat_Searching:     state = A_REGISTRATION_SEARCHING;     break;
            case Cellular_Stat_Denied:        state = A_REGISTRATION_DENIED;        break;
            case Cellular_Stat_Unregistered:  state = A_REGISTRATION_UNREGISTERED;  break;
        }

        amodem_set_voice_registration(android_modem, state);
    }
}

static void cellular_setMeterStatus(enum CellularMeterStatus meterStatus)
{
    int meteron = -1;

    if (android_modem) {
        switch (meterStatus) {
            case Cellular_Metered:                  meteron = 1;                    break;
            case Cellular_Temporarily_Not_Metered:  meteron = 0;                    break;
            default: return;
        }
        amodem_set_meter_state(android_modem, meteron);
    }
}

static void cellular_setDataStatus(enum CellularStatus dataStatus)
{
    // (See do_gsm_data() in android-qemu2-glue/console.c)
    ARegistrationState  state = A_REGISTRATION_UNKNOWN;

    switch (dataStatus) {
        case Cellular_Stat_Home:          state = A_REGISTRATION_HOME;          break;
        case Cellular_Stat_Roaming:       state = A_REGISTRATION_ROAMING;       break;
        case Cellular_Stat_Searching:     state = A_REGISTRATION_SEARCHING;     break;
        case Cellular_Stat_Denied:        state = A_REGISTRATION_DENIED;        break;
        case Cellular_Stat_Unregistered:  state = A_REGISTRATION_UNREGISTERED;  break;
    }

    if (android_modem) {
        amodem_set_data_registration(android_modem, state);
    }

    android_net_disable = (state != A_REGISTRATION_HOME    &&
                        state != A_REGISTRATION_ROAMING );
}

static void cellular_setStandard(enum CellularStandard cStandard)
{
    // (See do_network_speed() in android-qemu2-glue/console.c)
    char *speedName;

    switch (cStandard) {
        case Cellular_Std_GSM:    speedName = "gsm";    break;
        case Cellular_Std_HSCSD:  speedName = "hscsd";  break;
        case Cellular_Std_GPRS:   speedName = "gprs";   break;
        case Cellular_Std_EDGE:   speedName = "edge";   break;
        case Cellular_Std_UMTS:   speedName = "umts";   break;
        case Cellular_Std_HSDPA:  speedName = "hsdpa";  break;
        case Cellular_Std_LTE:    speedName = "lte";    break;
        case Cellular_Std_5G:     speedName = "5g";     break;
        case Cellular_Std_full:   speedName = "full";   break;

        default:
            return; // Error
    }

    // Find this entry in the speed table and set
    // android_net_download_speed and android_net_upload_speed
    if (!android_network_set_speed(speedName)) {
        return;
    }

    // Tell the network shaper the new rates
    netshaper_set_rate(android_net_shaper_in,  android_net_download_speed);
    netshaper_set_rate(android_net_shaper_out, android_net_upload_speed);

    if (android_modem) {
        // Tell the guest about the new network type.
        amodem_set_data_network_type(android_modem,
                                     android_parse_network_type(speedName));
    }
}

static const QAndroidCellularAgent sQAndroidCellularAgent = {
    .setSignalStrength = cellular_setSignalStrength,
    .setSignalStrengthProfile = cellular_setSignalStrengthProfile,
    .setVoiceStatus = cellular_setVoiceStatus,
    .setMeterStatus = cellular_setMeterStatus,
    .setDataStatus = cellular_setDataStatus,
    .setStandard = cellular_setStandard,
    .setSimPresent = cellular_setSimPresent};

const QAndroidCellularAgent* const gQAndroidCellularAgent =
        &sQAndroidCellularAgent;
