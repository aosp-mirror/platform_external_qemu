/* Copyright (C) 2007-2008 The Android Open Source Project
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
#pragma once

#include "android/utils/compiler.h"
#include "android/utils/stream.h"

ANDROID_BEGIN_HEADER

/* Support for the qemud-based 'services' in the emulator.
 * Please read docs/ANDROID-QEMUD.TXT to understand what this is about.
 */

/* initialize the qemud support code in the emulator
 */

extern void android_qemud_init(void);

/* A QemudService service is used to connect one or more clients to
 * a given emulator facility. Only one client can be connected at any
 * given time, but the connection can be closed periodically.
 */

typedef struct QemudClient QemudClient;
typedef struct QemudService QemudService;

/* A function that will be called when the client running in the emulated
 * system has closed its connection to qemud.
 */
typedef void (*QemudClientClose)(void* opaque);

/* A function that will be called when the client sends a message to the
 * service through qemud.
 */
typedef void (*QemudClientRecv)(void* opaque,
                                uint8_t* msg,
                                int msglen,
                                QemudClient* client);

/* A function that will be called when the state of the client should be
 * saved to a snapshot.
 */
typedef void (*QemudClientSave)(Stream* f, QemudClient* client, void* opaque);

/* A function that will be called when the state of the client should be
 * restored from a snapshot.
 */
typedef int (*QemudClientLoad)(Stream* f, QemudClient* client, void* opaque);

/* Register a new client for a given service.
 * 'clie_opaque' will be sent as the first argument to 'clie_recv' and
 * 'clie_close'
 * 'clie_recv' and 'clie_close' are both optional and may be NULL.
 *
 * You should typically use this function within a QemudServiceConnect callback
 * (see below).
 */
extern QemudClient* qemud_client_new(QemudService* service,
                                     int channel_id,
                                     const char* client_param,
                                     void* clie_opaque,
                                     QemudClientRecv clie_recv,
                                     QemudClientClose clie_close,
                                     QemudClientSave clie_save,
                                     QemudClientLoad clie_load);

/* Enable framing on a given client channel.
 */
extern void qemud_client_set_framing(QemudClient* client, int enabled);

/* Send a message to a given qemud client
 */
extern void qemud_client_send(QemudClient* client,
                              const uint8_t* msg,
                              int msglen);

/* Force-close the connection to a given qemud client.
 */
extern void qemud_client_close(QemudClient* client);

/* A function that will be called each time a new client in the emulated
 * system tries to connect to a given qemud service. This should typically
 * call qemud_client_new() to register a new client.
 */
typedef QemudClient* (*QemudServiceConnect)(void* opaque,
                                            QemudService* service,
                                            int channel,
                                            const char* client_param);

/* A function that will be called when the state of the service should be
 * saved to a snapshot.
 */
typedef void (*QemudServiceSave)(Stream* f,
                                 QemudService* service,
                                 void* opaque);

/* A function that will be called when the state of the service should be
 * restored from a snapshot.
 */
typedef int (*QemudServiceLoad)(Stream* f, QemudService* service, void* opaque);

/* Register a new qemud service.
 * 'serv_opaque' is the first parameter to 'serv_connect'
 */
extern QemudService* qemud_service_register(const char* serviceName,
                                            int max_clients,
                                            void* serv_opaque,
                                            QemudServiceConnect serv_connect,
                                            QemudServiceSave serv_save,
                                            QemudServiceLoad serv_load);

/* Sends a message to all clients of a given service.
 */
extern void qemud_service_broadcast(QemudService* sv,
                                    const uint8_t* msg,
                                    int msglen);

ANDROID_END_HEADER
