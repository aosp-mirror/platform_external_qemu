/*
 * QEMU HAXM support
 *
 * Copyright (c) 2015, Intel Corporation
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * See the COPYING file in the top-level directory.
 *
 */

#include "sysemu/hax.h"

int hax_sync_vcpus(void)
{
    return 0;
}

void hax_disable(int disable)
{
   return;
}

int hax_pre_init(uint64_t ram_size)
{
   return 0;
}

int hax_enabled(void)
{
   return 0;
}

int hax_ug_platform(void)
{
    return 0;
}
