// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/utils/ipaddr.h"

#include <stdio.h>

int inet_strtoip(const char* str, uint32_t* ip)
{
    int comp[4];

    if (sscanf(str, "%d.%d.%d.%d", &comp[0], &comp[1], &comp[2], &comp[3]) != 4)
        return -1;

    if ((unsigned)comp[0] >= 256 ||
        (unsigned)comp[1] >= 256 ||
        (unsigned)comp[2] >= 256 ||
        (unsigned)comp[3] >= 256)
        return -1;

    *ip = (uint32_t)((comp[0] << 24) | (comp[1] << 16) |
                     (comp[2] << 8)  |  comp[3]);
    return 0;
}

char* inet_iptostr(uint32_t ip)
{
    static char buff[32];

    snprintf(buff, sizeof(buff), "%d.%d.%d.%d",
             (ip >> 24) & 255,
             (ip >> 16) & 255,
             (ip >> 8) & 255,
             ip & 255);
    return buff;
}
