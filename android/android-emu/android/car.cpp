#include "android/car.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"

#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"
#include "android/utils/looper.h"

#include "android/emulation/proto/VehicleHalProto.pb.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <functional>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)
#define V(...) VERBOSE_PRINT(init, __VA_ARGS__)

using namespace emulator;
typedef struct HwCarClient HwCarClient;

typedef struct {
    QemudService* service;
    HwCarClient* client;
} HwCar;

struct HwCarClient {
    HwCar* car;
    QemudClient* client;
};

static HwCar _car[1] = {};

std::function<void(const char*)> _callback;

static HwCarClient* _hwCarClient_new(HwCar* car) {
    HwCarClient* cl;

    cl = new HwCarClient();

    cl->car = car;
    car->client = cl;

    return cl;
}

static void _hwCarClient_recv(void* opaque,
                                 uint8_t* msg,
                                 int msglen,
                                 QemudClient* client) {
     D(" Yao: %s: msg length: %d  '%s'", __FUNCTION__, msglen, msg);
    // if (_callback != NULL) {
//        _callback((char *)msg);
 //    }
    if (_callback != NULL) {
        _callback((char*) msg);
    }
    android_send_car_data((char*) msg);
}


static void _hwCarClient_close(void* opaque) {
	D("Yao: close!!!");
}

/* Saves sensor-specific client data to snapshot */
static void _hwCarClient_save(Stream* f, QemudClient* client, void* opaque) {
//    HwCarClient* sc = opaque;

//    stream_put_be32(f, sc->delay_ms);
//    stream_put_be32(f, sc->enabledMask);
//    stream_put_timer(f, sc->timer);
}

/* Loads sensor-specific client data from snapshot */
static int _hwCarClient_load(Stream* f, QemudClient* client, void* opaque) {
//    HwCarClient* sc = opaque;

 //   sc->delay_ms = stream_get_be32(f);
//    sc->enabledMask = stream_get_be32(f);
//    stream_get_timer(f, sc->timer);

    return 0;
}

static QemudClient* _hwCar_connect(void* opaque,
                                       QemudService* service,
                                       int channel,
                                       const char* client_param) {
    HwCar* car = (HwCar *) opaque;
    HwCarClient* cl = _hwCarClient_new(car);
    QemudClient* client = qemud_client_new(
            service, channel, client_param, cl, _hwCarClient_recv,
            _hwCarClient_close, _hwCarClient_save, _hwCarClient_load);
    qemud_client_set_framing(client, 1);
    cl->client = client;
    D("Yao connected");
    return client;
}

static void _hwCar_save(Stream* f, QemudService* sv, void* opaque) {
  D("Yao _hwCar_save");
}

static int _hwCar_load(Stream* f, QemudService* s, void* opaque) {
  D("Yao _hwCar_load");
  return 0;
}

void android_car_init(void) {
  HwCar* car = _car;
  if (car->service == NULL) {
    car->service = qemud_service_register("car", 0, car, _hwCar_connect,
			   _hwCar_save, _hwCar_load);
	D("%s: car qemud service initialized", __FUNCTION__);
  } else {
    D("Yao: already initilized");
  }
}



void set_call_back(const std::function<void(const char*)>& callback) {
    _callback = callback;

    D("Yao: setcallback %s");
}


void android_send_car_data(const char* msg)
{
    if (_car->service == NULL) {
        D("Yao: pipe service NULL");
        return;
    }

    if (_car->client == NULL) {
     D("Yao: pipe client  NULL");
     return;
     }
//    EmulatorMessage* emulatorMsg = new EmulatorMessage();

//    VehiclePropConfig *cfg = emulatorMsg->add_config();
//    cfg->set_prop(1);
//    emulatorMsg->set_status(RESULT_OK);
//    emulatorMsg->set_msg_type(GET_CONFIG_RESP);


//    string msgString;
//    D("Byte size %d", emulatorMsg->ByteSize());
//    if (emulatorMsg->SerializeToString(&msgString)) {
//        int msgLen = msgString.length();
//        D("string length %d ", msgLen);
        // TODO:  Prepend the message length to the string without a copy
       //msgString.insert(0, reinterpret_cast<char*>(&msgLen), 4);
//    } else {
//        D("serialize to string failed");
//    }

//    EmulatorMessage* receivedMsg = new EmulatorMessage();
//    receivedMsg->ParseFromString(msgString);


//    D("received msg type: %d ", receivedMsg->msg_type());

//    qemud_client_send(_car->client->client, (const uint8_t*)msgString.c_str(), msgString.length());
    qemud_client_send(_car->client->client, (const uint8_t*)msg, strlen(msg));

   D("Yao: android_send_car_data %s %d", msg, strlen(msg));
}


