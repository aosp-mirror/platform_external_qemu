/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef SYSTEM_KEYMASTER_ATTESTATION_RECORD_H_
#define SYSTEM_KEYMASTER_ATTESTATION_RECORD_H_

#include <hardware/keymaster_defs.h>

#include <keymaster/authorization_set.h>

namespace keymaster {

class KeymasterContext;

/**
 * The OID for Android attestation records.  For the curious, it breaks down as follows:
 *
 * 1 = ISO
 * 3 = org
 * 6 = DoD (Huh? OIDs are weird.)
 * 1 = IANA
 * 4 = Private
 * 1 = Enterprises
 * 11129 = Google
 * 2 = Google security
 * 1 = certificate extension
 * 17 = Android attestation extension.
 */
static const char kAttestionRecordOid[] = "1.3.6.1.4.1.11129.2.1.17";

keymaster_error_t build_attestation_record(const AuthorizationSet& attestation_params,
                                           const AuthorizationSet& software_enforced,
                                           const AuthorizationSet& tee_enforced,
                                           const KeymasterContext& context,
                                           UniquePtr<uint8_t[]>* asn1_key_desc,
                                           size_t* asn1_key_desc_len);

keymaster_error_t parse_attestation_record(const uint8_t* asn1_key_desc, size_t asn1_key_desc_len,
                                           uint32_t* attestation_version,  //
                                           keymaster_security_level_t* attestation_security_level,
                                           uint32_t* keymaster_version,
                                           keymaster_security_level_t* keymaster_security_level,
                                           keymaster_blob_t* attestation_challenge,
                                           AuthorizationSet* software_enforced,
                                           AuthorizationSet* tee_enforced,
                                           keymaster_blob_t* unique_id);
}

#endif  // SYSTEM_KEYMASTER_ATTESTATION_RECORD_H_
