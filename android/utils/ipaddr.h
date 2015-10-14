#pragma once

#include "android/utils/compiler.h"

#include <stdint.h>

ANDROID_BEGIN_HEADER

// convert an IPv4 string representation to int
// returns 0 on success
int inet_strtoip(const char* str, uint32_t* ip);

// convert an IPv4 to a string
// returns a static string buffer
char* inet_iptostr(uint32_t ip);

ANDROID_END_HEADER
