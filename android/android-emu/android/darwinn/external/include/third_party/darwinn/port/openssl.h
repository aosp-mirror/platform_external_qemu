#ifndef THIRD_PARTY_DARWINN_PORT_OPENSSL_H_
#define THIRD_PARTY_DARWINN_PORT_OPENSSL_H_

#if defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)

#include <openssl/bio.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#else

#include "third_party/openssl/bio.h"
#include "third_party/openssl/ecdsa.h"
#include "third_party/openssl/evp.h"
#include "third_party/openssl/pem.h"

#endif

#endif  // THIRD_PARTY_DARWINN_PORT_OPENSSL_H_
