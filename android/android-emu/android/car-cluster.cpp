/* Copyright (C) 2018 The Android Open Source Project
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
#include "android/car-cluster.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

#define DEBUG_TRANSPORT 0

#if DEBUG_TRANSPORT
#define T(...) VERBOSE_PRINT(car, __VA_ARGS__)
#else
#define T(...) ((void)0)
#endif


struct HwCarCluster {
    QemudService* service;
    QemudClient* client;
    car_cluster_callback_t callback;
    unsigned int nextSize;
};

static HwCarCluster _carCluster[1] = {};

static void _hwCarClusterClient_recv(void* opaque,
                              uint8_t* msg,
                              int msglen,
                              QemudClient* carCluster) {
    T("%s: msg length %d", __func__, msglen);
    HwCarCluster* hwCarCluster = (HwCarCluster*) opaque;

    // The sender always alternates between a size, and then the corresponding
    // frame of that size. This is because when the android sender sends
    // some buffer, the corresponding msglen is the size of the whole buffer,
    // rather than the actual size of the data, so this is a workaround.
    if (hwCarCluster->nextSize == UINT_MAX) {
        if (msglen != 4) {
            D("%s: size not sent prior to data", __func__);
            return;
        }
        hwCarCluster->nextSize = ((int32_t*) msg)[0];
    } else if (hwCarCluster->callback != nullptr) {
        hwCarCluster->callback(msg, hwCarCluster->nextSize);
        hwCarCluster->nextSize = UINT_MAX;
    }
}

static void _hwCarClusterClient_close(void* opaque) {
    auto cl = static_cast<HwCarCluster*>(opaque);
    if (cl->client != nullptr) {
        qemud_client_close(cl->client);
        cl->client = NULL;
    }
}

static QemudClient* _hwCarCluster_connect(void* opaque,
                                   QemudService* service,
                                   int channel,
                                   const char* client_param) {
    D("CarCluster client connected");
    HwCarCluster* carCluster = (HwCarCluster*) opaque;
    // Allow only one client -- Vehicle Hal
    if (carCluster->client != nullptr) {
        qemud_client_close(carCluster->client);
    }

    carCluster->client = qemud_client_new(
            service, channel, client_param, opaque, _hwCarClusterClient_recv,
            _hwCarClusterClient_close, NULL, NULL);
    return carCluster->client;
}

void android_car_cluster_init(void) {
    HwCarCluster* carCluster = _carCluster;
    if (carCluster->service == nullptr) {
        // Car Cluster service have nothing to save and load
        // The callback won't change
        carCluster->service = qemud_service_register(
                "carCluster", 0, carCluster, _hwCarCluster_connect, NULL, NULL);
        D("%s: carCluster qemud service initialized", __func__);
    } else {
        D("%s: carCluster qemud service already initialized", __func__);
    }
    carCluster->nextSize = UINT_MAX;
}

void set_car_cluster_call_back(car_cluster_callback_t func) {
    HwCarCluster* carCluster = _carCluster;
    if (func == nullptr) {
        D("%s: callback is invalid", __func__);
        return;
    }
    if (carCluster->callback != nullptr) {
        D("%s: callback has been set before, overwrite it for now", __func__);
    }
    carCluster->callback = func;
}

void android_send_car_cluster_data(const uint8_t* msg, int msgLen) {
    if (_carCluster->service == nullptr) {
        D("pipe service is not initialized");
        return;
    }

    if (_carCluster->client == nullptr) {
        D("no car cluster client is connected");
        return;
    }
    T("%s: Sent message length of %d", __func__, );
    qemud_client_send(_carCluster->client, msg, msgLen);
}
