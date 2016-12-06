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
