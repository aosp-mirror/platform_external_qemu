#ifndef THIRD_PARTY_DARWINN_PORT_STATUS_H_
#define THIRD_PARTY_DARWINN_PORT_STATUS_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "util/task/status.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/status.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_STATUS_H_
