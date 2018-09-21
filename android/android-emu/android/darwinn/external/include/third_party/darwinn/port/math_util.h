#ifndef THIRD_PARTY_DARWINN_PORT_MATH_UTIL_H_
#define THIRD_PARTY_DARWINN_PORT_MATH_UTIL_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "util/math/mathutil.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/math_util.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_MATH_UTIL_H_
