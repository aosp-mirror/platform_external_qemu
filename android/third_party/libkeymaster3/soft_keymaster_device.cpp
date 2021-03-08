/*
 * Copyright 2015 The Android Open Source Project
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

#include <keymaster/soft_keymaster_device.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <vector>
#include <iterator>

#include <type_traits>

#include <openssl/x509.h>

#include <hardware/keymaster1.h>
#define LOG_TAG "SoftKeymasterDevice"
#include <cutils/log.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/android_keymaster_messages.h>
#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>
#include <keymaster/soft_keymaster_context.h>
#include <keymaster/soft_keymaster_logger.h>

#include "openssl_utils.h"

struct keystore_module soft_keymaster1_device_module = {
    .common =
        {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = KEYMASTER_MODULE_API_VERSION_1_0,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = KEYSTORE_HARDWARE_MODULE_ID,
            .name = "OpenSSL-based SoftKeymaster HAL",
            .author = "The Android Open Source Project",
            .methods = nullptr,
            .dso = 0,
            .reserved = {},
        },
};

struct keystore_module soft_keymaster2_device_module = {
    .common =
        {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = KEYMASTER_MODULE_API_VERSION_2_0,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = KEYSTORE_HARDWARE_MODULE_ID,
            .name = "OpenSSL-based SoftKeymaster HAL",
            .author = "The Android Open Source Project",
            .methods = nullptr,
            .dso = 0,
            .reserved = {},
        },
};

namespace keymaster {

const size_t kMaximumAttestationChallengeLength = 128;
const size_t kOperationTableSize = 16;

template <typename T> std::vector<T> make_vector(const T* array, size_t len) {
    return std::vector<T>(array, array + len);
}

// This helper class implements just enough of the C++ standard collection interface to be able to
// accept push_back calls, and it does nothing but count them.  It's useful when you want to count
// insertions but not actually store anything.  It's used in digest_set_is_full below to count the
// size of a set intersection.
struct PushbackCounter {
    struct value_type {
        template <typename T> value_type(const T&) {}
    };
    void push_back(const value_type&) { ++count; }
    size_t count = 0;
};

static std::vector<keymaster_digest_t> full_digest_list = {
    KM_DIGEST_MD5,       KM_DIGEST_SHA1,      KM_DIGEST_SHA_2_224,
    KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512};

template <typename Iter> static bool digest_set_is_full(Iter begin, Iter end) {
    PushbackCounter counter;
    std::set_intersection(begin, end, full_digest_list.begin(), full_digest_list.end(),
                          std::back_inserter(counter));
    return counter.count == full_digest_list.size();
}

static keymaster_error_t add_digests(keymaster1_device_t* dev, keymaster_algorithm_t algorithm,
                                     keymaster_purpose_t purpose,
                                     SoftKeymasterDevice::DigestMap* map, bool* supports_all) {
    auto key = std::make_pair(algorithm, purpose);

    keymaster_digest_t* digests;
    size_t digests_length;
    keymaster_error_t error =
        dev->get_supported_digests(dev, algorithm, purpose, &digests, &digests_length);
    if (error != KM_ERROR_OK) {
        LOG_E("Error %d getting supported digests from keymaster1 device", error);
        return error;
    }
    std::unique_ptr<keymaster_digest_t, Malloc_Delete> digests_deleter(digests);

    auto digest_vec = make_vector(digests, digests_length);
    *supports_all = digest_set_is_full(digest_vec.begin(), digest_vec.end());
    (*map)[key] = std::move(digest_vec);
    return error;
}

static keymaster_error_t map_digests(keymaster1_device_t* dev, SoftKeymasterDevice::DigestMap* map,
                                     bool* supports_all) {
    map->clear();
    *supports_all = true;

    keymaster_algorithm_t sig_algorithms[] = {KM_ALGORITHM_RSA, KM_ALGORITHM_EC, KM_ALGORITHM_HMAC};
    keymaster_purpose_t sig_purposes[] = {KM_PURPOSE_SIGN, KM_PURPOSE_VERIFY};
    for (auto algorithm : sig_algorithms)
        for (auto purpose : sig_purposes) {
            bool alg_purpose_supports_all;
            keymaster_error_t error =
                add_digests(dev, algorithm, purpose, map, &alg_purpose_supports_all);
            if (error != KM_ERROR_OK)
                return error;
            *supports_all &= alg_purpose_supports_all;
        }

    keymaster_algorithm_t crypt_algorithms[] = {KM_ALGORITHM_RSA};
    keymaster_purpose_t crypt_purposes[] = {KM_PURPOSE_ENCRYPT, KM_PURPOSE_DECRYPT};
    for (auto algorithm : crypt_algorithms)
        for (auto purpose : crypt_purposes) {
            bool alg_purpose_supports_all;
            keymaster_error_t error =
                add_digests(dev, algorithm, purpose, map, &alg_purpose_supports_all);
            if (error != KM_ERROR_OK)
                return error;
            *supports_all &= alg_purpose_supports_all;
        }

    return KM_ERROR_OK;
}

SoftKeymasterDevice::SoftKeymasterDevice()
    : wrapped_km0_device_(nullptr), wrapped_km1_device_(nullptr),
      context_(new SoftKeymasterContext),
      impl_(new AndroidKeymaster(context_, kOperationTableSize)), configured_(false) {
    LOG_I("Creating device", 0);
    LOG_D("Device address: %p", this);

    initialize_device_struct(KEYMASTER_BLOBS_ARE_STANDALONE |
                             KEYMASTER_SUPPORTS_EC);
}

SoftKeymasterDevice::SoftKeymasterDevice(SoftKeymasterContext* context)
    : wrapped_km0_device_(nullptr), wrapped_km1_device_(nullptr), context_(context),
      impl_(new AndroidKeymaster(context_, kOperationTableSize)), configured_(false) {
    ALOGE("Creating test device with context");
    ALOGE("Device address: %p", this);

    initialize_device_struct(KEYMASTER_BLOBS_ARE_STANDALONE |
                             KEYMASTER_SUPPORTS_EC);
}

keymaster_error_t SoftKeymasterDevice::SetHardwareDevice(keymaster0_device_t* keymaster0_device) {
    assert(keymaster0_device);
    LOG_D("Reinitializing SoftKeymasterDevice to use HW keymaster0", 0);

    if (!context_)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    supports_all_digests_ = false;
    keymaster_error_t error = context_->SetHardwareDevice(keymaster0_device);
    if (error != KM_ERROR_OK)
        return error;

    initialize_device_struct(keymaster0_device->flags);

    module_name_ = km1_device_.common.module->name;
    module_name_.append("(Wrapping ");
    module_name_.append(keymaster0_device->common.module->name);
    module_name_.append(")");

    updated_module_ = *km1_device_.common.module;
    updated_module_.name = module_name_.c_str();

    km1_device_.common.module = &updated_module_;

    wrapped_km0_device_ = keymaster0_device;
    wrapped_km1_device_ = nullptr;
    return KM_ERROR_OK;
}

keymaster_error_t SoftKeymasterDevice::SetHardwareDevice(keymaster1_device_t* keymaster1_device) {
    assert(keymaster1_device);
    LOG_D("Reinitializing SoftKeymasterDevice to use HW keymaster1", 0);

    if (!context_)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    keymaster_error_t error =
        map_digests(keymaster1_device, &km1_device_digests_, &supports_all_digests_);
    if (error != KM_ERROR_OK)
        return error;

    error = context_->SetHardwareDevice(keymaster1_device);
    if (error != KM_ERROR_OK)
        return error;

    initialize_device_struct(keymaster1_device->flags);

    module_name_ = km1_device_.common.module->name;
    module_name_.append(" (Wrapping ");
    module_name_.append(keymaster1_device->common.module->name);
    module_name_.append(")");

    updated_module_ = *km1_device_.common.module;
    updated_module_.name = module_name_.c_str();

    km1_device_.common.module = &updated_module_;

    wrapped_km0_device_ = nullptr;
    wrapped_km1_device_ = keymaster1_device;
    return KM_ERROR_OK;
}

bool SoftKeymasterDevice::Keymaster1DeviceIsGood() {
    std::vector<keymaster_digest_t> expected_rsa_digests = {
        KM_DIGEST_NONE,      KM_DIGEST_MD5,       KM_DIGEST_SHA1,     KM_DIGEST_SHA_2_224,
        KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512};
    std::vector<keymaster_digest_t> expected_ec_digests = {
        KM_DIGEST_NONE,      KM_DIGEST_SHA1,      KM_DIGEST_SHA_2_224,
        KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512};

    for (auto& entry : km1_device_digests_) {
        if (entry.first.first == KM_ALGORITHM_RSA)
            if (!std::is_permutation(entry.second.begin(), entry.second.end(),
                                     expected_rsa_digests.begin()))
                return false;
        if (entry.first.first == KM_ALGORITHM_EC)
            if (!std::is_permutation(entry.second.begin(), entry.second.end(),
                                     expected_ec_digests.begin()))
                return false;
    }
    return true;
}

void SoftKeymasterDevice::initialize_device_struct(uint32_t flags) {
    memset(&km1_device_, 0, sizeof(km1_device_));

    km1_device_.common.tag = HARDWARE_DEVICE_TAG;
    km1_device_.common.version = 1;
    km1_device_.common.module = reinterpret_cast<hw_module_t*>(&soft_keymaster1_device_module);
    km1_device_.common.close = &close_device;

    km1_device_.flags = flags;

    km1_device_.context = this;

    // keymaster0 APIs
    km1_device_.generate_keypair = nullptr;
    km1_device_.import_keypair = nullptr;
    km1_device_.get_keypair_public = nullptr;
    km1_device_.delete_keypair = nullptr;
    km1_device_.delete_all = nullptr;
    km1_device_.sign_data = nullptr;
    km1_device_.verify_data = nullptr;

    // keymaster1 APIs
    km1_device_.get_supported_algorithms = get_supported_algorithms;
    km1_device_.get_supported_block_modes = get_supported_block_modes;
    km1_device_.get_supported_padding_modes = get_supported_padding_modes;
    km1_device_.get_supported_digests = get_supported_digests;
    km1_device_.get_supported_import_formats = get_supported_import_formats;
    km1_device_.get_supported_export_formats = get_supported_export_formats;
    km1_device_.add_rng_entropy = add_rng_entropy;
    km1_device_.generate_key = generate_key;
    km1_device_.get_key_characteristics = get_key_characteristics;
    km1_device_.import_key = import_key;
    km1_device_.export_key = export_key;
    km1_device_.delete_key = delete_key;
    km1_device_.delete_all_keys = delete_all_keys;
    km1_device_.begin = begin;
    km1_device_.update = update;
    km1_device_.finish = finish;
    km1_device_.abort = abort;

    // keymaster2 APIs
    memset(&km2_device_, 0, sizeof(km2_device_));

    km2_device_.flags = flags;
    km2_device_.context = this;
    ALOGE("%s line: %d keymaster flags: %d", __func__, __LINE__, flags);

    km2_device_.common.tag = HARDWARE_DEVICE_TAG;
    km2_device_.common.version = 1;
    km2_device_.common.module = reinterpret_cast<hw_module_t*>(&soft_keymaster2_device_module);
    km2_device_.common.close = &close_device;

    km2_device_.configure = configure;
    km2_device_.add_rng_entropy = add_rng_entropy;
    km2_device_.generate_key = generate_key;
    km2_device_.get_key_characteristics = get_key_characteristics;
    km2_device_.import_key = import_key;
    km2_device_.export_key = export_key;
    km2_device_.attest_key = attest_key;
    km2_device_.upgrade_key = upgrade_key;
    km2_device_.delete_key = delete_key;
    km2_device_.delete_all_keys = delete_all_keys;
    km2_device_.begin = begin;
    km2_device_.update = update;
    km2_device_.finish = finish;
    km2_device_.abort = abort;
}

hw_device_t* SoftKeymasterDevice::hw_device() {
    return &km1_device_.common;
}

keymaster1_device_t* SoftKeymasterDevice::keymaster_device() {
    return &km1_device_;
}

keymaster2_device_t* SoftKeymasterDevice::keymaster2_device() {
    return &km2_device_;
}

namespace {

keymaster_key_characteristics_t* BuildCharacteristics(const AuthorizationSet& hw_enforced,
                                                      const AuthorizationSet& sw_enforced) {
    keymaster_key_characteristics_t* characteristics =
        reinterpret_cast<keymaster_key_characteristics_t*>(
            malloc(sizeof(keymaster_key_characteristics_t)));
    if (characteristics) {
        hw_enforced.CopyToParamSet(&characteristics->hw_enforced);
        sw_enforced.CopyToParamSet(&characteristics->sw_enforced);
    }
    return characteristics;
}

template <typename RequestType>
void AddClientAndAppData(const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
                         RequestType* request) {
    request->additional_params.Clear();
    if (client_id)
        request->additional_params.push_back(TAG_APPLICATION_ID, *client_id);
    if (app_data)
        request->additional_params.push_back(TAG_APPLICATION_DATA, *app_data);
}

template <typename T> SoftKeymasterDevice* convert_device(const T* dev) {
    static_assert((std::is_same<T, keymaster0_device_t>::value ||
                   std::is_same<T, keymaster1_device_t>::value ||
                   std::is_same<T, keymaster2_device_t>::value),
                  "convert_device should only be applied to keymaster devices");
    return reinterpret_cast<SoftKeymasterDevice*>(dev->context);
}

template <keymaster_tag_t Tag, keymaster_tag_type_t Type, typename KeymasterEnum>
bool FindTagValue(const keymaster_key_param_set_t& params,
                  TypedEnumTag<Type, Tag, KeymasterEnum> tag, KeymasterEnum* value) {
    for (size_t i = 0; i < params.length; ++i)
        if (params.params[i].tag == tag) {
            *value = static_cast<KeymasterEnum>(params.params[i].enumerated);
            return true;
        }
    return false;
}

}  // unnamed namespaced

/* static */
int SoftKeymasterDevice::close_device(hw_device_t* dev) {
    switch (dev->module->module_api_version) {
    case KEYMASTER_MODULE_API_VERSION_2_0: {
        delete convert_device(reinterpret_cast<keymaster2_device_t*>(dev));
        break;
    }

    case KEYMASTER_MODULE_API_VERSION_1_0: {
        delete convert_device(reinterpret_cast<keymaster1_device_t*>(dev));
        break;
    }

    default:
        return -1;
    }

    return 0;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_supported_algorithms(const keymaster1_device_t* dev,
                                                                keymaster_algorithm_t** algorithms,
                                                                size_t* algorithms_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!algorithms || !algorithms_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->get_supported_algorithms(km1_dev, algorithms, algorithms_length);

    SupportedAlgorithmsRequest request;
    SupportedAlgorithmsResponse response;
    convert_device(dev)->impl_->SupportedAlgorithms(request, &response);
    if (response.error != KM_ERROR_OK) {
        LOG_E("get_supported_algorithms failed with %d", response.error);

        return response.error;
    }

    *algorithms_length = response.results_length;
    *algorithms =
        reinterpret_cast<keymaster_algorithm_t*>(malloc(*algorithms_length * sizeof(**algorithms)));
    if (!*algorithms)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *algorithms);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_supported_block_modes(const keymaster1_device_t* dev,
                                                                 keymaster_algorithm_t algorithm,
                                                                 keymaster_purpose_t purpose,
                                                                 keymaster_block_mode_t** modes,
                                                                 size_t* modes_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!modes || !modes_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->get_supported_block_modes(km1_dev, algorithm, purpose, modes, modes_length);

    SupportedBlockModesRequest request;
    request.algorithm = algorithm;
    request.purpose = purpose;
    SupportedBlockModesResponse response;
    convert_device(dev)->impl_->SupportedBlockModes(request, &response);

    if (response.error != KM_ERROR_OK) {
        LOG_E("get_supported_block_modes failed with %d", response.error);

        return response.error;
    }

    *modes_length = response.results_length;
    *modes = reinterpret_cast<keymaster_block_mode_t*>(malloc(*modes_length * sizeof(**modes)));
    if (!*modes)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *modes);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_supported_padding_modes(const keymaster1_device_t* dev,
                                                                   keymaster_algorithm_t algorithm,
                                                                   keymaster_purpose_t purpose,
                                                                   keymaster_padding_t** modes,
                                                                   size_t* modes_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!modes || !modes_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->get_supported_padding_modes(km1_dev, algorithm, purpose, modes,
                                                    modes_length);

    SupportedPaddingModesRequest request;
    request.algorithm = algorithm;
    request.purpose = purpose;
    SupportedPaddingModesResponse response;
    convert_device(dev)->impl_->SupportedPaddingModes(request, &response);

    if (response.error != KM_ERROR_OK) {
        LOG_E("get_supported_padding_modes failed with %d", response.error);
        return response.error;
    }

    *modes_length = response.results_length;
    *modes = reinterpret_cast<keymaster_padding_t*>(malloc(*modes_length * sizeof(**modes)));
    if (!*modes)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *modes);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_supported_digests(const keymaster1_device_t* dev,
                                                             keymaster_algorithm_t algorithm,
                                                             keymaster_purpose_t purpose,
                                                             keymaster_digest_t** digests,
                                                             size_t* digests_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!digests || !digests_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->get_supported_digests(km1_dev, algorithm, purpose, digests, digests_length);

    SupportedDigestsRequest request;
    request.algorithm = algorithm;
    request.purpose = purpose;
    SupportedDigestsResponse response;
    convert_device(dev)->impl_->SupportedDigests(request, &response);

    if (response.error != KM_ERROR_OK) {
        LOG_E("get_supported_digests failed with %d", response.error);
        return response.error;
    }

    *digests_length = response.results_length;
    *digests = reinterpret_cast<keymaster_digest_t*>(malloc(*digests_length * sizeof(**digests)));
    if (!*digests)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *digests);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_supported_import_formats(
    const keymaster1_device_t* dev, keymaster_algorithm_t algorithm,
    keymaster_key_format_t** formats, size_t* formats_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!formats || !formats_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->get_supported_import_formats(km1_dev, algorithm, formats, formats_length);

    SupportedImportFormatsRequest request;
    request.algorithm = algorithm;
    SupportedImportFormatsResponse response;
    convert_device(dev)->impl_->SupportedImportFormats(request, &response);

    if (response.error != KM_ERROR_OK) {
        LOG_E("get_supported_import_formats failed with %d", response.error);
        return response.error;
    }

    *formats_length = response.results_length;
    *formats =
        reinterpret_cast<keymaster_key_format_t*>(malloc(*formats_length * sizeof(**formats)));
    if (!*formats)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *formats);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_supported_export_formats(
    const keymaster1_device_t* dev, keymaster_algorithm_t algorithm,
    keymaster_key_format_t** formats, size_t* formats_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!formats || !formats_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->get_supported_export_formats(km1_dev, algorithm, formats, formats_length);

    SupportedExportFormatsRequest request;
    request.algorithm = algorithm;
    SupportedExportFormatsResponse response;
    convert_device(dev)->impl_->SupportedExportFormats(request, &response);

    if (response.error != KM_ERROR_OK) {
        LOG_E("get_supported_export_formats failed with %d", response.error);
        return response.error;
    }

    *formats_length = response.results_length;
    *formats =
        reinterpret_cast<keymaster_key_format_t*>(malloc(*formats_length * sizeof(**formats)));
    if (!*formats)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + *formats_length, *formats);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::configure(const keymaster2_device_t* dev,
                                                 const keymaster_key_param_set_t* params) {
    AuthorizationSet params_copy(*params);
    uint32_t os_version;
    uint32_t os_patchlevel;
    if (!params_copy.GetTagValue(TAG_OS_VERSION, &os_version) ||
        !params_copy.GetTagValue(TAG_OS_PATCHLEVEL, &os_patchlevel)) {
        LOG_E("Configuration parameters must contain OS version and patch level", 0);
        return KM_ERROR_INVALID_ARGUMENT;
    }

    keymaster_error_t error =
        convert_device(dev)->context_->SetSystemVersion(os_version, os_patchlevel);
    if (error == KM_ERROR_OK)
        convert_device(dev)->configured_ = true;
    return error;
}

/* static */
keymaster_error_t SoftKeymasterDevice::add_rng_entropy(const keymaster1_device_t* dev,
                                                       const uint8_t* data, size_t data_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->add_rng_entropy(km1_dev, data, data_length);

    AddEntropyRequest request;
    request.random_data.Reinitialize(data, data_length);
    AddEntropyResponse response;
    convert_device(dev)->impl_->AddRngEntropy(request, &response);
    if (response.error != KM_ERROR_OK)
        LOG_E("add_rng_entropy failed with %d", response.error);
    return response.error;
}

/* static */
keymaster_error_t SoftKeymasterDevice::add_rng_entropy(const keymaster2_device_t* dev,
                                                       const uint8_t* data, size_t data_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    SoftKeymasterDevice* sk_dev = convert_device(dev);
    return add_rng_entropy(&sk_dev->km1_device_, data, data_length);
}

template <typename Collection, typename Value> bool contains(const Collection& c, const Value& v) {
    return std::find(c.begin(), c.end(), v) != c.end();
}

bool SoftKeymasterDevice::FindUnsupportedDigest(keymaster_algorithm_t algorithm,
                                                keymaster_purpose_t purpose,
                                                const AuthorizationSet& params,
                                                keymaster_digest_t* unsupported) const {
    assert(wrapped_km1_device_);

    auto supported_digests = km1_device_digests_.find(std::make_pair(algorithm, purpose));
    if (supported_digests == km1_device_digests_.end())
        // Invalid algorith/purpose pair (e.g. EC encrypt).  Let the error be handled by HW module.
        return false;

    for (auto& entry : params)
        if (entry.tag == TAG_DIGEST)
            if (!contains(supported_digests->second, entry.enumerated)) {
                LOG_I("Digest %d requested but not supported by module %s", entry.enumerated,
                      wrapped_km1_device_->common.module->name);
                *unsupported = static_cast<keymaster_digest_t>(entry.enumerated);
                return true;
            }
    return false;
}

bool SoftKeymasterDevice::RequiresSoftwareDigesting(keymaster_algorithm_t algorithm,
                                                    keymaster_purpose_t purpose,
                                                    const AuthorizationSet& params) const {
    assert(wrapped_km1_device_);
    if (!wrapped_km1_device_)
        return true;

    switch (algorithm) {
    case KM_ALGORITHM_AES:
        LOG_D("Not performing software digesting for AES keys", algorithm);
        return false;
    case KM_ALGORITHM_HMAC:
    case KM_ALGORITHM_RSA:
    case KM_ALGORITHM_EC:
        break;
    }

    keymaster_digest_t unsupported;
    if (!FindUnsupportedDigest(algorithm, purpose, params, &unsupported)) {
        LOG_D("Requested digest(s) supported for algorithm %d and purpose %d", algorithm, purpose);
        return false;
    }

    return true;
}

bool SoftKeymasterDevice::KeyRequiresSoftwareDigesting(
    const AuthorizationSet& key_description) const {
    assert(wrapped_km1_device_);
    if (!wrapped_km1_device_)
        return true;

    keymaster_algorithm_t algorithm;
    if (!key_description.GetTagValue(TAG_ALGORITHM, &algorithm)) {
        // The hardware module will return an error during keygen.
        return false;
    }

    for (auto& entry : key_description)
        if (entry.tag == TAG_PURPOSE) {
            keymaster_purpose_t purpose = static_cast<keymaster_purpose_t>(entry.enumerated);
            if (RequiresSoftwareDigesting(algorithm, purpose, key_description))
                return true;
        }

    return false;
}

/* static */
keymaster_error_t SoftKeymasterDevice::generate_key(
    const keymaster1_device_t* dev, const keymaster_key_param_set_t* params,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t** characteristics) {
    if (!dev || !params)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SoftKeymasterDevice* sk_dev = convert_device(dev);

    GenerateKeyRequest request;
    request.key_description.Reinitialize(*params);

    keymaster1_device_t* km1_dev = sk_dev->wrapped_km1_device_;
    if (km1_dev && !sk_dev->KeyRequiresSoftwareDigesting(request.key_description))
        return km1_dev->generate_key(km1_dev, params, key_blob, characteristics);

    GenerateKeyResponse response;
    sk_dev->impl_->GenerateKey(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    key_blob->key_material_size = response.key_blob.key_material_size;
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(key_blob->key_material_size));
    if (!tmp)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(tmp, response.key_blob.key_material, response.key_blob.key_material_size);
    key_blob->key_material = tmp;

    if (characteristics) {
        // This is a keymaster1 method, and keymaster1 doesn't include version info, so remove it.
        response.enforced.erase(response.enforced.find(TAG_OS_VERSION));
        response.enforced.erase(response.enforced.find(TAG_OS_PATCHLEVEL));
        response.unenforced.erase(response.unenforced.find(TAG_OS_VERSION));
        response.unenforced.erase(response.unenforced.find(TAG_OS_PATCHLEVEL));

        *characteristics = BuildCharacteristics(response.enforced, response.unenforced);
        if (!*characteristics)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    return KM_ERROR_OK;
}

keymaster_error_t
SoftKeymasterDevice::generate_key(const keymaster2_device_t* dev,  //
                                  const keymaster_key_param_set_t* params,
                                  keymaster_key_blob_t* key_blob,
                                  keymaster_key_characteristics_t* characteristics) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SoftKeymasterDevice* sk_dev = convert_device(dev);

    GenerateKeyRequest request;
    request.key_description.Reinitialize(*params);

    keymaster1_device_t* km1_dev = sk_dev->wrapped_km1_device_;
    if (km1_dev && !sk_dev->KeyRequiresSoftwareDigesting(request.key_description)) {
        keymaster_ec_curve_t curve;
        if (request.key_description.Contains(TAG_ALGORITHM, KM_ALGORITHM_EC) &&
            request.key_description.GetTagValue(TAG_EC_CURVE, &curve)) {
            // Keymaster1 doesn't know about EC curves. We need to translate to key size.
            uint32_t key_size_from_curve;
            keymaster_error_t error = EcCurveToKeySize(curve, &key_size_from_curve);
            if (error != KM_ERROR_OK) {
                return error;
            }

            uint32_t key_size_from_desc;
            if (request.key_description.GetTagValue(TAG_KEY_SIZE, &key_size_from_desc)) {
                if (key_size_from_desc != key_size_from_curve) {
                    return KM_ERROR_INVALID_ARGUMENT;
                }
            } else {
                request.key_description.push_back(TAG_KEY_SIZE, key_size_from_curve);
            }
        }

        keymaster_key_characteristics_t* chars_ptr;
        keymaster_error_t error = km1_dev->generate_key(km1_dev, &request.key_description, key_blob,
                                                        characteristics ? &chars_ptr : nullptr);
        if (error != KM_ERROR_OK)
            return error;

        if (characteristics) {
            *characteristics = *chars_ptr;
            free(chars_ptr);
        }

        return KM_ERROR_OK;
    }

    GenerateKeyResponse response;
    sk_dev->impl_->GenerateKey(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    key_blob->key_material_size = response.key_blob.key_material_size;
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(key_blob->key_material_size));
    if (!tmp)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(tmp, response.key_blob.key_material, response.key_blob.key_material_size);
    key_blob->key_material = tmp;

    if (characteristics) {
        response.enforced.CopyToParamSet(&characteristics->hw_enforced);
        response.unenforced.CopyToParamSet(&characteristics->sw_enforced);
    }

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_key_characteristics(
    const keymaster1_device_t* dev, const keymaster_key_blob_t* key_blob,
    const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
    keymaster_key_characteristics_t** characteristics) {
    if (!dev || !key_blob || !key_blob->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!characteristics)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev) {
        keymaster_error_t error = km1_dev->get_key_characteristics(km1_dev, key_blob, client_id,
                                                                   app_data, characteristics);
        if (error != KM_ERROR_INVALID_KEY_BLOB) {
            return error;
        }
        // If we got "invalid blob", continue to try with the software device. This might be a
        // software key blob.
    }

    GetKeyCharacteristicsRequest request;
    request.SetKeyMaterial(*key_blob);
    AddClientAndAppData(client_id, app_data, &request);

    GetKeyCharacteristicsResponse response;
    convert_device(dev)->impl_->GetKeyCharacteristics(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    // This is a keymaster1 method, and keymaster1 doesn't include version info, so remove it.
    response.enforced.erase(response.enforced.find(TAG_OS_VERSION));
    response.enforced.erase(response.enforced.find(TAG_OS_PATCHLEVEL));
    response.unenforced.erase(response.unenforced.find(TAG_OS_VERSION));
    response.unenforced.erase(response.unenforced.find(TAG_OS_PATCHLEVEL));

    *characteristics = BuildCharacteristics(response.enforced, response.unenforced);
    if (!*characteristics)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::get_key_characteristics(
    const keymaster2_device_t* dev, const keymaster_key_blob_t* key_blob,
    const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
    keymaster_key_characteristics_t* characteristics) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (!characteristics)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SoftKeymasterDevice* sk_dev = convert_device(dev);

    GetKeyCharacteristicsRequest request;
    request.SetKeyMaterial(*key_blob);
    AddClientAndAppData(client_id, app_data, &request);

    GetKeyCharacteristicsResponse response;
    sk_dev->impl_->GetKeyCharacteristics(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    response.enforced.CopyToParamSet(&characteristics->hw_enforced);
    response.unenforced.CopyToParamSet(&characteristics->sw_enforced);

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::import_key(
    const keymaster1_device_t* dev, const keymaster_key_param_set_t* params,
    keymaster_key_format_t key_format, const keymaster_blob_t* key_data,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t** characteristics) {
    if (!params || !key_data)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SoftKeymasterDevice* sk_dev = convert_device(dev);

    ImportKeyRequest request;
    request.key_description.Reinitialize(*params);

    keymaster1_device_t* km1_dev = sk_dev->wrapped_km1_device_;
    if (km1_dev && !sk_dev->KeyRequiresSoftwareDigesting(request.key_description))
        return km1_dev->import_key(km1_dev, params, key_format, key_data, key_blob,
                                   characteristics);

    if (characteristics)
        *characteristics = nullptr;

    request.key_format = key_format;
    request.SetKeyMaterial(key_data->data, key_data->data_length);

    ImportKeyResponse response;
    convert_device(dev)->impl_->ImportKey(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    key_blob->key_material_size = response.key_blob.key_material_size;
    key_blob->key_material = reinterpret_cast<uint8_t*>(malloc(key_blob->key_material_size));
    if (!key_blob->key_material)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(const_cast<uint8_t*>(key_blob->key_material), response.key_blob.key_material,
           response.key_blob.key_material_size);

    if (characteristics) {
        *characteristics = BuildCharacteristics(response.enforced, response.unenforced);
        if (!*characteristics)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::import_key(
    const keymaster2_device_t* dev, const keymaster_key_param_set_t* params,
    keymaster_key_format_t key_format, const keymaster_blob_t* key_data,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t* characteristics) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    SoftKeymasterDevice* sk_dev = convert_device(dev);

    keymaster_error_t error;
    if (characteristics) {
        keymaster_key_characteristics_t* characteristics_ptr;
        error = import_key(&sk_dev->km1_device_, params, key_format, key_data, key_blob,
                           &characteristics_ptr);
        if (error == KM_ERROR_OK) {
            *characteristics = *characteristics_ptr;
            free(characteristics_ptr);
        }
    } else {
        error = import_key(&sk_dev->km1_device_, params, key_format, key_data, key_blob, nullptr);
    }

    return error;
}

/* static */
keymaster_error_t SoftKeymasterDevice::export_key(const keymaster1_device_t* dev,
                                                  keymaster_key_format_t export_format,
                                                  const keymaster_key_blob_t* key_to_export,
                                                  const keymaster_blob_t* client_id,
                                                  const keymaster_blob_t* app_data,
                                                  keymaster_blob_t* export_data) {
    if (!key_to_export || !key_to_export->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!export_data)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev)
        return km1_dev->export_key(km1_dev, export_format, key_to_export, client_id, app_data,
                                   export_data);

    export_data->data = nullptr;
    export_data->data_length = 0;

    ExportKeyRequest request;
    request.key_format = export_format;
    request.SetKeyMaterial(*key_to_export);
    AddClientAndAppData(client_id, app_data, &request);

    ExportKeyResponse response;
    convert_device(dev)->impl_->ExportKey(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    export_data->data_length = response.key_data_length;
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(export_data->data_length));
    if (!tmp)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(tmp, response.key_data, export_data->data_length);
    export_data->data = tmp;
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::export_key(const keymaster2_device_t* dev,
                                                  keymaster_key_format_t export_format,
                                                  const keymaster_key_blob_t* key_to_export,
                                                  const keymaster_blob_t* client_id,
                                                  const keymaster_blob_t* app_data,
                                                  keymaster_blob_t* export_data) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    SoftKeymasterDevice* sk_dev = convert_device(dev);
    return export_key(&sk_dev->km1_device_, export_format, key_to_export, client_id, app_data,
                      export_data);
}

/* static */
keymaster_error_t SoftKeymasterDevice::attest_key(const keymaster2_device_t* dev,
                                                  const keymaster_key_blob_t* key_to_attest,
                                                  const keymaster_key_param_set_t* attest_params,
                                                  keymaster_cert_chain_t* cert_chain) {
    if (!dev || !key_to_attest || !attest_params || !cert_chain)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    cert_chain->entry_count = 0;
    cert_chain->entries = nullptr;

    AttestKeyRequest request;
    request.SetKeyMaterial(*key_to_attest);
    request.attest_params.Reinitialize(*attest_params);

    keymaster_blob_t attestation_challenge = {};
    request.attest_params.GetTagValue(TAG_ATTESTATION_CHALLENGE, &attestation_challenge);
    if (attestation_challenge.data_length > kMaximumAttestationChallengeLength) {
        LOG_E("%d-byte attestation challenge; only %d bytes allowed",
              attestation_challenge.data_length, kMaximumAttestationChallengeLength);
        return KM_ERROR_INVALID_INPUT_LENGTH;
    }

    AttestKeyResponse response;
    convert_device(dev)->impl_->AttestKey(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    // Allocate and clear storage for cert_chain.
    keymaster_cert_chain_t& rsp_chain = response.certificate_chain;
    cert_chain->entries = reinterpret_cast<keymaster_blob_t*>(
        malloc(rsp_chain.entry_count * sizeof(*cert_chain->entries)));
    if (!cert_chain->entries)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    cert_chain->entry_count = rsp_chain.entry_count;
    for (keymaster_blob_t& entry : array_range(cert_chain->entries, cert_chain->entry_count))
        entry = {};

    // Copy cert_chain contents
    size_t i = 0;
    for (keymaster_blob_t& entry : array_range(rsp_chain.entries, rsp_chain.entry_count)) {
        cert_chain->entries[i].data = reinterpret_cast<uint8_t*>(malloc(entry.data_length));
        if (!cert_chain->entries[i].data) {
            keymaster_free_cert_chain(cert_chain);
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        }
        cert_chain->entries[i].data_length = entry.data_length;
        memcpy(const_cast<uint8_t*>(cert_chain->entries[i].data), entry.data, entry.data_length);
        ++i;
    }

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::upgrade_key(const keymaster2_device_t* dev,
                                                   const keymaster_key_blob_t* key_to_upgrade,
                                                   const keymaster_key_param_set_t* upgrade_params,
                                                   keymaster_key_blob_t* upgraded_key) {
    if (!dev || !key_to_upgrade || !upgrade_params)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!upgraded_key)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    UpgradeKeyRequest request;
    request.SetKeyMaterial(*key_to_upgrade);
    request.upgrade_params.Reinitialize(*upgrade_params);

    UpgradeKeyResponse response;
    convert_device(dev)->impl_->UpgradeKey(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    upgraded_key->key_material_size = response.upgraded_key.key_material_size;
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(upgraded_key->key_material_size));
    if (!tmp)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(tmp, response.upgraded_key.key_material, response.upgraded_key.key_material_size);
    upgraded_key->key_material = tmp;

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::delete_key(const keymaster1_device_t* dev,
                                                  const keymaster_key_blob_t* key) {
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    KeymasterKeyBlob blob(*key);
    return convert_device(dev)->context_->DeleteKey(blob);
}

/* static */
keymaster_error_t SoftKeymasterDevice::delete_key(const keymaster2_device_t* dev,
                                                  const keymaster_key_blob_t* key) {
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    KeymasterKeyBlob blob(*key);
    return convert_device(dev)->context_->DeleteKey(blob);
}

/* static */
keymaster_error_t SoftKeymasterDevice::delete_all_keys(const keymaster1_device_t* dev) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    return convert_device(dev)->context_->DeleteAllKeys();
}

/* static */
keymaster_error_t SoftKeymasterDevice::delete_all_keys(const keymaster2_device_t* dev) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    return convert_device(dev)->context_->DeleteAllKeys();
}

/* static */
keymaster_error_t SoftKeymasterDevice::begin(const keymaster1_device_t* dev,
                                             keymaster_purpose_t purpose,
                                             const keymaster_key_blob_t* key,
                                             const keymaster_key_param_set_t* in_params,
                                             keymaster_key_param_set_t* out_params,
                                             keymaster_operation_handle_t* operation_handle) {
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!operation_handle)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SoftKeymasterDevice* skdev = convert_device(dev);
    const keymaster1_device_t* km1_dev = skdev->wrapped_km1_device_;

    if (km1_dev) {
        AuthorizationSet in_params_set(*in_params);

        KeymasterKeyBlob key_material;
        AuthorizationSet hw_enforced;
        AuthorizationSet sw_enforced;
        skdev->context_->ParseKeyBlob(KeymasterKeyBlob(*key), in_params_set, &key_material,
                                      &hw_enforced, &sw_enforced);

        keymaster_algorithm_t algorithm = KM_ALGORITHM_AES;
        if (!hw_enforced.GetTagValue(TAG_ALGORITHM, &algorithm) &&
            !sw_enforced.GetTagValue(TAG_ALGORITHM, &algorithm)) {
            return KM_ERROR_INVALID_KEY_BLOB;
        }

        if (algorithm == KM_ALGORITHM_HMAC) {
            // Because HMAC keys can have only one digest, in_params_set doesn't contain it.  We
            // need to get the digest from the key and add it to in_params_set.
            keymaster_digest_t digest;
            if (!hw_enforced.GetTagValue(TAG_DIGEST, &digest) &&
                !sw_enforced.GetTagValue(TAG_DIGEST, &digest)) {
                return KM_ERROR_INVALID_KEY_BLOB;
            }
            in_params_set.push_back(TAG_DIGEST, digest);
        }

        if (!skdev->RequiresSoftwareDigesting(algorithm, purpose, in_params_set)) {
            LOG_D("Operation supported by %s, passing through to keymaster1 module",
                  km1_dev->common.module->name);
            return km1_dev->begin(km1_dev, purpose, key, in_params, out_params, operation_handle);
        }
        LOG_I("Doing software digesting for keymaster1 module %s", km1_dev->common.module->name);
    }

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }

    BeginOperationRequest request;
    request.purpose = purpose;
    request.SetKeyMaterial(*key);
    request.additional_params.Reinitialize(*in_params);

    BeginOperationResponse response;
    skdev->impl_->BeginOperation(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    if (response.output_params.size() > 0) {
        if (out_params)
            response.output_params.CopyToParamSet(out_params);
        else
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    *operation_handle = response.op_handle;
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::begin(const keymaster2_device_t* dev,
                                             keymaster_purpose_t purpose,
                                             const keymaster_key_blob_t* key,
                                             const keymaster_key_param_set_t* in_params,
                                             keymaster_key_param_set_t* out_params,
                                             keymaster_operation_handle_t* operation_handle) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    SoftKeymasterDevice* sk_dev = convert_device(dev);
    return begin(&sk_dev->km1_device_, purpose, key, in_params, out_params, operation_handle);
}

/* static */
keymaster_error_t SoftKeymasterDevice::update(const keymaster1_device_t* dev,
                                              keymaster_operation_handle_t operation_handle,
                                              const keymaster_key_param_set_t* in_params,
                                              const keymaster_blob_t* input, size_t* input_consumed,
                                              keymaster_key_param_set_t* out_params,
                                              keymaster_blob_t* output) {
    if (!input)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!input_consumed)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev && !convert_device(dev)->impl_->has_operation(operation_handle)) {
        // This operation is being handled by km1_dev (or doesn't exist).  Pass it through to
        // km1_dev.  Otherwise, we'll use the software AndroidKeymaster, which may delegate to
        // km1_dev after doing necessary digesting.
        return km1_dev->update(km1_dev, operation_handle, in_params, input, input_consumed,
                               out_params, output);
    }

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }
    if (output) {
        output->data = nullptr;
        output->data_length = 0;
    }

    UpdateOperationRequest request;
    request.op_handle = operation_handle;
    if (input)
        request.input.Reinitialize(input->data, input->data_length);
    if (in_params)
        request.additional_params.Reinitialize(*in_params);

    UpdateOperationResponse response;
    convert_device(dev)->impl_->UpdateOperation(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    if (response.output_params.size() > 0) {
        if (out_params)
            response.output_params.CopyToParamSet(out_params);
        else
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    *input_consumed = response.input_consumed;
    if (output) {
        output->data_length = response.output.available_read();
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(output->data_length));
        if (!tmp)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        memcpy(tmp, response.output.peek_read(), output->data_length);
        output->data = tmp;
    } else if (response.output.available_read() > 0) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::update(const keymaster2_device_t* dev,
                                              keymaster_operation_handle_t operation_handle,
                                              const keymaster_key_param_set_t* in_params,
                                              const keymaster_blob_t* input, size_t* input_consumed,
                                              keymaster_key_param_set_t* out_params,
                                              keymaster_blob_t* output) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    SoftKeymasterDevice* sk_dev = convert_device(dev);
    return update(&sk_dev->km1_device_, operation_handle, in_params, input, input_consumed,
                  out_params, output);
}

/* static */
keymaster_error_t SoftKeymasterDevice::finish(const keymaster1_device_t* dev,
                                              keymaster_operation_handle_t operation_handle,
                                              const keymaster_key_param_set_t* params,
                                              const keymaster_blob_t* signature,
                                              keymaster_key_param_set_t* out_params,
                                              keymaster_blob_t* output) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev && !convert_device(dev)->impl_->has_operation(operation_handle)) {
        // This operation is being handled by km1_dev (or doesn't exist).  Pass it through to
        // km1_dev.  Otherwise, we'll use the software AndroidKeymaster, which may delegate to
        // km1_dev after doing necessary digesting.
        return km1_dev->finish(km1_dev, operation_handle, params, signature, out_params, output);
    }

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }

    if (output) {
        output->data = nullptr;
        output->data_length = 0;
    }

    FinishOperationRequest request;
    request.op_handle = operation_handle;
    if (signature && signature->data_length > 0)
        request.signature.Reinitialize(signature->data, signature->data_length);
    request.additional_params.Reinitialize(*params);

    FinishOperationResponse response;
    convert_device(dev)->impl_->FinishOperation(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    if (response.output_params.size() > 0) {
        if (out_params)
            response.output_params.CopyToParamSet(out_params);
        else
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }
    if (output) {
        output->data_length = response.output.available_read();
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(output->data_length));
        if (!tmp)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        memcpy(tmp, response.output.peek_read(), output->data_length);
        output->data = tmp;
    } else if (response.output.available_read() > 0) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    return KM_ERROR_OK;
}

