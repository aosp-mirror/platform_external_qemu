#ifndef THIRD_PARTY_DARWINN_PORT_ALIGNED_MALLOC_H_
#define THIRD_PARTY_DARWINN_PORT_ALIGNED_MALLOC_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "base/port.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/aligned_malloc.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_ALIGNED_MALLOC_H_
