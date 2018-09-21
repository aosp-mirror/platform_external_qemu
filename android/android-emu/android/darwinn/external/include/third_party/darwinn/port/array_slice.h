#ifndef THIRD_PARTY_DARWINN_PORT_ARRAY_SLICE_H_
#define THIRD_PARTY_DARWINN_PORT_ARRAY_SLICE_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "util/gtl/array_slice.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/array_slice.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_ARRAY_SLICE_H_