struct KeyParamSetContents_Delete {
    void operator()(keymaster_key_param_set_t* p) { keymaster_free_param_set(p); }
};

/* static */
keymaster_error_t SoftKeymasterDevice::finish(const keymaster2_device_t* dev,
                                              keymaster_operation_handle_t operation_handle,
                                              const keymaster_key_param_set_t* params,
                                              const keymaster_blob_t* input,
                                              const keymaster_blob_t* signature,
                                              keymaster_key_param_set_t* out_params,
                                              keymaster_blob_t* output) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (out_params)
        *out_params = {};

    if (output)
        *output = {};

    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev && !convert_device(dev)->impl_->has_operation(operation_handle)) {
        // This operation is being handled by km1_dev (or doesn't exist).  Pass it through to
        // km1_dev.  Otherwise, we'll use the software AndroidKeymaster, which may delegate to
        // km1_dev after doing necessary digesting.

        std::vector<uint8_t> accumulated_output;
        AuthorizationSet accumulated_out_params;
        AuthorizationSet mutable_params(*params);
        if (input && input->data && input->data_length) {
            // Keymaster1 doesn't support input to finish().  Call update() to process input.

            accumulated_output.reserve(input->data_length);  // Guess at output size
            keymaster_blob_t mutable_input = *input;

            while (mutable_input.data_length > 0) {
                keymaster_key_param_set_t update_out_params = {};
                keymaster_blob_t update_output = {};
                size_t input_consumed = 0;
                keymaster_error_t error =
                    km1_dev->update(km1_dev, operation_handle, &mutable_params, &mutable_input,
                                    &input_consumed, &update_out_params, &update_output);
                if (error != KM_ERROR_OK) {
                    return error;
                }

                accumulated_output.reserve(accumulated_output.size() + update_output.data_length);
                std::copy(update_output.data, update_output.data + update_output.data_length,
                          std::back_inserter(accumulated_output));
                free(const_cast<uint8_t*>(update_output.data));

                accumulated_out_params.push_back(update_out_params);
                keymaster_free_param_set(&update_out_params);

                mutable_input.data += input_consumed;
                mutable_input.data_length -= input_consumed;

                // AAD should only be sent once, so remove it if present.
                int aad_pos = mutable_params.find(TAG_ASSOCIATED_DATA);
                if (aad_pos != -1) {
                    mutable_params.erase(aad_pos);
                }

                if (input_consumed == 0) {
                    // Apparently we need more input than we have to complete an operation.
                    km1_dev->abort(km1_dev, operation_handle);
                    return KM_ERROR_INVALID_INPUT_LENGTH;
                }
            }
        }

        keymaster_key_param_set_t finish_out_params = {};
        keymaster_blob_t finish_output = {};
        keymaster_error_t error = km1_dev->finish(km1_dev, operation_handle, &mutable_params,
                                                  signature, &finish_out_params, &finish_output);
        if (error != KM_ERROR_OK) {
            return error;
        }

        if (!accumulated_out_params.empty()) {
            accumulated_out_params.push_back(finish_out_params);
            keymaster_free_param_set(&finish_out_params);
            accumulated_out_params.Deduplicate();
            accumulated_out_params.CopyToParamSet(&finish_out_params);
        }
        std::unique_ptr<keymaster_key_param_set_t, KeyParamSetContents_Delete>
            finish_out_params_deleter(&finish_out_params);

        if (!accumulated_output.empty()) {
            size_t finish_out_length = accumulated_output.size() + finish_output.data_length;
            uint8_t* finish_out_buf = reinterpret_cast<uint8_t*>(malloc(finish_out_length));

            std::copy(accumulated_output.begin(), accumulated_output.end(), finish_out_buf);
            std::copy(finish_output.data, finish_output.data + finish_output.data_length,
                      finish_out_buf + accumulated_output.size());

            free(const_cast<uint8_t*>(finish_output.data));
            finish_output.data_length = finish_out_length;
            finish_output.data = finish_out_buf;
        }
        std::unique_ptr<uint8_t, Malloc_Delete> finish_output_deleter(
            const_cast<uint8_t*>(finish_output.data));

        if ((!out_params && finish_out_params.length) || (!output && finish_output.data_length)) {
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
        }

        if (out_params) {
            *out_params = finish_out_params;
        }

        if (output) {
            *output = finish_output;
        }

        finish_out_params_deleter.release();
        finish_output_deleter.release();

        return KM_ERROR_OK;
    }

    FinishOperationRequest request;
    request.op_handle = operation_handle;
    if (signature && signature->data_length > 0)
        request.signature.Reinitialize(signature->data, signature->data_length);
    if (input && input->data_length > 0)
        request.input.Reinitialize(input->data, input->data_length);
    request.additional_params.Reinitialize(*params);

    FinishOperationResponse response;
    convert_device(dev)->impl_->FinishOperation(request, &response);
    if (response.error != KM_ERROR_OK)
        return response.error;

    if (response.output_params.size() > 0) {
        if (out_params)
            response.output_params.CopyToParamSet(out_params);
        else
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }
    if (output) {
        output->data_length = response.output.available_read();
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(output->data_length));
        if (!tmp)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        memcpy(tmp, response.output.peek_read(), output->data_length);
        output->data = tmp;
    } else if (response.output.available_read() > 0) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t SoftKeymasterDevice::abort(const keymaster1_device_t* dev,
                                             keymaster_operation_handle_t operation_handle) {
    const keymaster1_device_t* km1_dev = convert_device(dev)->wrapped_km1_device_;
    if (km1_dev && !convert_device(dev)->impl_->has_operation(operation_handle)) {
        // This operation is being handled by km1_dev (or doesn't exist).  Pass it through to
        // km1_dev.  Otherwise, we'll use the software AndroidKeymaster, which may delegate to
        // km1_dev.
        return km1_dev->abort(km1_dev, operation_handle);
    }

    AbortOperationRequest request;
    request.op_handle = operation_handle;
    AbortOperationResponse response;
    convert_device(dev)->impl_->AbortOperation(request, &response);
    return response.error;
}

