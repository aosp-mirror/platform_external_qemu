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

#ifndef SYSTEM_KEYMASTER_SOFT_KEYMASTER_DEVICE_H_
#define SYSTEM_KEYMASTER_SOFT_KEYMASTER_DEVICE_H_

#include <cstdlib>
#include <map>
#include <vector>

#include <hardware/keymaster0.h>
#include <hardware/keymaster1.h>
#include <hardware/keymaster2.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/soft_keymaster_context.h>

#include <UniquePtr.h>

namespace keymaster {

class AuthorizationSet;

/**
 * Keymaster1 device implementation.
 *
 * This is a hybrid software/hardware implementation which wraps a keymaster0_device_t, forwarding
 * RSA operations to secure hardware and doing everything else in software.
 *
 * IMPORTANT MAINTAINER NOTE: Pointers to instances of this class must be castable to hw_device_t
 * and keymaster_device. This means it must remain a standard layout class (no virtual functions and
 * no data members which aren't standard layout), and device_ must be the first data member.
 * Assertions in the constructor validate compliance with those constraints.
 */
class SoftKeymasterDevice {
  public:
    SoftKeymasterDevice();

    // Public only for testing.
    explicit SoftKeymasterDevice(SoftKeymasterContext* context);

    /**
     * Set SoftKeymasterDevice to wrap the speicified HW keymaster0 device.  Takes ownership of the
     * specified device (will call keymaster0_device->common.close());
     */
    keymaster_error_t SetHardwareDevice(keymaster0_device_t* keymaster0_device);

    /**
     * Set SoftKeymasterDevice to wrap specified HW keymaster1 device.  Takes ownership of the
     * specified device (will call keymaster1_device->common.close());
     */
    keymaster_error_t SetHardwareDevice(keymaster1_device_t* keymaster1_device);

    /**
     * Returns true if a keymaster1_device_t has been set as the hardware device, and if that
     * hardware device should be used directly.
     */
    bool Keymaster1DeviceIsGood();

    hw_device_t* hw_device();
    keymaster1_device_t* keymaster_device();
    keymaster2_device_t* keymaster2_device();

    // Public only for testing
    void GetVersion(const GetVersionRequest& req, GetVersionResponse* rsp) {
        impl_->GetVersion(req, rsp);
    }

    bool configured() const { return configured_; }

    typedef std::pair<keymaster_algorithm_t, keymaster_purpose_t> AlgPurposePair;
    typedef std::map<AlgPurposePair, std::vector<keymaster_digest_t>> DigestMap;

  private:
    void initialize_device_struct(uint32_t flags);
    bool FindUnsupportedDigest(keymaster_algorithm_t algorithm, keymaster_purpose_t purpose,
                               const AuthorizationSet& params,
                               keymaster_digest_t* unsupported) const;
    bool RequiresSoftwareDigesting(keymaster_algorithm_t algorithm, keymaster_purpose_t purpose,
                                   const AuthorizationSet& params) const;
    bool KeyRequiresSoftwareDigesting(const AuthorizationSet& key_description) const;

    static void StoreDefaultNewKeyParams(keymaster_algorithm_t algorithm,
                                         AuthorizationSet* auth_set);
    static keymaster_error_t GetPkcs8KeyAlgorithm(const uint8_t* key, size_t key_length,
                                                  keymaster_algorithm_t* algorithm);

    static int close_device(hw_device_t* dev);

    /*
     * These static methods are the functions referenced through the function pointers in
     * keymaster_device.
     */

