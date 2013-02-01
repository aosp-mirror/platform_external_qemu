/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/hw-qemud.h"
#include "android/utils/system.h"
#include "android/utils/debug.h"
#include "android/camera/service-common.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(camera)

/* the T(...) macro is used to dump traffic */
#define  T_ACTIVE   0

#if T_ACTIVE
#define  T(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#else
#define  T(...)    ((void)0)
#endif

/* Extracts query name, and (optionally) query parameters from the query string.
 * Param:
 *  query - Query string. Query string in the camera service are formatted as such:
 *          "<query name>[ <parameters>]",
 *      where parameters are optional, and if present, must be separated from the
 *      query name with a single ' '. See comments to get_token_value routine
 *      for the format of the parameters string.
 *  query_name - Upon success contains query name extracted from the query
 *      string.
 *  query_name_size - Buffer size for 'query_name' string.
 *  query_param - Upon success contains a pointer to the beginning of the query
 *      parameters. If query has no parameters, NULL will be passed back with
 *      this parameter. This parameter is optional and can be NULL.
 * Return:
 *  0 on success, or number of bytes required for query name if 'query_name'
 *  string buffer was too small to contain it.
 */
int
parse_query(const char* query,
            char* query_name,
            int query_name_size,
            const char** query_param)
{
    /* Extract query name. */
    const char* qend = strchr(query, ' ');
    if (qend == NULL) {
        qend = query + strlen(query);
    }
    if ((qend - query) >= query_name_size) {
        return qend - query + 1;
    }
    memcpy(query_name, query, qend - query);
    query_name[qend - query] = '\0';

    /* Calculate query parameters pointer (if needed) */
    if (query_param != NULL) {
        if (*qend == ' ') {
            qend++;
        }
        *query_param = (*qend == '\0') ? NULL : qend;
    }

    return 0;
}

/********************************************************************************
 * Helpers for handling camera client queries
 *******************************************************************************/

/* Formats paload size according to the protocol, and sends it to the client.
 * To simplify endianess handling we convert payload size to an eight characters
 * string, representing payload size value in hexadecimal format.
 * Param:
 *  qc - Qemu client to send the payload size to.
 *  payload_size - Payload size to report to the client.
 */
void
qemu_client_reply_payload(QemudClient* qc, size_t payload_size)
{
    char payload_size_str[9];
    snprintf(payload_size_str, sizeof(payload_size_str), "%08zx", payload_size);
    qemud_client_send(qc, (const uint8_t*)payload_size_str, 8);
}

/*
 * Prefixes for replies to camera client queries.
 */

/* Success, no data to send in reply. */
#define OK_REPLY        "ok"
/* Failure, no data to send in reply. */
#define KO_REPLY        "ko"
/* Success, there are data to send in reply. */
#define OK_REPLY_DATA   OK_REPLY ":"
/* Failure, there are data to send in reply. */
#define KO_REPLY_DATA   KO_REPLY ":"

/* Builds and sends a reply to a query.
 * All replies to a query in camera service have a prefix indicating whether the
 * query has succeeded ("ok"), or failed ("ko"). The prefix can be followed by
 * extra data, containing response to the query. In case there are extra data,
 * they are separated from the prefix with a ':' character.
 * Param:
 *  qc - Qemu client to send the reply to.
 *  ok_ko - An "ok", or "ko" selector, where 0 is for "ko", and !0 is for "ok".
 *  extra - Optional extra query data. Can be NULL.
 *  extra_size - Extra data size.
 */
void
qemu_client_query_reply(QemudClient* qc,
                         int ok_ko,
                         const void* extra,
                         size_t extra_size)
{
    const char* ok_ko_str;
    size_t payload_size;

    /* Make sure extra_size is 0 if extra is NULL. */
    if (extra == NULL && extra_size != 0) {
        W("%s: 'extra' = NULL, while 'extra_size' = %d",
          __FUNCTION__, (int)extra_size);
        extra_size = 0;
    }

    /* Calculate total payload size, and select appropriate 'ok'/'ko' prefix */
    if (extra_size) {
        /* 'extra' size + 2 'ok'/'ko' bytes + 1 ':' separator byte. */
        payload_size = extra_size + 3;
        ok_ko_str = ok_ko ? OK_REPLY_DATA : KO_REPLY_DATA;
    } else {
        /* No extra data: just zero-terminated 'ok'/'ko'. */
        payload_size = 3;
        ok_ko_str = ok_ko ? OK_REPLY : KO_REPLY;
    }

    /* Send payload size first. */
    qemu_client_reply_payload(qc, payload_size);
    /* Send 'ok[:]'/'ko[:]' next. Note that if there is no extra data, we still
     * need to send a zero-terminator for 'ok'/'ko' string instead of the ':'
     * separator. So, one way or another, the prefix is always 3 bytes. */
    qemud_client_send(qc, (const uint8_t*)ok_ko_str, 3);
    /* Send extra data (if present). */
    if (extra != NULL) {
        qemud_client_send(qc, (const uint8_t*)extra, extra_size);
    }
}

/* Replies query success ("OK") back to the client.
 * Param:
 *  qc - Qemu client to send the reply to.
 *  ok_str - An optional string containing query results. Can be NULL.
 */
void
qemu_client_reply_ok(QemudClient* qc, const char* ok_str)
{
    qemu_client_query_reply(qc, 1, ok_str,
                             (ok_str != NULL) ? (strlen(ok_str) + 1) : 0);
}

/* Replies query failure ("KO") back to the client.
 * Param:
 *  qc - Qemu client to send the reply to.
 *  ko_str - An optional string containing reason for failure. Can be NULL.
 */
void
qemu_client_reply_ko(QemudClient* qc, const char* ko_str)
{
    qemu_client_query_reply(qc, 0, ko_str,
                             (ko_str != NULL) ? (strlen(ko_str) + 1) : 0);
}