/* static */
keymaster_error_t SoftKeymasterDevice::abort(const keymaster2_device_t* dev,
                                             keymaster_operation_handle_t operation_handle) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!convert_device(dev)->configured())
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    SoftKeymasterDevice* sk_dev = convert_device(dev);
    return abort(&sk_dev->km1_device_, operation_handle);
}

/* static */
void SoftKeymasterDevice::StoreDefaultNewKeyParams(keymaster_algorithm_t algorithm,
                                                   AuthorizationSet* auth_set) {
    auth_set->push_back(TAG_PURPOSE, KM_PURPOSE_SIGN);
    auth_set->push_back(TAG_PURPOSE, KM_PURPOSE_VERIFY);
    auth_set->push_back(TAG_ALL_USERS);
    auth_set->push_back(TAG_NO_AUTH_REQUIRED);

    // All digests.
    auth_set->push_back(TAG_DIGEST, KM_DIGEST_NONE);
    auth_set->push_back(TAG_DIGEST, KM_DIGEST_MD5);
    auth_set->push_back(TAG_DIGEST, KM_DIGEST_SHA1);
    auth_set->push_back(TAG_DIGEST, KM_DIGEST_SHA_2_224);
    auth_set->push_back(TAG_DIGEST, KM_DIGEST_SHA_2_256);
    auth_set->push_back(TAG_DIGEST, KM_DIGEST_SHA_2_384);
    auth_set->push_back(TAG_DIGEST, KM_DIGEST_SHA_2_512);

    if (algorithm == KM_ALGORITHM_RSA) {
        auth_set->push_back(TAG_PURPOSE, KM_PURPOSE_ENCRYPT);
        auth_set->push_back(TAG_PURPOSE, KM_PURPOSE_DECRYPT);
        auth_set->push_back(TAG_PADDING, KM_PAD_NONE);
        auth_set->push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_SIGN);
        auth_set->push_back(TAG_PADDING, KM_PAD_RSA_PKCS1_1_5_ENCRYPT);
        auth_set->push_back(TAG_PADDING, KM_PAD_RSA_PSS);
        auth_set->push_back(TAG_PADDING, KM_PAD_RSA_OAEP);
    }
}

}  // namespace keymaster
