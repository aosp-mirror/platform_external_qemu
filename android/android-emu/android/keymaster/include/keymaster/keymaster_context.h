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

#ifndef SYSTEM_KEYMASTER_KEYMASTER_CONTEXT_H_
#define SYSTEM_KEYMASTER_KEYMASTER_CONTEXT_H_

#include <assert.h>

#include <openssl/evp.h>

#include <hardware/keymaster_defs.h>
#include <keymaster/keymaster_enforcement.h>

namespace keymaster {

class AuthorizationSet;
class KeyFactory;
class OperationFactory;
struct KeymasterKeyBlob;

/**
 * KeymasterContext provides a singleton abstract interface that encapsulates various
 * environment-dependent elements of AndroidKeymaster.
 *
 * AndroidKeymaster runs in multiple contexts.  Primarily:
 *
 * - In a trusted execution environment (TEE) as a "secure hardware" implementation.  In this
 *   context keys are wrapped with an master key that never leaves the TEE, TEE-specific routines
 *   are used for random number generation, all AndroidKeymaster-enforced authorizations are
 *   considered hardware-enforced, and there's a bootloader-provided root of trust.
 *
 * - In the non-secure world as a software-only implementation.  In this context keys are not
 *   encrypted (though they are integrity-checked) because there is no place to securely store a
 *   key, OpenSSL is used for random number generation, no AndroidKeymaster-enforced authorizations
 *   are considered hardware enforced and the root of trust is a static string.
 *
 * - In the non-secure world as a hybrid implementation fronting a less-capable hardware
 *   implementation.  For example, a keymaster0 hardware implementation.  In this context keys are
 *   not encrypted by AndroidKeymaster, but some may be opaque blobs provided by the backing
 *   hardware, but blobs that lack the extended authorization lists of keymaster1.  In addition,
 *   keymaster0 lacks many features of keymaster1, including modes of operation related to the
 *   backing keymaster0 keys.  AndroidKeymaster must extend the blobs to add authorization lists,
 *   and must provide the missing operation mode implementations in software, which means that
 *   authorization lists are partially hardware-enforced (the bits that are enforced by the
 *   underlying keymaster0) and partially software-enforced (the rest). OpenSSL is used for number
 *   generation and the root of trust is a static string.
 *
 * More contexts are possible.
 */
class KeymasterContext {
  public:
    KeymasterContext() {}
    virtual ~KeymasterContext(){};

    /**
     * Returns the security level (SW or TEE) of this keymaster implementation.
     */
    virtual keymaster_security_level_t GetSecurityLevel() const = 0;

    /**
     * Sets the system version as reported by the system *itself*.  This is used to verify that the
     * system believes itself to be running the same version that is reported by the bootloader, in
     * hardware implementations.  For SoftKeymasterDevice, this sets the version information used.
     *
     * If the specified values don't match the bootloader-provided values, this method must return
     * KM_ERROR_INVALID_ARGUMENT;
     */
    virtual keymaster_error_t SetSystemVersion(uint32_t os_version, uint32_t os_patchlevel) = 0;

    /**
     * Returns the system version.  For hardware-based implementations this will be the value
     * reported by the bootloader.  For SoftKeymasterDevice it will be the verion information set by
     * SetSystemVersion above.
     */
    virtual void GetSystemVersion(uint32_t* os_version, uint32_t* os_patchlevel) const = 0;

    virtual KeyFactory* GetKeyFactory(keymaster_algorithm_t algorithm) const = 0;
    virtual OperationFactory* GetOperationFactory(keymaster_algorithm_t algorithm,
                                                  keymaster_purpose_t purpose) const = 0;
    virtual keymaster_algorithm_t* GetSupportedAlgorithms(size_t* algorithms_count) const = 0;

