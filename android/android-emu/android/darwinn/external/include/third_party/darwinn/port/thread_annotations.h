#ifndef THIRD_PARTY_DARWINN_PORT_THREAD_ANNOTATIONS_H_
#define THIRD_PARTY_DARWINN_PORT_THREAD_ANNOTATIONS_H_

#if defined(DARWINN_PORT_GOOGLE3)
#include "base/thread_annotations.h"
#endif

#if defined(DARWINN_PORT_DEFAULT) || \
    defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)
#include "third_party/darwinn/port/default/thread_annotations.h"
#endif

#endif  // THIRD_PARTY_DARWINN_PORT_THREAD_ANNOTATIONS_H_
