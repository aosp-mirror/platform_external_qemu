/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "TimestampPacketManip.h"

#include <cstring>
#include <stdlib.h>

static const size_t tstampSize = sizeof(uint64_t);

size_t TimestampPacketManip::getTimestampSize() {
  return tstampSize;
}

void* TimestampPacketManip::tagPacket(void* ptr, const uint64_t tstamp) {
  memcpy(ptr, &tstamp, tstampSize);
  return static_cast<unsigned char*>(ptr)+ tstampSize;
}

uint64_t TimestampPacketManip::stripPacket(void* ptr) {
  uint64_t tstamp;
  memcpy(&tstamp, ptr, tstampSize);
  return tstamp;
}
