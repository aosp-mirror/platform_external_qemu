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

#include "android/meminfo.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#define DEBUG 1

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

enum class QemuMemInfoType {
    CMD_MEMINFO = 0,
};

struct QemuMemInfo {
    QemudService* service;
    QemudClient* client;  // represents the qemud client on guest side
    meminfo_callback_t callback;
};

static QemuMemInfo _memInfo[1] = {};

static void _memInfoClient_recv(void* opaque,
                                uint8_t* msg,
                                int msglen,
                                QemudClient* client) {
    D("%s: msg length %d", __func__, msglen);
    QemuMemInfo* meminfo = (QemuMemInfo*)opaque;

    if (meminfo->callback != nullptr) {
        meminfo->callback((const char*)msg, msglen);
    }
}

static void _memInfoClient_close(void* opaque) {
    // TODO: handle close
}

static void _memInfoClient_save(Stream* f, QemudClient* client, void* opaque) {
    // TODO
}

static int _memInfoClient_load(Stream* f, QemudClient* client, void* opaque) {
    // TODO
    return 0;
}

static void _memInfo_save(Stream* f, QemudService* sv, void* opaque) {
    // TODO
}

static int _memInfo_load(Stream* f, QemudService* s, void* opaque) {
    // TODO
    return 0;
}

static QemudClient* _memInfo_connect(void* opaque,
                                     QemudService* service,
                                     int channel,
                                     const char* client_param) {
    D("Qemu Meminfo client connected");
    QemuMemInfo* meminfo = (QemuMemInfo*)opaque;
    // only allow one qemu-meminfo client
    if (meminfo->client != nullptr) {
        qemud_client_close(meminfo->client);
    }
    QemudClient* client = qemud_client_new(
            service, channel, client_param, opaque, _memInfoClient_recv,
            _memInfoClient_close, _memInfoClient_save, _memInfoClient_load);
    qemud_client_set_framing(client, 1);
    meminfo->client = client;
    return client;
}

void android_qemu_meminfo_init(void) {
    QemuMemInfo* meminfo = _memInfo;
    if (meminfo->service == nullptr) {
        meminfo->service = qemud_service_register("qemu-meminfo", 1, meminfo,
                                                  _memInfo_connect,
                                                  _memInfo_save, _memInfo_load);
        D("%s: qemu-meminfo qemud service initialized", __func__);
    } else {
        D("%s qemu-meminfo qemud service already initialized", __func__);
    }
}

bool android_query_meminfo(meminfo_callback_t callback) {
    QemuMemInfo* memInfo = _memInfo;
    if (memInfo->service == nullptr || memInfo->client == nullptr) {
        D("pipe service is not initialized");
        return false;
    }
    memInfo->callback = callback;
    char cmd = (char)QemuMemInfoType::CMD_MEMINFO;
    qemud_client_send(memInfo->client, (const uint8_t*)&cmd, 1);
    return true;
}
