// Copyright 2002 and onwards Google Inc.
// Author: Sanjay Ghemawat
//
// Printf variants that place their output in a C++ string.
//
// Usage:
//      string result = StringPrintf("%d %s\n", 10, "hello");
//
// While StringF and StreamF are recommended for use, they are difficult to
// port. Fallback to StringPrintf as an alternative as it is easier to port.

#ifndef THIRD_PARTY_DARWINN_PORT_DEFAULT_STRINGPRINTF_H_
#define THIRD_PARTY_DARWINN_PORT_DEFAULT_STRINGPRINTF_H_

#include <stdarg.h>
#include <string>
#include <vector>

#include "third_party/darwinn/port/default/macros.h"

namespace platforms {
namespace darwinn {

// Returns a C++ string
std::string StringPrintf(const char* format, ...)
    // Tell the compiler to do printf format string checking.
    PRINTF_ATTRIBUTE(1,2);

}  // namespace darwinn
}  // namespace platforms

#endif  // THIRD_PARTY_DARWINN_PORT_DEFAULT_STRINGPRINTF_H_
