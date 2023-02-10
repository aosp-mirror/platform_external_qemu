/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/*
 * This is the source for a dummy crash program that starts the crash reporter
 * and then crashes due to an undefined symbol
 */
#include <stdio.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "android/crashreport/CrashConsent.h"
#include "android/crashreport/crash-initializer.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <stdlib.h>
#include <signal.h>
#endif

using consentProviderFunction = android::crashreport::CrashConsent* (*)(void);

void crashme(int arg, bool nocrash, int delay_ms) {
    if (delay_ms) {
        printf("Delaying crash by %d ms\n", delay_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
    for (int i = 0; i < arg; i++) {
        if (!nocrash) {
            volatile int* volatile ptr = nullptr;
            *ptr = 1313;  // die

#ifndef _WIN32
            raise(SIGABRT);
#endif
            abort();
        }
    }
}

#ifdef _WIN32
typedef HMODULE HandleType;
#else
typedef void* HandleType;
#endif

consentProviderFunction getConsentProvider(HandleType lib) {
#ifdef _WIN32
    return reinterpret_cast<consentProviderFunction>(
            GetProcAddress(lib, "consentProvider"));
#else
    return reinterpret_cast<consentProviderFunction>(
            dlsym(lib, "consentProvider"));
#endif
}

bool load_consent_provider(const char* fromDll) {
    HandleType library;
#ifdef _WIN32
    library = LoadLibraryA(fromDll);
#else
    library = dlopen(fromDll, RTLD_NOW);
#endif

    if (!library) {
        fprintf(stderr, "Unable to load dll, not overriding initializer.\n");
        return false;
    }

    // Probe for function symbol.
    auto provider = getConsentProvider(library);
    android::crashreport::inject_consent_provider(provider());
    return true;
}

/* Main routine */
int main(int argc, char** argv) {
    const char* dll = nullptr;
    bool nocrash = false;
    int delay_ms = 0;
    int nn;
    for (nn = 1; nn < argc; nn++) {
        const char* opt = argv[nn];
        if (!strcmp(opt, "-dll")) {
            if (nn + 1 < argc) {
                nn++;
                dll = argv[nn];
            }
        } else if (!strcmp(opt, "-nocrash")) {
            nocrash = true;
        } else if (!strcmp(opt, "-delay_ms")) {
            if (nn + 1 < argc) {
                nn++;
                delay_ms = atoi(argv[nn]);
            }
        }
    }

    if (dll) {
        load_consent_provider(dll);
    }

    crashhandler_init(argc, argv);
    crashme(argc, nocrash, delay_ms);
    printf("test crasher done\n");
    return 0;
}
