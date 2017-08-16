/*
 * Copyright (C) 2014-2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <compiler.h>
#include <sys/types.h>

#include <trusty_ipc.h>
#include <interface/hwkey/hwkey.h>

__BEGIN_CDECLS

typedef handle_t hwkey_session_t;

/**
 * hwkey_open() - Opens a trusty hwkey session.
 *
 * Return: a hwkey_session_t > 0 on success, * or an error code < 0 on
 * failure.
 */
long hwkey_open(void);

/**
 * hwkey_get_keyslot_data() - Gets the keyslot data referenced by slot_id.
 * @session:    session handle retrieved from hwkey_open
 * @slot_id:    string identifier for the requested keyslot
 * @data:       buffer for retrieved data
 * @data_size:  pointer to allocated size of data buffer. Updated to actual retrieved size if
 *              different from allocated size.
 *
 * Fills *data with result if size is sufficient. If actual size is less than data_size,
 * data_size is updated with the * actual returned size.
 *
 * Return: NO_ERROR on success, error code less than 0 on error. Possible error codes include:
 * - ERR_NOT_VALID: if input is NULL
 * - ERR_IO: if there's an issue communicating with the server
 * - ERR_TOO_BIG: if keyslot does not fit in data buffer
 * - ERR_NOT_FOUND: if keyslot is not found
 */
long hwkey_get_keyslot_data(hwkey_session_t session, const char *slot_id, uint8_t *data,
                            uint32_t *data_size);

/**
 * hwkey_derive() - Derives a cryptographic key based on input values.
 * @session:        session handle retrieved from hwkey_open
 * @kdf_version:    key derivation function version. If simply latest is
 *                  desired, pass HWKEY_KDF_VERSION_BEST. Actual
 *                  KDF used is written to field as out param.
 * @src:            the input for the KDF (key-derivation function)
 * @dest:           The result buffer into which the derived key is written.
 *                  Must be at least *src_buf_len bytes.
 * @buf_size:       The size of src and dest.
 *
 * Return: NO_ERROR on success, error code less than 0 on error. Possible error codes include:
 * - ERR_NOT_VALID: if input is NULL or if kdf_version is not supported
 * - ERR_IO: if there's an issue communicating with the server
 * - ERR_BAD_LEN: if buf_size is not valid
 *
 */
long hwkey_derive(hwkey_session_t session, uint32_t *kdf_version, const uint8_t *src,
                  uint8_t *dest, uint32_t buf_size);

/**
 * hwkey_close() - Closes the session.
 */
void hwkey_close(hwkey_session_t session);

__END_CDECLS

