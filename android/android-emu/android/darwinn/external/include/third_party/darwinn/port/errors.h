#ifndef THIRD_PARTY_DARWINN_PORT_ERRORS_H_
#define THIRD_PARTY_DARWINN_PORT_ERRORS_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "util/task/canonical_errors.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/errors.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_ERRORS_H_
