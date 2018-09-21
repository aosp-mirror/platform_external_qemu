// Copyright (C) 1999 and onwards Google, Inc.
//
// Various portability macros, type definitions, and inline functions
// This file is used for both C and C++!
//
// These are weird things we need to do to get this compiling on
// random systems (and on SWIG).
//
// MOE:begin_strip
// This file is open source. You may export it with your open source projects
// as long as you use MOE to strip proprietary comments.
// MOE:end_strip

#ifndef THIRD_PARTY_DARWINN_PORT_DEFAULT_ALIGNED_MALLOC_H_
#define THIRD_PARTY_DARWINN_PORT_DEFAULT_ALIGNED_MALLOC_H_

#include <stdlib.h>         // for free(), aligned_alloc(),

#if defined(OS_CYGWIN) || defined(__ANDROID__)
#include <malloc.h>         // for memalign()
#endif

namespace platforms {
namespace darwinn {

inline void *aligned_malloc(size_t size, int minimum_alignment) {
#if defined(__ANDROID__) || defined(OS_ANDROID) || defined(OS_CYGWIN)
  return memalign(minimum_alignment, size);
#else  // !__ANDROID__ && !OS_ANDROID && !OS_CYGWIN
  return aligned_alloc(minimum_alignment, size);
#endif
}

inline void aligned_free(void *aligned_memory) {
  free(aligned_memory);
}

}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_PORT_DEFAULT_ALIGNED_MALLOC_H_
