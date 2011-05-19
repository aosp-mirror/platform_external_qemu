#include <unistd.h>
#include <string.h>
#include <sys/utsname.h>
#include "android/utils/debug.h"

#define D(...) VERBOSE_PRINT(init,__VA_ARGS__)

/* A simple routine used to check that we can run the program under KVM.
 * We simply want to ensure that the emulator binary and the kernel have the
 * same endianess.
 *
 * A 32-bit executable cannot use the 64-bit KVM interface, even to run a 32-bit guest.
 */

#ifndef __linux__
#error "This file should only be compiled under linux"
#endif

int
kvm_check_allowed(void)
{
    /* sizeof(void*) is enough to give us the program's bitness */
    const int isProgram64bits = (sizeof(void*) == 8);
    int isKernel64bits = 0;
    struct utsname utn;

    /* Is there a /dev/kvm device file here? */
    if (access("/dev/kvm",F_OK)) {
        /* no need to print a warning here */
        D("No kvm device file detected");
        return 0;
    }

    /* Can we access it? */
    if (access("/dev/kvm",R_OK)) {
        D("KVM device file is not readable for this user.");
        return 0;
    }

    /* Determine kernel endianess */
    memset(&utn,0,sizeof(utn));
    uname(&utn);
    D("Kernel machine type: %s", utn.machine);
    if (strcmp(utn.machine,"x86_64") == 0)
        isKernel64bits = 1;
    else if (strcmp(utn.machine,"amd64") == 0)
        isKernel64bits = 1;


    if (isProgram64bits != isKernel64bits) {
        if (isProgram64bits) {
            D("kvm disabled (64-bit emulator and 32-bit kernel)");
        } else {
            D("kvm disabled (32-bit emulator and 64-bit kernel)");
        }
        return 0;
    }

    D("KVM mode auto-enabled!");
    return 1;
}