    /**
     * CreateKeyBlob takes authorization sets and key material and produces a key blob and hardware
     * and software authorization lists ready to be returned to the AndroidKeymaster client
     * (Keystore, generally).  The blob is integrity-checked and may be encrypted, depending on the
     * needs of the context.
    *
     * This method is generally called only by KeyFactory subclassses.
     */
    virtual keymaster_error_t CreateKeyBlob(const AuthorizationSet& key_description,
                                            keymaster_key_origin_t origin,
                                            const KeymasterKeyBlob& key_material,
                                            KeymasterKeyBlob* blob, AuthorizationSet* hw_enforced,
                                            AuthorizationSet* sw_enforced) const = 0;

    /**
     * UpgradeKeyBlob takes an existing blob, parses out key material and constructs a new blob with
     * the current format and OS version info.
     */
    virtual keymaster_error_t UpgradeKeyBlob(const KeymasterKeyBlob& key_to_upgrade,
                                             const AuthorizationSet& upgrade_params,
                                             KeymasterKeyBlob* upgraded_key) const = 0;

    /**
     * ParseKeyBlob takes a blob and extracts authorization sets and key material, returning an
     * error if the blob fails integrity checking or decryption.  Note that the returned key
     * material may itself be an opaque blob usable only by secure hardware (in the hybrid case).
     *
     * This method is called by AndroidKeymaster.
     */
    virtual keymaster_error_t ParseKeyBlob(const KeymasterKeyBlob& blob,
                                           const AuthorizationSet& additional_params,
                                           KeymasterKeyBlob* key_material,
                                           AuthorizationSet* hw_enforced,
                                           AuthorizationSet* sw_enforced) const = 0;

    /**
     * Take whatever environment-specific action is appropriate (if any) to delete the specified
     * key.
     */
    virtual keymaster_error_t DeleteKey(const KeymasterKeyBlob& /* blob */) const {
        return KM_ERROR_OK;
    }

    /**
     * Take whatever environment-specific action is appropriate to delete all keys.
     */
    virtual keymaster_error_t DeleteAllKeys() const { return KM_ERROR_OK; }

    /**
     * Adds entropy to the Cryptographic Pseudo Random Number Generator used to generate key
     * material, and other cryptographic protocol elements.  Note that if the underlying CPRNG
     * tracks the size of its entropy pool, it should not assume that the provided data contributes
     * any entropy, and it should also ensure that data provided through this interface cannot
     * "poison" the CPRNG outputs, making them predictable.
     */
    virtual keymaster_error_t AddRngEntropy(const uint8_t* buf, size_t length) const = 0;

    /**
     * Generates \p length random bytes, placing them in \p buf.
     */
    virtual keymaster_error_t GenerateRandom(uint8_t* buf, size_t length) const = 0;

    /**
     * Return the enforcement policy for this context, or null if no enforcement should be done.
     */
    virtual KeymasterEnforcement* enforcement_policy() = 0;

    /**
     * Return the attestation signing key of the specified algorithm (KM_ALGORITHM_RSA or
     * KM_ALGORITHM_EC).  Caller acquires ownership and should free using EVP_PKEY_free.
     */
    virtual EVP_PKEY* AttestationKey(keymaster_algorithm_t algorithm,
                                     keymaster_error_t* error) const = 0;

    /**
     * Return the certificate chain of the attestation signing key of the specified algorithm
     * (KM_ALGORITHM_RSA or KM_ALGORITHM_EC).  Caller acquires ownership and should free.
     */
    virtual keymaster_cert_chain_t* AttestationChain(keymaster_algorithm_t algorithm,
                                                     keymaster_error_t* error) const = 0;

    /**
     * Generate the current unique ID.
     */
    virtual keymaster_error_t GenerateUniqueId(uint64_t creation_date_time,
                                               const keymaster_blob_t& application_id,
                                               bool reset_since_rotation,
                                               Buffer* unique_id) const = 0;

  private:
    // Uncopyable.
    KeymasterContext(const KeymasterContext&);
    void operator=(const KeymasterContext&);
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEYMASTER_CONTEXT_H_
