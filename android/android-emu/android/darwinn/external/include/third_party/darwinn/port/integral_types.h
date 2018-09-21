#ifndef THIRD_PARTY_DARWINN_PORT_INTEGRAL_TYPES_H_
#define THIRD_PARTY_DARWINN_PORT_INTEGRAL_TYPES_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "base/integral_types.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/integral_types.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_INTEGRAL_TYPES_H_
