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

#include <math.h>
#include "android/hw-fingerprint.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#include "android/hw-qemud.h"
#include "android/globals.h"
#include "hw/hw.h"
#include "sysemu/char.h"
#include "qemu/timer.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#define  V(...)  VERBOSE_PRINT(init,__VA_ARGS__)

/*********************************************************************************** 
   
            All the declarations
 
***********************************************************************************/

typedef struct HwFingerprintClient   HwFingerprintClient;

typedef struct {
    QemudService*           qemu_service;
    QemudService*           qemu_listen_service;
    HwFingerprintClient*    fp_clients;
    HwFingerprintClient*    fp_sender;
    int     fingerid;
    int     finger_is_on_sensor;
} HwFingerprint;

struct HwFingerprintClient {
    HwFingerprintClient*    next;
    HwFingerprint*          fp;
    QemudClient*            qemu_client;
    QEMUTimer*      timer;
    uint32_t    enabledMask;
    int32_t     delay_ms;
};

static QemudClient* _hwFingerprint_connect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param);

static void
_hwFingerprintClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client );

static void
_hwFingerprintlistenClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client );

static void
_hwFingerprintClient_close(void* opaque);

static HwFingerprintClient*
_hwFingerprintClient_new( HwFingerprint*  fp);

/* the only static variable */
static HwFingerprint _fingerprintState[1];

void _hwFingerprintlisten_send();

static QemudClient*
_hwFingerprintlisten_connect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param);
static void
_hwFingerprintClient_removeFromList(HwFingerprintClient** phead,
        HwFingerprintClient* target);
/*********************************************************************************** 
   
            All the public methods
 
***********************************************************************************/

void
android_hw_fingerprint_init( void )
{
    HwFingerprint*  fp = _fingerprintState;

    if (fp->qemu_service == NULL) {
        fp->qemu_service = qemud_service_register("fingerprint", 0, fp, _hwFingerprint_connect,
                                        NULL, /* no save */
                                        NULL /* no load */ );
        fprintf(stderr, "%s: fingerprint qemud service initialized\n", __FUNCTION__);
    }

    if (fp->qemu_listen_service == NULL) {
        fp->qemu_listen_service = qemud_service_register("fingerprintlisten", 0, fp, _hwFingerprintlisten_connect,
                                        NULL, /* no save */
                                        NULL /* no load */ );
        fprintf(stderr, "%s: fingerprint qemud listen service initialized\n", __FUNCTION__);
    }
}

void
android_hw_fingerprint_touch(int fingerid)
{
    HwFingerprint*  fp = _fingerprintState;
    fprintf(stderr, "touched fingerprint sensor with finger id %d\n", fingerid);
    fp->fingerid = fingerid;
    fp->finger_is_on_sensor = 1;
    /* send it right away */
    _hwFingerprintlisten_send();
}

void
android_hw_fingerprint_off()
{
    HwFingerprint*  fp = _fingerprintState;
    fprintf(stderr, "finger off the fingerprint sensor\n");
    fp->finger_is_on_sensor = 0;
    /* send it right away */
    _hwFingerprintlisten_send();
}

/*********************************************************************************** 
   
            All the static methods
 
***********************************************************************************/

void _hwFingerprintlisten_send()
{
    /* send back the finger id that is on the finger print sensor */
    HwFingerprint*  fp = _fingerprintState;
    HwFingerprintClient*    fp_client = fp->fp_sender;
    if (!fp_client)
        return;

    char buffer[128];
    if (fp->finger_is_on_sensor) {
        snprintf(buffer, sizeof buffer, "on:%d", fp_client->fp->fingerid);
    } else {
        snprintf(buffer, sizeof buffer, "off");
    }
    qemud_client_send(fp_client->qemu_client, (const uint8_t*)buffer, strlen(buffer)+1);
}

static QemudClient*
_hwFingerprint_connect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param)
{
    HwFingerprint*       fp = opaque;
    HwFingerprintClient*       fp_client = _hwFingerprintClient_new(fp);
    QemudClient*     client  = qemud_client_new(service, channel, client_param, fp_client,
                                                _hwFingerprintClient_recv,
                                                _hwFingerprintClient_close,
                                                NULL, /* no save */
                                                NULL /* no load */ );
    qemud_client_set_framing(client, 1);
    fp_client->qemu_client = client;

    fprintf(stderr, "%s: connect finger print is called\n", __FUNCTION__);
    return client;
}


