/* Copyright 2016 The Android Open Source Project
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

#include "android/hw-nfc.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  E(...)  derror(__VA_ARGS__)
#define  W(...)  dwarning(__VA_ARGS__)
#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#define  V(...)  VERBOSE_PRINT(init,__VA_ARGS__)

#define NFC_LISTEN_SERVICE_NAME "nfclisten"

/***********************************************************************************

            All the declarations

***********************************************************************************/

typedef struct HwNfcClient   HwNfcClient;

typedef struct {
    QemudService*           qemu_listen_service;
    HwNfcClient*            nfc_clients;
    uint8_t*                p_request_data;
    uint64_t                request_length;
    uint8_t*                p_response_data;
    uint64_t                response_length;
} HwNfcService;

struct HwNfcClient {
    HwNfcClient*            next;
    HwNfcService*           nfc;
    QemudClient*            qemu_client;
};

static void _hwNfcClient_recv(void* opaque, uint8_t* msg,
                              int msglen, QemudClient*  client );

static void _hwNfcClient_close(void* opaque);

static HwNfcClient* _hwNfcClient_new(HwNfcService*  nfc);

/* the only static variable */
static HwNfcService _nfcState[1];

static void _hwNfc_send(void);

static void _hwNfc_reset_request(void);
static void _hwNfc_reset_response(void);

static QemudClient* _hwNfc_connect(void*  opaque,
                                   QemudService*  service,
                                   int  channel,
                                   const char* client_param);

static void _hwNfcClient_removeFromList(HwNfcClient** phead,
                                        HwNfcClient* target);

static void _hwNfc_save(Stream* f, QemudService* sv, void* opaque);

static int _hwNfc_load(Stream* f, QemudService* sv, void* opaque);

/***********************************************************************************

            All the public methods

***********************************************************************************/
void android_hw_nfc_init(void)
{
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcService*  nfc = _nfcState;

    if (nfc->qemu_listen_service == NULL) {
        nfc->qemu_listen_service = qemud_service_register(NFC_LISTEN_SERVICE_NAME, 0, nfc,
                                                          _hwNfc_connect,
                                                          _hwNfc_save,
                                                          _hwNfc_load);
        D("%s: nfc qemud listen service initialized\n", __FUNCTION__);
    }
}

void android_hw_nfc_send_message(uint8_t* data, uint64_t msg_len)
{
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcService*  nfc = _nfcState;
    D("sending message to nfc %s\n", data);
    if (nfc->request_length > 0) {
        free(nfc->p_request_data);
        nfc->p_request_data = NULL;
    }
    nfc->p_request_data = (uint8_t*)calloc(msg_len, sizeof(uint8_t));
    memcpy(nfc->p_request_data, data, msg_len);

    nfc->request_length = msg_len;
    _hwNfc_send();
}

/***********************************************************************************

            All the static methods

***********************************************************************************/

static void _hwNfc_reset_request(void) {
    D("HW-NFC: %s", __FUNCTION__);
    if (nfc->request_length > 0 && nfc->p_request_data) {
        free(nfc->p_request_data);
        nfc->p_request_data = NULL;
        nfc->request_length = 0;
    }
}

static void _hwNfc_reset_response(void) {
    D("HW-NFC: %s", __FUNCTION__);
    if (nfc->response_length > 0 && nfc->p_response_data) {
        free(nfc->p_response_data);
        nfc->p_response_data = NULL;
        nfc->response_length = 0;
    }
}

static void _hwNfc_save(Stream* f, QemudService* sv, void* opaque) {
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcService* nfc = opaque;
    stream_putc(f, nfc->response_length);
    stream_write(f, nfc->p_response_data, nfc->response_length);
}

static int _hwNfc_load(Stream* f, QemudService* sv, void* opaque) {
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcService* nfc = opaque;
    if (nfc->request_length > 0) {
        _hwNfc_reset_request();
    }

    size_t sz = stream_getc(f);
    nfc->p_request_data = (uint8_t *)calloc(sz, sizeof(uint8_t));
    stream_read(f, nfc->p_request_data, sz);
    nfc->request_length = sz;
    return 0;
}

static void _hwNfc_send()
{
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcService*  nfc = _nfcState;
    HwNfcClient*   nfc_client = nfc->nfc_clients;
    if (!nfc_client)
        return;

    qemud_client_send(nfc_client->qemu_client, nfc->p_request_data, nfc->request_length);
    _hwNfc_reset();
}

static QemudClient* _hwNfc_connect(void*  opaque,
                                   QemudService*  service,
                                   int  channel,
                                   const char* client_param)
{
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcService*  nfc = opaque;
    HwNfcClient*   nfc_client = _hwNfcClient_new(nfc);
    QemudClient*   client  = qemud_client_new(service, channel, client_param, nfc_client,
                                              _hwNfcClient_recv,
                                              _hwNfcClient_close,
                                              NULL, /* no save */
                                              NULL /* no load */ );
    qemud_client_set_framing(client, 1);
    nfc_client->qemu_client = client;

    D("%s: connect nfc listen is called\n", __FUNCTION__);
    return client;
}

static void _hwNfcClient_recv(void* opaque, uint8_t* msg, int msglen,
                              QemudClient*  client )
{
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcService*  nfc = opaque;
    if (nfc->response_length > 0) {
        _hwNfc_reset();
    }

    nfc->p_response_data = (uint8_t*)calloc(msglen, sizeof(uint8_t));
    memcpy(nfc->p_response_data, msg, msglen);
    nfc->response_length = msglen;
}

static HwNfcClient* _hwNfcClient_new( HwNfcService*  nfc)
{
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcClient*  nfc_client;
    ANEW0(nfc_client);
    nfc_client->nfc = nfc;
    nfc_client->next = nfc->nfc_clients;
    nfc->nfc_clients = nfc_client;
    return nfc_client;
}

static void _hwNfcClient_close(void* opaque)
{
    D("HW-NFC: %s", __FUNCTION__);
    HwNfcClient* nfc_client = opaque;
    if (nfc_client->nfc) {
        HwNfcClient** pnode = &nfc_client->nfc->nfc_clients;
        _hwNfcClient_removeFromList(pnode, nfc_client);
        nfc_client->next = NULL;
        nfc_client->nfc = NULL;
    }
    AFREE(nfc_client);
}

static void _hwNfcClient_removeFromList(HwNfcClient** phead, HwNfcClient* target)
{
    D("HW-NFC: %s", __FUNCTION__);
    for (;;) {
        HwNfcClient* node = *phead;
        if (node == NULL)
            break;
        if (node == target) {
            *phead = target->next;
            break;
        }
        phead = &node->next;
    }
}
