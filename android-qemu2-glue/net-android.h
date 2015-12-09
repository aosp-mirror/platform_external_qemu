// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "config-host.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#if defined(CONFIG_SLIRP)
#include "libslirp.h"

void slirp_init_shapers(void* slirp_state,
                        void* net_client_state,
                        Slirp *slirp);
#endif  // CONFIG_SLIRP

ANDROID_END_HEADER
