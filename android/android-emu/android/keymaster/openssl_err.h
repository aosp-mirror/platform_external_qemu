/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef SYSTEM_KEYMASTER_OPENSSL_ERR_H_
#define SYSTEM_KEYMASTER_OPENSSL_ERR_H_

#include <hardware/keymaster_defs.h>
#include <keymaster/logger.h>

namespace keymaster {

/**
 * Translate the last OpenSSL error to a keymaster error.  Does not remove the error from the queue.
 */
keymaster_error_t TranslateLastOpenSslError(bool log_message = true);

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_OPENSSL_ERR_H_
