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

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

#ifdef DEBUG
#   define  DD(...)    do { printf("%s:%d: ", __FUNCTION__, __LINE__); printf(__VA_ARGS__); printf("\n");fflush(stdout); } while (0)
#else
#   define  DD(...)   ((void)0)
#endif

//#include <err.h>

#include <interface/keymaster/keymaster.h>

#include <UniquePtr.h>

#include "keymaster/soft_keymaster_device.h"
#include "keymaster/soft_keymaster_context.h"
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


//static TrustyKeymaster* device = nullptr;
static AndroidKeymaster* device = nullptr;
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
    // req.dump(stdout);
    // fflush(stdout);

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
    DD("Dispatching command %d", msg->cmd);
    switch (msg->cmd) {
    case KM_GENERATE_KEY:
        DD("Dispatching GENERATE_KEY, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::GenerateKey, msg, payload_size, out, out_size);

    case KM_BEGIN_OPERATION:
        DD("Dispatching BEGIN_OPERATION, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::BeginOperation, msg, payload_size, out, out_size);

    case KM_UPDATE_OPERATION:
        DD("Dispatching UPDATE_OPERATION, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::UpdateOperation, msg, payload_size, out, out_size);

    case KM_FINISH_OPERATION:
        DD("Dispatching FINISH_OPERATION, size: %d", payload_size);
        {
            long ret = do_dispatch(&AndroidKeymaster::FinishOperation, msg, payload_size, out, out_size);
            return ret;
        }

    case KM_IMPORT_KEY:
        DD("Dispatching IMPORT_KEY, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::ImportKey, msg, payload_size, out, out_size);

    case KM_EXPORT_KEY:
        DD("Dispatching EXPORT_KEY, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::ExportKey, msg, payload_size, out, out_size);

    case KM_GET_VERSION:
        DD("Dispatching GET_VERSION, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::GetVersion, msg, payload_size, out, out_size);

    case KM_ADD_RNG_ENTROPY:
        DD("Dispatching ADD_RNG_ENTROPY, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::AddRngEntropy, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_ALGORITHMS:
        DD("Dispatching GET_SUPPORTED_ALGORITHMS, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::SupportedAlgorithms, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_BLOCK_MODES:
        DD("Dispatching GET_SUPPORTED_BLOCK_MODES, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::SupportedBlockModes, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_PADDING_MODES:
        DD("Dispatching GET_SUPPORTED_PADDING_MODES, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::SupportedPaddingModes, msg, payload_size, out,
                           out_size);

    case KM_GET_SUPPORTED_DIGESTS:
        DD("Dispatching GET_SUPPORTED_DIGESTS, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::SupportedDigests, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_IMPORT_FORMATS:
        DD("Dispatching GET_SUPPORTED_IMPORT_FORMATS, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::SupportedImportFormats, msg, payload_size, out,
                           out_size);

    case KM_GET_SUPPORTED_EXPORT_FORMATS:
        DD("Dispatching GET_SUPPORTED_EXPORT_FORMATS, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::SupportedExportFormats, msg, payload_size, out,
                           out_size);

    case KM_GET_KEY_CHARACTERISTICS:
        DD("Dispatching GET_KEY_CHARACTERISTICS, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::GetKeyCharacteristics, msg, payload_size, out,
                           out_size);

    case KM_ATTEST_KEY:
        DD("Dispatching ATTEST_KEY, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::AttestKey, msg, payload_size, out,
                           out_size);

    case KM_UPGRADE_KEY:
        DD("Dispatching UPGRADE_KEY, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::UpgradeKey, msg, payload_size, out,
                           out_size);

    case KM_CONFIGURE:
        DD("Dispatching CONFIGURE, size: %d", payload_size);
        return do_dispatch(&AndroidKeymaster::Configure, msg, payload_size, out,
                           out_size);

    case KM_ABORT_OPERATION:
        DD("Dispatching ABORT_OPERATION, size %d", payload_size);
        return do_dispatch(&AndroidKeymaster::AbortOperation, msg, payload_size, out, out_size);
    default:
        DD("cannot find command %d", msg->cmd);
        return ERR_NOT_IMPLEMENTED;
    }
}

int keymaster_ipc_call(const std::vector<uint8_t> &cinput, std::vector<uint8_t> *output) {
    std::vector<uint8_t> input(cinput);
    // do some converting and call other method
    keymaster_message *msg =  reinterpret_cast<keymaster_message*>(input.data());
    UniquePtr<uint8_t[]> out;
    DD("input size %d\n", (int)(input.size()));
    uint32_t payload_size=input.size() - sizeof(keymaster_message);
    DD("payload size %d\n", (int)(payload_size));
    uint32_t out_size=0;
    keymaster_dispatch_non_secure(msg, payload_size, &out, &out_size);
    DD("output size %d\n", (int)(out_size));
    output->resize(out_size);
    //int64_t out64 = out_size;
    //memcpy(output->data(), &(out64), 8);
    memcpy(output->data(), out.get(), out_size);
    return 0;
}

class SoftwareOnlyHidlKeymasterEnforcement : public ::keymaster::KeymasterEnforcement {
  public:
    SoftwareOnlyHidlKeymasterEnforcement() : KeymasterEnforcement(64, 64) {}

    uint32_t get_current_time() const override {
#if defined(CLOCK_MONOTONIC) && !defined(__APPLE__)
        struct timespec tp;
        int err = clock_gettime(CLOCK_MONOTONIC, &tp);
        if (err || tp.tv_sec < 0) return 0;
        return static_cast<uint32_t>(tp.tv_sec);
#else
        timeval tv;
        gettimeofday(&tv, nullptr);
        return static_cast<uint32_t>(tv.tv_sec);
#endif // !defined(CLOCK_MONOTONIC) || defined(__APPLE__)
    }

    bool activation_date_valid(uint64_t) const override { return true; }
    bool expiration_date_passed(uint64_t) const override { return false; }
    bool auth_token_timed_out(const hw_auth_token_t&, uint32_t) const override { return false; }
    bool ValidateTokenSignature(const hw_auth_token_t&) const override { return true; }
};

class SoftwareOnlyHidlKeymasterContext : public ::keymaster::SoftKeymasterContext {
  public:
    SoftwareOnlyHidlKeymasterContext() : enforcement_(new SoftwareOnlyHidlKeymasterEnforcement) {}

    ::keymaster::KeymasterEnforcement* enforcement_policy() override { return enforcement_.get(); }

  private:
    std::unique_ptr<::keymaster::KeymasterEnforcement> enforcement_;
};

int keymaster_ipc_init(void) {

#ifdef DEBUG
    TrustyLogger::initialize();
#endif

    device = new AndroidKeymaster(new SoftwareOnlyHidlKeymasterContext, 16);

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
