// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Tests hypervisors if capabilities are available.
// For now, this is Mac-only.

#include <gtest/gtest.h>

#ifdef __APPLE__

#ifdef __arm64__
#include <Hypervisor/Hypervisor.h>
#else
#include <Hypervisor/hv.h>
#include <Hypervisor/hv_vmx.h>
#endif
 
TEST(HypervisorTest, DISABLED_HypervisorFrameworkVmCreate) {
#ifdef __arm64__
    int res = hv_vm_create(0);
    EXPECT_EQ(HV_SUCCESS, res);
    hv_vm_destroy();
#else
    int res = hv_vm_create(HV_VM_DEFAULT);
    EXPECT_EQ(HV_SUCCESS, res);
    hv_vm_destroy();
#endif
}


#endif
