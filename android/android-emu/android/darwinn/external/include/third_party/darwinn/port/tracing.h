#ifndef THIRD_PARTY_DARWINN_PORT_DEFAULT_SYSTRACE_H_
#define THIRD_PARTY_DARWINN_PORT_DEFAULT_SYSTRACE_H_

#include <string>

// Use systrace for android.
#if defined(DARWINN_PORT_ANDROID_SYSTEM)

#include "Tracing.h"

// Use this only once per function, ideally at the beginning of each scope.
#define TRACE_SCOPE(name) NNTRACE_NAME_1(name)

// Use this to add trace markers in a scope that already has a TRACE_SCOPE.
#define TRACE_WITHIN_SCOPE(name) NNTRACE_NAME_SWITCH(name)

// No tracing for other environments.
#else

#define TRACE_SCOPE(name)
#define TRACE_WITHIN_SCOPE(name)

#endif

#endif  // THIRD_PARTY_DARWINN_PORT_DEFAULT_SYSTRACE_H_
