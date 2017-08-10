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

#ifndef SYSTEM_KEYMASTER_SOFT_KEYMASTER_CONTEXT_H_
#define SYSTEM_KEYMASTER_SOFT_KEYMASTER_CONTEXT_H_

#include <memory>
#include <string>

#include <openssl/evp.h>

#include <hardware/keymaster0.h>
#include <hardware/keymaster1.h>
#include <keymaster/keymaster_context.h>

namespace keymaster {

class SoftKeymasterKeyRegistrations;
class Keymaster0Engine;
class Keymaster1Engine;

/**
 * SoftKeymasterContext provides the context for a non-secure implementation of AndroidKeymaster.
 */
class SoftKeymasterContext : public KeymasterContext {
  public:
    explicit SoftKeymasterContext(const std::string& root_of_trust = "SW");
    ~SoftKeymasterContext() override;

    /**
     * Use the specified HW keymaster0 device for the operations it supports.  Takes ownership of
     * the specified device (will call keymaster0_device->common.close());
     */
    keymaster_error_t SetHardwareDevice(keymaster0_device_t* keymaster0_device);

    /**
     * Use the specified HW keymaster1 device for performing undigested RSA and EC operations after
     * digesting has been done in software.  Takes ownership of the specified device (will call
     * keymaster1_device->common.close());
     */
    keymaster_error_t SetHardwareDevice(keymaster1_device_t* keymaster1_device);

    keymaster_security_level_t GetSecurityLevel() const override {
        return KM_SECURITY_LEVEL_SOFTWARE;
    }

    keymaster_error_t SetSystemVersion(uint32_t os_version, uint32_t os_patchlevel) override;
    void GetSystemVersion(uint32_t* os_version, uint32_t* os_patchlevel) const override;

    KeyFactory* GetKeyFactory(keymaster_algorithm_t algorithm) const override;
    OperationFactory* GetOperationFactory(keymaster_algorithm_t algorithm,
                                          keymaster_purpose_t purpose) const override;
    keymaster_algorithm_t* GetSupportedAlgorithms(size_t* algorithms_count) const override;
    keymaster_error_t CreateKeyBlob(const AuthorizationSet& auths, keymaster_key_origin_t origin,
                                    const KeymasterKeyBlob& key_material, KeymasterKeyBlob* blob,
                                    AuthorizationSet* hw_enforced,
                                    AuthorizationSet* sw_enforced) const override;
    keymaster_error_t UpgradeKeyBlob(const KeymasterKeyBlob& key_to_upgrade,
                                     const AuthorizationSet& upgrade_params,
                                     KeymasterKeyBlob* upgraded_key) const override;
    keymaster_error_t ParseKeyBlob(const KeymasterKeyBlob& blob,
                                   const AuthorizationSet& additional_params,
                                   KeymasterKeyBlob* key_material, AuthorizationSet* hw_enforced,
                                   AuthorizationSet* sw_enforced) const override;
    keymaster_error_t DeleteKey(const KeymasterKeyBlob& blob) const override;
    keymaster_error_t DeleteAllKeys() const override;
    keymaster_error_t AddRngEntropy(const uint8_t* buf, size_t length) const override;
    keymaster_error_t GenerateRandom(uint8_t* buf, size_t length) const override;

    EVP_PKEY* AttestationKey(keymaster_algorithm_t algorithm,
                             keymaster_error_t* error) const override;
    keymaster_cert_chain_t* AttestationChain(keymaster_algorithm_t algorithm,
                                             keymaster_error_t* error) const override;
    keymaster_error_t GenerateUniqueId(uint64_t creation_date_time,
                                       const keymaster_blob_t& application_id,
                                       bool reset_since_rotation, Buffer* unique_id) const override;

    KeymasterEnforcement* enforcement_policy() override {
        // SoftKeymaster does no enforcement; it's all done by Keystore.
        return nullptr;
    }

    void AddSystemVersionToSet(AuthorizationSet* auth_set) const;

  private:
    keymaster_error_t ParseOldSoftkeymasterBlob(const KeymasterKeyBlob& blob,
                                                KeymasterKeyBlob* key_material,
                                                AuthorizationSet* hw_enforced,
                                                AuthorizationSet* sw_enforced) const;
    keymaster_error_t ParseKeymaster1HwBlob(const KeymasterKeyBlob& blob,
                                            const AuthorizationSet& additional_params,
                                            KeymasterKeyBlob* key_material,
                                            AuthorizationSet* hw_enforced,
                                            AuthorizationSet* sw_enforced) const;
    keymaster_error_t ParseKeymaster0HwBlob(const KeymasterKeyBlob& blob,
                                            KeymasterKeyBlob* key_material,
                                            AuthorizationSet* hw_enforced,
                                            AuthorizationSet* sw_enforced) const;
    keymaster_error_t FakeKeyAuthorizations(EVP_PKEY* pubkey, AuthorizationSet* hw_enforced,
                                            AuthorizationSet* sw_enforced) const;
    keymaster_error_t BuildHiddenAuthorizations(const AuthorizationSet& input_set,
                                                AuthorizationSet* hidden) const;

    std::unique_ptr<Keymaster0Engine> km0_engine_;
    std::unique_ptr<Keymaster1Engine> km1_engine_;
    std::unique_ptr<KeyFactory> rsa_factory_;
    std::unique_ptr<KeyFactory> ec_factory_;
    std::unique_ptr<KeyFactory> aes_factory_;
    std::unique_ptr<KeyFactory> hmac_factory_;
    keymaster1_device* km1_dev_;
    const std::string root_of_trust_;
    uint32_t os_version_;
    uint32_t os_patchlevel_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_SOFT_KEYMASTER_CONTEXT_H_
