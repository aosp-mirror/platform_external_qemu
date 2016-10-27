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

#include "android-qemu1-glue/qemu-control-impl.h"

#include "android/emulation/control/nfc_agent.h"
#include "android/hw-nfc.h"

static void nfc_sendMessage(uint8_t* message, uint64_t msg_len)
{
    android_hw_nfc_send_message(message, msg_len);
}

static const QAndroidNfcAgent sQAndroidNfcAgent = {
    .sendMessage = nfc_sendMessage
};
const QAndroidNfcAgent* const gQAndroidNfcAgent = &sQAndroidNfcAgent;