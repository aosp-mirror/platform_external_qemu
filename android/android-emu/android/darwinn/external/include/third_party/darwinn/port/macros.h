#ifndef THIRD_PARTY_DARWINN_PORT_MACROS_H_
#define THIRD_PARTY_DARWINN_PORT_MACROS_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "base/macros.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/macros.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_MACROS_H_
