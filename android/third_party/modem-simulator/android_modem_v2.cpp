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

void amodem_receive_sms_vx( AModem  modem, SmsPDU  sms ) {
    if (android_modem_version == 1) {
        amodem_receive_sms(modem, sms);
    } else if (android_modem_version == 2) {
         cvd::send_sms_msg(std::string(amodem_sms_to_string(modem, sms)));

    }
}
