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

// TODO: add guard in header
extern "C" {
#include <stdlib.h>
}

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <err.h>

#include <interface/keymaster/keymaster.h>

#include <UniquePtr.h>

#include "trusty_keymaster.h"
#include "trusty_logger.h"
#include "keymaster_ipc.h"
#define NO_ERROR 0
#define ERR_GENERIC 1
#define ERR_NOT_VALID 2
#define ERR_NO_MSG 3
#define ERR_NOT_IMPLEMENTED 4
#define ERR_NO_MEMORY 5
#define ERR_BAD_STATE 6
#define ERR_NOT_ENOUGH_BUFFER 7


using namespace keymaster;


static TrustyKeymaster* device = nullptr;
int32_t message_version = -1;


template <typename Request, typename Response>
static long do_dispatch(void (AndroidKeymaster::*operation)(const Request&, Response*),
                        struct keymaster_message* msg, uint32_t payload_size,
                        UniquePtr<uint8_t[]>* out, uint32_t* out_size) {
    const uint8_t* payload = msg->payload;
    Request req;
    req.message_version = message_version;
    if (!req.Deserialize(&payload, msg->payload + payload_size))
        return ERR_NOT_VALID;

    Response rsp;
    rsp.message_version = message_version;
    (device->*operation)(req, &rsp);

    *out_size = rsp.SerializedSize();
    if (*out_size > KEYMASTER_MAX_BUFFER_LENGTH) {
        *out_size = 0;
        return ERR_NOT_ENOUGH_BUFFER;
    }

    out->reset(new uint8_t[*out_size]);
    if (out->get() == NULL) {
        *out_size = 0;
        return ERR_NO_MEMORY;
    }

    rsp.Serialize(out->get(), out->get() + *out_size);

    return NO_ERROR;
}

static long keymaster_dispatch_non_secure(keymaster_message* msg,
                                          uint32_t payload_size, UniquePtr<uint8_t[]>* out,
                                          uint32_t* out_size) {
    LOG_D("Dispatching command %d", msg->cmd);
    switch (msg->cmd) {
    case KM_GENERATE_KEY:
        LOG_D("Dispatching GENERATE_KEY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::GenerateKey, msg, payload_size, out, out_size);

    case KM_BEGIN_OPERATION:
        LOG_D("Dispatching BEGIN_OPERATION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::BeginOperation, msg, payload_size, out, out_size);

    case KM_UPDATE_OPERATION:
        LOG_D("Dispatching UPDATE_OPERATION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::UpdateOperation, msg, payload_size, out, out_size);

    case KM_FINISH_OPERATION:
        LOG_D("Dispatching FINISH_OPERATION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::FinishOperation, msg, payload_size, out, out_size);

    case KM_IMPORT_KEY:
        LOG_D("Dispatching IMPORT_KEY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::ImportKey, msg, payload_size, out, out_size);

    case KM_EXPORT_KEY:
        LOG_D("Dispatching EXPORT_KEY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::ExportKey, msg, payload_size, out, out_size);

    case KM_GET_VERSION:
        LOG_D("Dispatching GET_VERSION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::GetVersion, msg, payload_size, out, out_size);

    case KM_ADD_RNG_ENTROPY:
        LOG_D("Dispatching ADD_RNG_ENTROPY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::AddRngEntropy, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_ALGORITHMS:
        LOG_D("Dispatching GET_SUPPORTED_ALGORITHMS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedAlgorithms, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_BLOCK_MODES:
        LOG_D("Dispatching GET_SUPPORTED_BLOCK_MODES, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedBlockModes, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_PADDING_MODES:
        LOG_D("Dispatching GET_SUPPORTED_PADDING_MODES, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedPaddingModes, msg, payload_size, out,
                           out_size);

    case KM_GET_SUPPORTED_DIGESTS:
        LOG_D("Dispatching GET_SUPPORTED_DIGESTS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedDigests, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_IMPORT_FORMATS:
        LOG_D("Dispatching GET_SUPPORTED_IMPORT_FORMATS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedImportFormats, msg, payload_size, out,
                           out_size);

    case KM_GET_SUPPORTED_EXPORT_FORMATS:
        LOG_D("Dispatching GET_SUPPORTED_EXPORT_FORMATS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedExportFormats, msg, payload_size, out,
                           out_size);

    case KM_GET_KEY_CHARACTERISTICS:
        LOG_D("Dispatching GET_KEY_CHARACTERISTICS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::GetKeyCharacteristics, msg, payload_size, out,
                           out_size);

    case KM_ABORT_OPERATION:
        LOG_D("Dispatching ABORT_OPERATION, size %d", payload_size);
        return do_dispatch(&TrustyKeymaster::AbortOperation, msg, payload_size, out, out_size);
    default:
        return ERR_NOT_IMPLEMENTED;
    }
}

int keymaster_ipc_call(std::vector<uint8_t> &input, std::vector<uint8_t> *output) {
    // do some converting and call other method
    keymaster_message *msg =  reinterpret_cast<keymaster_message*>(input.data());
    UniquePtr<uint8_t[]> out;
    uint32_t payload_size=input.size();
    uint32_t out_size=0;
    keymaster_dispatch_non_secure(msg, payload_size, &out, &out_size);
    output->resize(out_size);
    memcpy(output->data(), out.get(), out_size);
    return 0;
}

int keymaster_ipc_init(void) {

    device = new TrustyKeymaster(new TrustyKeymasterContext, 16);

    TrustyLogger::initialize();

    LOG_I("Initializing", 0);

    GetVersionRequest request;
    GetVersionResponse response;
    device->GetVersion(request, &response);
    if (response.error == KM_ERROR_OK) {
      message_version = MessageVersion(
          response.major_ver, response.minor_ver, response.subminor_ver);
    } else {
      LOG_E("Error %d determining AndroidKeymaster version.", response.error);
      return ERR_GENERIC;
    }
    return 0;
}
