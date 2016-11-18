/*
 * os-win32.c
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 * Copyright (c) 2010 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include "config-host.h"
#include "sysemu/sysemu.h"
#include "qemu-options.h"

#ifdef CONFIG_ANDROID
#include "android/skin/winsys.h"
#include "android/utils/win32_unicode.h"
#endif

/***********************************************************/
/* Functions missing in mingw */

int setenv(const char *name, const char *value, int overwrite)
{
    int result = 0;
    if (overwrite || !getenv(name)) {
        size_t length = strlen(name) + strlen(value) + 2;
        char *string = g_malloc(length);
        snprintf(string, length, "%s=%s", name, value);
        result = putenv(string);
    }
    return result;
}

static BOOL WINAPI qemu_ctrl_handler(DWORD type)
{
#ifdef CONFIG_ANDROID
    // In android, request closing the UI, instead of short-circuting down to
    // qemu. This will eventually call qemu_system_shutdown_request via a skin
    // event.
    skin_winsys_quit_request();
    // Windows 7 kills application when the function returns.
    // Sleep here to give QEMU a try for closing.
    // Sleep period is 10000ms because Windows kills the program
    // after 10 seconds anyway.
    Sleep(10000);
#else
    exit(STATUS_CONTROL_C_EXIT);
#endif  // !CONFIG_ANDROID
    return TRUE;
}

void os_setup_early_signal_handling(void)
{
    SetConsoleCtrlHandler(qemu_ctrl_handler, TRUE);
}

/* Look for support files in the same directory as the executable.  */
char *os_find_datadir(const char *argv0)
{
    char *p;
    char buf[MAX_PATH];
    DWORD len;

    wchar_t wideBuf[MAX_PATH];
    len = GetModuleFileNameW(NULL, wideBuf, MAX_PATH);
    if (len <= 0 || len >= MAX_PATH) {
        return NULL;
    }
    int written = win32_utf16_to_utf8_buf(wideBuf, buf, sizeof(buf));
    if (written < 0) {
        return NULL;
    }
    len = written;

    p = buf + len - 1;
    while (p != buf && *p != '\\')
        p--;
    *p = 0;
    if (access(buf, R_OK) == 0) {
        return g_strdup(buf);
    }
    return NULL;
}

void os_set_line_buffering(void)
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
}

/*
 * Parse OS specific command line options.
 * return 0 if option handled, -1 otherwise
 */
void os_parse_cmd_args(int index, const char *optarg)
{
    return;
}

void os_pidfile_error(void)
{
    fprintf(stderr, "Could not acquire pid file: %s\n", strerror(errno));
}

int qemu_create_pidfile(const char *filename)
{
    char buffer[128];
    int len;
    HANDLE file;
    OVERLAPPED overlap;
    BOOL ret;
    memset(&overlap, 0, sizeof(overlap));

    file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        return -1;
    }
    len = snprintf(buffer, sizeof(buffer), "%ld\n", (long)getpid());
    ret = WriteFileEx(file, (LPCVOID)buffer, (DWORD)len,
		      &overlap, NULL);
    if (ret == 0) {
        return -1;
    }
    return 0;
}
