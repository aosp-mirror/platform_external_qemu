// THIS IS EMPTY ON PURPOSE, AS THESE IMPLEMENTATIONS ARE NOT UNICODE SAFE

// You should take a dependency on android-emu-base, where you will find the proper
// implementation that is unicode compliant.

#if 0
#ifdef _MSC_VER
  #include "msvc-posix.h"
// TODO: MSVC build currently relies on this; it should not use this.
#else

#ifndef LIBQEMUSTUB
#error "This file should not be linked against the Android emulator binaries, as it makes non-ASCII filename handling fail."
#endif

#endif
#include "qemu/osdep.h"

/* NOTE: This file should not be linked against the Android emulator binaries */
HANDLE win32CreateFile(
        LPCTSTR               lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile)
{
    return CreateFile(lpFileName, dwDesiredAccess, dwShareMode,
                      lpSecurityAttributes, dwCreationDisposition,
                      dwFlagsAndAttributes, hTemplateFile);
}

DWORD win32GetCurrentDirectory(
        DWORD  nBufferLength,
        LPTSTR lpBuffer)
{
    return GetCurrentDirectory(nBufferLength, lpBuffer);
}

DWORD win32GetModuleFileName(
        HMODULE hModule,
        LPTSTR  lpFilename,
        DWORD   nSize)
{
    return GetModuleFileName(hModule, lpFilename, nSize);
}

int win32_stat(const char* filepath, struct _stati64* st) {
    return _stati64(filepath, st);
}

int win32_lstat(const char* filepath, struct _stati64* st) {
    return win32_stat(filepath, st);
}
#endif