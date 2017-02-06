#include "android/car.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"

#include "android/utils/debug.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include "android/emulation/proto/VehicleHalProto.pb.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <functional>

#define  D(...)  VERBOSE_PRINT(car,__VA_ARGS__)

#define  DEBUG_TRANSPORT  0

#if DEBUG_TRANSPORT
#define  T(...)  VERBOSE_PRINT(car,__VA_ARGS__)
#else
#define  T(...)   ((void)0)
#endif

using namespace emulator;

typedef struct {
    QemudService* service;
    QemudClient* client;
} HwCar;

static HwCar _car[1] = {};

typedef struct {
    car_callback_t callback;
    void* context;
} CarDataCallback;

static CarDataCallback* _car_callback = NULL;

static void _hwCarClient_recv(void* opaque, uint8_t* msg, int msglen, QemudClient* car) {
    T("%s: msg length %d", __FUNCTION__, msglen);
    if (_car_callback != NULL) {
        _car_callback->callback((char*) msg, msglen, _car_callback->context);
    }
}

static void _hwCarClient_close(void* opaque) {
	// TODO: handle close
}

/* Saves client data to snapshot */
static void _hwCarClient_save(Stream* f, QemudClient* client, void* opaque) {
    // TODO
}

/* Loads client data from snapshot */
static int _hwCarClient_load(Stream* f, QemudClient* client, void* opaque) {
    // TODO
    return 0;
}

static void _hwCar_save(Stream* f, QemudService* sv, void* opaque) {
    // TODO
}

static int _hwCar_load(Stream* f, QemudService* s, void* opaque) {
  // TODO
  return 0;
}

static QemudClient* _hwCar_connect(void* opaque,
        QemudService* service,
        int channel,
        const char* client_param) {
    D("Car client connected");
    HwCar* car = (HwCar *) opaque;
    // Allow only one client, usually it's just Vehical Hal
    if (car->client != NULL) {
        qemud_client_close(car->client);
    }
    QemudClient* client = qemud_client_new(
            service, channel, client_param, opaque, _hwCarClient_recv,
            _hwCarClient_close, _hwCarClient_save, _hwCarClient_load);
    qemud_client_set_framing(client, 1);
    car->client = client;
    return client;
}

void android_car_init(void) {
    HwCar* car = _car;
    if (car->service == NULL) {
        car->service = qemud_service_register(
                "car", 0, car, _hwCar_connect, _hwCar_save, _hwCar_load);
        D("%s: car qemud service initialized", __FUNCTION__);
    } else {
        D("%s: car qemud service already initialized", __FUNCTION__);
    }
}

void set_car_call_back(car_callback_t callback, void* context) {
    if (_car_callback != NULL) {
        delete(_car_callback);
    }

    _car_callback = new CarDataCallback();
    _car_callback->callback = callback;
    _car_callback->context = context;
}

void android_send_car_data(const char* msg, int msgLen)
{
    if (_car->service == NULL) {
        D("pipe service is not initialized");
        return;
    }

    if (_car->client == NULL) {
        D("no car client is connected");
        return;
    }
    T("%s: Sent message length of %d", __FUNCTION__, );
    qemud_client_send(_car->client, (const uint8_t*)msg, msgLen);
}
