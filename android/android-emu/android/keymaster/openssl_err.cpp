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

#include "openssl_err.h"

#include <openssl/err.h>
#include <openssl/evp.h>

#if defined(OPENSSL_IS_BORINGSSL)
#include <openssl/asn1.h>
#include <openssl/cipher.h>
#include <openssl/pkcs8.h>
#include <openssl/x509v3.h>
#endif

#include <hardware/keymaster_defs.h>
#include <keymaster/logger.h>

namespace keymaster {

static keymaster_error_t TranslateEvpError(int reason);
#if defined(OPENSSL_IS_BORINGSSL)
static keymaster_error_t TranslateASN1Error(int reason);
static keymaster_error_t TranslateCipherError(int reason);
static keymaster_error_t TranslatePKCS8Error(int reason);
static keymaster_error_t TranslateX509v3Error(int reason);
static keymaster_error_t TranslateRsaError(int reason);
#endif

keymaster_error_t TranslateLastOpenSslError(bool log_message) {
    unsigned long error = ERR_peek_last_error();

    if (log_message) {
        LOG_D("%s", ERR_error_string(error, NULL));
    }

    int reason = ERR_GET_REASON(error);

    /* Handle global error reasons */
    switch (reason) {
    case ERR_R_MALLOC_FAILURE:
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    case ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED:
    case ERR_R_PASSED_NULL_PARAMETER:
    case ERR_R_INTERNAL_ERROR:
    case ERR_R_OVERFLOW:
        return KM_ERROR_UNKNOWN_ERROR;
    default:
        break;
    }

    switch (ERR_GET_LIB(error)) {
    case ERR_LIB_USER:
        return static_cast<keymaster_error_t>(reason);
    case ERR_LIB_EVP:
        return TranslateEvpError(reason);
#if defined(OPENSSL_IS_BORINGSSL)
    case ERR_LIB_ASN1:
        return TranslateASN1Error(reason);
    case ERR_LIB_CIPHER:
        return TranslateCipherError(reason);
    case ERR_LIB_PKCS8:
        return TranslatePKCS8Error(reason);
    case ERR_LIB_X509V3:
        return TranslateX509v3Error(reason);
    case ERR_LIB_RSA:
        return TranslateRsaError(reason);
#else
    case ERR_LIB_ASN1:
        LOG_E("ASN.1 parsing error %d", reason);
        return KM_ERROR_INVALID_ARGUMENT;
#endif
    }

    LOG_E("Openssl error %d, %d", ERR_GET_LIB(error), reason);
    return KM_ERROR_UNKNOWN_ERROR;
}

#if defined(OPENSSL_IS_BORINGSSL)

keymaster_error_t TranslatePKCS8Error(int reason) {
    switch (reason) {
    case PKCS8_R_UNSUPPORTED_PRIVATE_KEY_ALGORITHM:
    case PKCS8_R_UNKNOWN_CIPHER:
        return KM_ERROR_UNSUPPORTED_ALGORITHM;

    case PKCS8_R_PRIVATE_KEY_ENCODE_ERROR:
    case PKCS8_R_PRIVATE_KEY_DECODE_ERROR:
        return KM_ERROR_INVALID_KEY_BLOB;

    case PKCS8_R_ENCODE_ERROR:
        return KM_ERROR_INVALID_ARGUMENT;

    default:
        return KM_ERROR_UNKNOWN_ERROR;
    }
}

keymaster_error_t TranslateCipherError(int reason) {
    switch (reason) {
    case CIPHER_R_DATA_NOT_MULTIPLE_OF_BLOCK_LENGTH:
    case CIPHER_R_WRONG_FINAL_BLOCK_LENGTH:
        return KM_ERROR_INVALID_INPUT_LENGTH;

    case CIPHER_R_UNSUPPORTED_KEY_SIZE:
    case CIPHER_R_BAD_KEY_LENGTH:
        return KM_ERROR_UNSUPPORTED_KEY_SIZE;

    case CIPHER_R_BAD_DECRYPT:
        return KM_ERROR_INVALID_ARGUMENT;

    case CIPHER_R_INVALID_KEY_LENGTH:
        return KM_ERROR_INVALID_KEY_BLOB;

    default:
        return KM_ERROR_UNKNOWN_ERROR;
    }
}

keymaster_error_t TranslateASN1Error(int reason) {
    switch (reason) {
#if !defined(OPENSSL_IS_BORINGSSL)
    case ASN1_R_UNSUPPORTED_CIPHER:
        return KM_ERROR_UNSUPPORTED_ALGORITHM;

    case ASN1_R_ERROR_LOADING_SECTION:
        return KM_ERROR_INVALID_KEY_BLOB;
#endif

    case ASN1_R_ENCODE_ERROR:
        return KM_ERROR_INVALID_ARGUMENT;

    default:
        return KM_ERROR_UNKNOWN_ERROR;
    }
}

keymaster_error_t TranslateX509v3Error(int reason) {
    switch (reason) {
    case X509V3_R_UNKNOWN_OPTION:
        return KM_ERROR_UNSUPPORTED_ALGORITHM;

    default:
        return KM_ERROR_UNKNOWN_ERROR;
    }
}

keymaster_error_t TranslateRsaError(int reason) {
    switch (reason) {
    case RSA_R_KEY_SIZE_TOO_SMALL:
        LOG_W("RSA key is too small to use with selected padding/digest", 0);
        return KM_ERROR_INCOMPATIBLE_PADDING_MODE;
    case RSA_R_DATA_TOO_LARGE_FOR_KEY_SIZE:
    case RSA_R_DATA_TOO_SMALL_FOR_KEY_SIZE:
        return KM_ERROR_INVALID_INPUT_LENGTH;
    case RSA_R_DATA_TOO_LARGE_FOR_MODULUS:
        return KM_ERROR_INVALID_ARGUMENT;
    default:
        return KM_ERROR_UNKNOWN_ERROR;
    };
}

#endif  // OPENSSL_IS_BORINGSSL

keymaster_error_t TranslateEvpError(int reason) {
    switch (reason) {

#if !defined(OPENSSL_IS_BORINGSSL)
    case EVP_R_UNKNOWN_DIGEST:
        return KM_ERROR_UNSUPPORTED_DIGEST;

    case EVP_R_UNSUPPORTED_PRF:
    case EVP_R_UNSUPPORTED_PRIVATE_KEY_ALGORITHM:
    case EVP_R_UNSUPPORTED_KEY_DERIVATION_FUNCTION:
    case EVP_R_UNSUPPORTED_SALT_TYPE:
    case EVP_R_UNKNOWN_PBE_ALGORITHM:
    case EVP_R_UNSUPORTED_NUMBER_OF_ROUNDS:
    case EVP_R_UNSUPPORTED_CIPHER:
    case EVP_R_PKCS8_UNKNOWN_BROKEN_TYPE:
    case EVP_R_UNKNOWN_CIPHER:
#endif
    case EVP_R_UNSUPPORTED_ALGORITHM:
    case EVP_R_OPERATON_NOT_INITIALIZED:
    case EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE:
        return KM_ERROR_UNSUPPORTED_ALGORITHM;

#if !defined(OPENSSL_IS_BORINGSSL)
    case EVP_R_DATA_NOT_MULTIPLE_OF_BLOCK_LENGTH:
    case EVP_R_WRONG_FINAL_BLOCK_LENGTH:
        return KM_ERROR_INVALID_INPUT_LENGTH;

    case EVP_R_UNSUPPORTED_KEYLENGTH:
    case EVP_R_BAD_KEY_LENGTH:
        return KM_ERROR_UNSUPPORTED_KEY_SIZE;
#endif

#if !defined(OPENSSL_IS_BORINGSSL)
    case EVP_R_BAD_BLOCK_LENGTH:
    case EVP_R_BN_DECODE_ERROR:
    case EVP_R_BN_PUBKEY_ERROR:
    case EVP_R_CIPHER_PARAMETER_ERROR:
    case EVP_R_ERROR_LOADING_SECTION:
    case EVP_R_EXPECTING_A_DH_KEY:
    case EVP_R_EXPECTING_A_ECDSA_KEY:
    case EVP_R_EXPECTING_A_EC_KEY:
    case EVP_R_INVALID_DIGEST:
    case EVP_R_INVALID_KEY_LENGTH:
    case EVP_R_NO_DSA_PARAMETERS:
    case EVP_R_PRIVATE_KEY_DECODE_ERROR:
    case EVP_R_PRIVATE_KEY_ENCODE_ERROR:
    case EVP_R_PUBLIC_KEY_NOT_RSA:
    case EVP_R_WRONG_PUBLIC_KEY_TYPE:
#endif
    case EVP_R_BUFFER_TOO_SMALL:
    case EVP_R_EXPECTING_AN_RSA_KEY:
    case EVP_R_EXPECTING_A_DSA_KEY:
    case EVP_R_MISSING_PARAMETERS:
        return KM_ERROR_INVALID_KEY_BLOB;

#if !defined(OPENSSL_IS_BORINGSSL)
    case EVP_R_BAD_DECRYPT:
    case EVP_R_ENCODE_ERROR:
#endif
    case EVP_R_DIFFERENT_PARAMETERS:
    case EVP_R_DECODE_ERROR:
        return KM_ERROR_INVALID_ARGUMENT;

    case EVP_R_DIFFERENT_KEY_TYPES:
        return KM_ERROR_INCOMPATIBLE_ALGORITHM;
    }

    return KM_ERROR_UNKNOWN_ERROR;
}

}  // namespace keymaster