/* this function is called periodically to send fingerprint
 * to the HAL module, and re-arm the timer if necessary
TODO: need to re-consider if we really need a timer: why not just send it
right away?
 */
static void
_hwFingerprintClient_tick( void*  opaque )
{
    HwFingerprintClient*  fp_client = opaque;
    int64_t          delay = fp_client->delay_ms;
    int64_t          now_ns;
    uint32_t         mask  = fp_client->enabledMask;
    char             buffer[128];

    now_ns = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    snprintf(buffer, sizeof buffer, "sync:%" PRId64, now_ns/1000);

    /* rearm timer, use a minimum delay of 20 ms, just to
     * be safe.
     */
    if (mask == 0)
        return;

    if (delay < 20)
        delay = 20;

    delay *= 1000000LL;  /* convert to nanoseconds */
    timer_mod(fp_client->timer, now_ns + delay);
}


static QemudClient*
_hwFingerprintlisten_connect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param)
{
    HwFingerprint*       fp = opaque;
    HwFingerprintClient*       fp_client = _hwFingerprintClient_new(fp);
    fp->fp_sender = fp_client;
    fp_client->timer = timer_new(QEMU_CLOCK_VIRTUAL, SCALE_NS, _hwFingerprintClient_tick, fp_client);
    QemudClient*     client  = qemud_client_new(service, channel, client_param, fp_client,
                                                _hwFingerprintlistenClient_recv,
                                                _hwFingerprintClient_close,
                                                NULL, /* no save */
                                                NULL /* no load */ );
    qemud_client_set_framing(client, 1);
    fp_client->qemu_client = client;

    fprintf(stderr, "%s: connect finger print listen is called\n", __FUNCTION__);
    return client;
}

static void
_hwFingerprintlistenClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client )
{
    HwFingerprintClient*       fp_client = opaque;
    msg[msglen-1]='\0';
    fprintf(stderr, "got message '%s'\n", msg);

    /* send back the finger id that is on the finger print sensor */
    char buffer[128];
    if (fp_client->fp->finger_is_on_sensor)
        snprintf(buffer, sizeof buffer, "ok:%d", fp_client->fp->fingerid);
    else
        snprintf(buffer, sizeof buffer, "ko:%d", -1);

    qemud_client_send(fp_client->qemu_client, (const uint8_t*)buffer, strlen(buffer)+1);

    //TODO: setup a timer
}

static void
_hwFingerprintClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client )
{
    HwFingerprintClient*       fp_client = opaque;
    msg[msglen-1]='\0';
    fprintf(stderr, "got message '%s'\n", msg);

    /* send back the finger id that is on the finger print sensor */
    char buffer[128];
    if (fp_client->fp->finger_is_on_sensor)
        snprintf(buffer, sizeof buffer, "ok:%d", fp_client->fp->fingerid);
    else
        snprintf(buffer, sizeof buffer, "ko:%d", -1);

    qemud_client_send(fp_client->qemu_client, (const uint8_t*)buffer, strlen(buffer)+1);
}

static void
_hwFingerprintClient_close(void* opaque)
{
    HwFingerprintClient*       fp_client = opaque;
    if (fp_client->fp) {
        HwFingerprintClient** pnode = &fp_client->fp->fp_clients;
        _hwFingerprintClient_removeFromList(pnode, fp_client);
        fp_client->next = NULL;
        fp_client->fp = NULL;
    }
    if (fp_client->timer) {
        timer_del(fp_client->timer);
        timer_free(fp_client->timer);
        fp_client->timer = NULL;
    }
    AFREE(fp_client);
}

static void
_hwFingerprintClient_removeFromList(HwFingerprintClient** phead, HwFingerprintClient* target)
{
    for (;;) {
        HwFingerprintClient* node = *phead;
        if (node == NULL)
            break;
        if (node == target) {
            *phead = target->next;
            break;
        }
        phead = &node->next;
    }
}

static HwFingerprintClient*
_hwFingerprintClient_new( HwFingerprint*  fp)
{
    HwFingerprintClient*  fp_client;

    ANEW0(fp_client);

    fp_client->fp = fp;

    fp_client->next         = fp->fp_clients;
    fp->fp_clients = fp_client;

    return fp_client;
}

