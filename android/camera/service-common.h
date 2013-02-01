/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_CAMERA_SERVICE_H_
#define ANDROID_CAMERA_SERVICE_H_

#include "qemu-common.h"
#include "android/hw-qemud.h"

extern int parse_query(const char* query, char* query_name,
                       int query_name_size, const char** query_param);

extern void qemu_client_reply_payload(QemudClient* qc, size_t payload_size);

extern void qemu_client_query_reply(QemudClient* qc, int ok_ko,
                                    const void* extra, size_t extra_size);

extern void qemu_client_reply_ok(QemudClient* qc, const char* ok_str);

extern void qemu_client_reply_ko(QemudClient* qc, const char* ko_str);

#endif  /* ANDROID_CAMERA_SERVICE_H_ */
