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
    HwFingerprintClient*    fp_clients;
} HwFingerprint;

struct HwFingerprintClient {
    HwFingerprintClient*    next;
    HwFingerprint*          fp;
    QemudClient*            qemu_client;
};

static QemudClient* _hwFingerprint_connect(void*  opaque,
                    QemudService*  service,
                    int  channel,
                    const char* client_param);

static void
_hwFingerprintClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client );

static void
_hwFingerprintClient_close(void* opaque);

static HwFingerprintClient*
_hwFingerprintClient_new( HwFingerprint*  fp);

/* the only static variable */
static HwFingerprint _fingerprintState[1];

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
}

void
android_hw_fingerprint_send(const char* msg, int msglen)
{
    HwFingerprint*  fp = _fingerprintState;
    if (fp && fp->fp_clients) {
        qemud_client_send(fp->fp_clients->qemu_client, msg, msglen);
    }
}

/*********************************************************************************** 
   
            All the static methods
 
***********************************************************************************/


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

static void
_hwFingerprintClient_recv(void* opaque, uint8_t* msg, int msglen,
                      QemudClient*  client )
{
    HwFingerprintClient*       fp_client = opaque;
    msg[msglen-1]='\0';
    fprintf(stderr, "got message %s\n", msg);
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
    AFREE(fp_client);
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

