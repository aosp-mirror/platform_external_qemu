#include "qemu/osdep.h"

#include "android/utils/win32_unicode.h"

HANDLE win32CreateFile(
        LPCTSTR               lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile)
{
    HANDLE result = INVALID_HANDLE_VALUE;
    wchar_t* wide_name = win32_utf8_to_utf16_str(lpFileName);
    if (wide_name != NULL) {
        result = CreateFileW(wide_name, dwDesiredAccess,
                             dwShareMode, lpSecurityAttributes,
                             dwCreationDisposition, dwFlagsAndAttributes,
                             hTemplateFile);
        free(wide_name);
    }
    return result;
}

DWORD win32GetCurrentDirectory(
        DWORD  nBufferLength,
        LPTSTR lpBuffer)
{
    wchar_t wide_buffer[MAX_PATH];
    DWORD ret = GetCurrentDirectoryW(MAX_PATH, wide_buffer);
    if (ret == 0 || ret > MAX_PATH) {
        return ret;
    }
    int ret2 = win32_utf16_to_utf8_buf(wide_buffer, lpBuffer, nBufferLength);
    return (ret2 < 0 || (DWORD)ret2 > nBufferLength) ? 0 : (DWORD)ret2;
}

DWORD win32GetModuleFileName(
        HMODULE hModule,
        LPTSTR  lpFilename,
        DWORD   nSize)
{
    wchar_t wide_buffer[MAX_PATH];
    if (!GetModuleFileNameW(hModule, wide_buffer, MAX_PATH)) {
        return 0;
    }

    int ret = win32_utf16_to_utf8_buf(wide_buffer, lpFilename, nSize);
    if (ret < 0 || ret >= nSize) {
        return 0;
    }
    return (DWORD)ret;
}
