#ifndef THIRD_PARTY_DARWINN_PORT_CASTS_H_
#define THIRD_PARTY_DARWINN_PORT_CASTS_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "base/casts.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || defined(DARWINN_PORT_ANDROID_SYSTEM)
#include "third_party/darwinn/port/default/casts.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_CASTS_H_
