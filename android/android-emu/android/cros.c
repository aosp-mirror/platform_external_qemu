/* Copyright (C) 2017 The Android Open Source Project
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

#include "android/cros.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/hw-qemud.h"
#include "android/utils/debug.h"

#include <stdlib.h>
#include <string.h>

static QemudClient*  sClient;

#define  D(...) VERBOSE_PRINT(events,__VA_ARGS__)

void cros_send_message(const char*  message) {
    if (message == NULL)
        return;

    D("sending '%s'", message);

    if (sClient == NULL) {
        D("no client, ignored");
        return;
    }

    qemud_client_send(sClient, (const uint8_t*) message, strlen(message));
}

static void cros_close(void* opaque) {
    sClient = NULL;
}

static QemudClient* cros_connect(void*  opaque,
                                 QemudService* service,
                                 int channel,
                                 const char* client_param) {
    sClient = qemud_client_new(service, channel, client_param,
                               opaque, NULL, cros_close, NULL, NULL);
    if (sClient) {
        qemud_client_set_framing(sClient, 1);
        if (android_hw->hw_arc_autologin)
            cros_send_message("autologin=1");
    }
    return sClient;
}

int cros_pipe_init() {
    static QemudService* service;
    if (service) {
        D("Already registered, ignored");
        return 0;
    }
    service = qemud_service_register(ANDROID_QEMUD_CROS, 1, NULL,
                                     cros_connect, NULL, NULL);
    if (service == NULL)
        return -1;
    return 0;
}
