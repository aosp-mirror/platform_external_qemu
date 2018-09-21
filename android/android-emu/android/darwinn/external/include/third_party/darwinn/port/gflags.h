#ifndef THIRD_PARTY_DARWINN_PORT_GFLAGS_H_
#define THIRD_PARTY_DARWINN_PORT_GFLAGS_H_

// Parses command line flags.
// See google::ParseCommandLineFlags for description of arguments.
inline void ParseFlags(const char* usage, int* argc, char*** argv,
                       bool remove_flags);

#if defined(DARWINN_PORT_GOOGLE3)
#include "base/commandlineflags.h"
#include "base/init_google.h"

inline void ParseFlags(const char* usage, int* argc, char*** argv,
                       bool remove_flags) {
  ::ParseCommandLineFlags(argc, argv, remove_flags);
}

#endif

#if defined(DARWINN_PORT_DEFAULT)

#include <gflags/gflags.h>

inline void ParseFlags(const char* usage, int* argc, char*** argv,
                       bool remove_flags) {
  google::ParseCommandLineFlags(argc, argv, remove_flags);
}

#endif

#if defined(DARWINN_PORT_ANDROID_SYSTEM) || \
    defined(DARWINN_PORT_ANDROID_EMULATOR)

// There is no gflags implementation in Android runtime. Provide a dummy
// implementation here.
#define DEFINE_bool(name, val, desc) bool FLAGS_##name = val
#define DEFINE_int32(name, val, desc) int32_t FLAGS_##name = val
inline void ParseFlags(const char* usage, int* argc, char*** argv,
                       bool remove_flags) {}

#endif

#endif  // THIRD_PARTY_DARWINN_PORT_GFLAGS_H_