    // Keymaster1 methods
    static keymaster_error_t get_supported_algorithms(const keymaster1_device_t* dev,
                                                      keymaster_algorithm_t** algorithms,
                                                      size_t* algorithms_length);
    static keymaster_error_t get_supported_block_modes(const keymaster1_device_t* dev,
                                                       keymaster_algorithm_t algorithm,
                                                       keymaster_purpose_t purpose,
                                                       keymaster_block_mode_t** modes,
                                                       size_t* modes_length);
    static keymaster_error_t get_supported_padding_modes(const keymaster1_device_t* dev,
                                                         keymaster_algorithm_t algorithm,
                                                         keymaster_purpose_t purpose,
                                                         keymaster_padding_t** modes,
                                                         size_t* modes_length);
    static keymaster_error_t get_supported_digests(const keymaster1_device_t* dev,
                                                   keymaster_algorithm_t algorithm,
                                                   keymaster_purpose_t purpose,
                                                   keymaster_digest_t** digests,
                                                   size_t* digests_length);
    static keymaster_error_t get_supported_import_formats(const keymaster1_device_t* dev,
                                                          keymaster_algorithm_t algorithm,
                                                          keymaster_key_format_t** formats,
                                                          size_t* formats_length);
    static keymaster_error_t get_supported_export_formats(const keymaster1_device_t* dev,
                                                          keymaster_algorithm_t algorithm,
                                                          keymaster_key_format_t** formats,
                                                          size_t* formats_length);
    static keymaster_error_t add_rng_entropy(const keymaster1_device_t* dev, const uint8_t* data,
                                             size_t data_length);
    static keymaster_error_t generate_key(const keymaster1_device_t* dev,
                                          const keymaster_key_param_set_t* params,
                                          keymaster_key_blob_t* key_blob,
                                          keymaster_key_characteristics_t** characteristics);
    static keymaster_error_t get_key_characteristics(const keymaster1_device_t* dev,
                                                     const keymaster_key_blob_t* key_blob,
                                                     const keymaster_blob_t* client_id,
                                                     const keymaster_blob_t* app_data,
                                                     keymaster_key_characteristics_t** character);
    static keymaster_error_t import_key(const keymaster1_device_t* dev,  //
                                        const keymaster_key_param_set_t* params,
                                        keymaster_key_format_t key_format,
                                        const keymaster_blob_t* key_data,
                                        keymaster_key_blob_t* key_blob,
                                        keymaster_key_characteristics_t** characteristics);
    static keymaster_error_t export_key(const keymaster1_device_t* dev,  //
                                        keymaster_key_format_t export_format,
                                        const keymaster_key_blob_t* key_to_export,
                                        const keymaster_blob_t* client_id,
                                        const keymaster_blob_t* app_data,
                                        keymaster_blob_t* export_data);
    static keymaster_error_t delete_key(const keymaster1_device_t* dev,
                                        const keymaster_key_blob_t* key);
    static keymaster_error_t delete_all_keys(const keymaster1_device_t* dev);
    static keymaster_error_t begin(const keymaster1_device_t* dev, keymaster_purpose_t purpose,
                                   const keymaster_key_blob_t* key,
                                   const keymaster_key_param_set_t* in_params,
                                   keymaster_key_param_set_t* out_params,
                                   keymaster_operation_handle_t* operation_handle);
    static keymaster_error_t update(const keymaster1_device_t* dev,  //
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* input, size_t* input_consumed,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output);
    static keymaster_error_t finish(const keymaster1_device_t* dev,  //
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* signature,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output);
    static keymaster_error_t abort(const keymaster1_device_t* dev,
                                   keymaster_operation_handle_t operation_handle);

    // Keymaster2 methods
    static keymaster_error_t configure(const keymaster2_device_t* dev,
                                       const keymaster_key_param_set_t* params);
    static keymaster_error_t add_rng_entropy(const keymaster2_device_t* dev, const uint8_t* data,
                                             size_t data_length);
    static keymaster_error_t generate_key(const keymaster2_device_t* dev,
                                          const keymaster_key_param_set_t* params,
                                          keymaster_key_blob_t* key_blob,
                                          keymaster_key_characteristics_t* characteristics);
    static keymaster_error_t get_key_characteristics(const keymaster2_device_t* dev,
                                                     const keymaster_key_blob_t* key_blob,
                                                     const keymaster_blob_t* client_id,
                                                     const keymaster_blob_t* app_data,
                                                     keymaster_key_characteristics_t* character);
    static keymaster_error_t import_key(const keymaster2_device_t* dev,  //
                                        const keymaster_key_param_set_t* params,
                                        keymaster_key_format_t key_format,
                                        const keymaster_blob_t* key_data,
                                        keymaster_key_blob_t* key_blob,
                                        keymaster_key_characteristics_t* characteristics);
    static keymaster_error_t export_key(const keymaster2_device_t* dev,  //
                                        keymaster_key_format_t export_format,
                                        const keymaster_key_blob_t* key_to_export,
                                        const keymaster_blob_t* client_id,
                                        const keymaster_blob_t* app_data,
                                        keymaster_blob_t* export_data);
    static keymaster_error_t attest_key(const keymaster2_device_t* dev,
                                        const keymaster_key_blob_t* key_to_attest,
                                        const keymaster_key_param_set_t* attest_params,
                                        keymaster_cert_chain_t* cert_chain);
    static keymaster_error_t upgrade_key(const keymaster2_device_t* dev,
                                         const keymaster_key_blob_t* key_to_upgrade,
                                         const keymaster_key_param_set_t* upgrade_params,
                                         keymaster_key_blob_t* upgraded_key);
    static keymaster_error_t delete_key(const keymaster2_device_t* dev,
                                        const keymaster_key_blob_t* key);
    static keymaster_error_t delete_all_keys(const keymaster2_device_t* dev);
    static keymaster_error_t begin(const keymaster2_device_t* dev, keymaster_purpose_t purpose,
                                   const keymaster_key_blob_t* key,
                                   const keymaster_key_param_set_t* in_params,
                                   keymaster_key_param_set_t* out_params,
                                   keymaster_operation_handle_t* operation_handle);
    static keymaster_error_t update(const keymaster2_device_t* dev,  //
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* input, size_t* input_consumed,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output);
    static keymaster_error_t finish(const keymaster2_device_t* dev,  //
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* input,
                                    const keymaster_blob_t* signature,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output);
    static keymaster_error_t abort(const keymaster2_device_t* dev,
                                   keymaster_operation_handle_t operation_handle);

    keymaster1_device_t km1_device_;
    keymaster2_device_t km2_device_;

    keymaster0_device_t* wrapped_km0_device_;
    keymaster1_device_t* wrapped_km1_device_;
    DigestMap km1_device_digests_;
    SoftKeymasterContext* context_;
    UniquePtr<AndroidKeymaster> impl_;
    std::string module_name_;
    hw_module_t updated_module_;
    bool configured_;
};

}  // namespace keymaster

#endif  // EXTERNAL_KEYMASTER_TRUSTY_KEYMASTER_DEVICE_H_
