/* Copyright (C) 2010 The Android Open Source Project
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

#ifndef QEMU_ANDROID_CONSOLE_CLIENT_H
#define QEMU_ANDROID_CONSOLE_CLIENT_H

// Opaque AndroidConsoleClient structure.
typedef struct AndroidConsoleClient AndroidConsoleClient;

// Base console port
#define CORE_BASE_PORT          5554

// Maximum number of core porocesses running simultaneously on a machine.
#define MAX_CORE_PROCS          16

// Socket timeout in millisec (set to half a second)
#define CORE_PORT_TIMEOUT_MS    500

/* Opens core console socket.
 * Param:
 *  sockaddr Socket address to the core console.
 * Return:
 *  Opened socket handle on success, or -1 on failure, with errno appropriately
 *  set.
 */
int android_console_open(SockAddress* sockaddr);

/* Creates descriptor for a console client.
 * Param:
 *  console_socket Socket address for the console.
 * Return:
 *  Allocated and initialized descriptor for the client on success, or NULL
 *  on failure. Use android_console_client_connect to connect to the console.
 */
AndroidConsoleClient* android_console_client_create(SockAddress* console_socket);

/* Frees descriptor allocated with android_console_client_create.
 * Param:
 *  desc Descriptor to free. Note that this routine will simply free the memory
 *      used by the descriptor.
 */
void android_console_client_free(AndroidConsoleClient* desc);

/* Opens a socket handle to the console.
 * Param:
 *  desc Console client descriptor. Note that if the descriptor has been already
 *      opened, this routine will simply return with success.
 * Return:
 *  0 on success, or -1 on failure with errno properly set.
 */
int android_console_client_open(AndroidConsoleClient* desc);

/* Closes a socket handle to the console opened with android_console_client_open.
 * Param:
 *  desc Console client descriptor opened with android_console_client_open.
 */
void android_console_client_close(AndroidConsoleClient* desc);

/* Synchronously writes to the console.
 * Param:
 *  desc Console client descriptor opened with android_console_client_open.
 *      buffer Buffer to write.
 *  to_write Number of bytes to write.
 *  written_bytes Upon success, contains number of bytes written. This parameter
 *      is optional, and can be NULL.
 * Return:
 *  0 on success, or -1 on failure.
 */
int android_console_client_write(AndroidConsoleClient* desc,
                                 const void* buffer,
                                 int to_write,
                                 int* written_bytes);

/* Synchronously reads from the console.
 * Param:
 *  desc Console client descriptor opened with android_console_client_open.
 *  buffer Buffer to read data to.
 *  to_read Number of bytes to read.
 *  read_bytes Upon success, contains number of bytes that have been actually
 *    read. This parameter is optional, and can be NULL.
 * Return:
 *  0 on success, or -1 on failure.
 */
int android_console_client_read(AndroidConsoleClient* desc,
                                void* buffer,
                                int to_read,
                                int* read_bytes);

/* Switches opened console client to a given stream.
 * Param:
 *  desc Console client descriptor opened with android_console_client_open. Note
 *      that this descriptor should represent console itself. In other words,
 *      there must have been no previous calls to this routine for that
 *      descriptor.
 *  stream_name Name of the stream to switch to.
 *  handshake Address of a string to allocate for a handshake message on
 *      success, or an error message on failure. If upon return from this
 *      routine that string is not NULL, its buffer must be freed with 'free'.
 * Return:
 *  0 on success, or -1 on failure.
 */
int android_console_client_switch_stream(AndroidConsoleClient* desc,
                                         const char* stream_name,
                                         char** handshake);

#endif  // QEMU_ANDROID_CONSOLE_CLIENT_H
