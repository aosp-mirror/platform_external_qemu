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
/* Special case to include "qemu-options.h" here without issues */
#undef POISON_CONFIG_ANDROID

#include "qemu/osdep.h"
#include <windows.h>
#include <mmsystem.h>
#include "sysemu/sysemu.h"
#include "qemu-options.h"

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

        /* Windows takes a copy and does not continue to use our string.
         * Therefore it can be safely freed on this platform.  POSIX code
         * typically has to leak the string because according to the spec it
         * becomes part of the environment.
         */
        g_free(string);
    }
    return result;
}

static void (*ctrlc_handler)(void) = NULL;

void qemu_set_ctrlc_handler(void(*handler)(void)) {
    ctrlc_handler = handler;
}

static BOOL WINAPI qemu_ctrl_handler(DWORD type)
{
    if (ctrlc_handler) {
        (*ctrlc_handler)();
    } else {
        qemu_system_shutdown_request(SHUTDOWN_CAUSE_HOST_SIGNAL);
    }
    fflush(stdout);
    fflush(stderr);

    // There are two major ways this handler can be invoked:
    // 1. Ctrl-C in console.
    // 2. Pressing 'x' on the console window.
    // If #2 (press 'x'), the emulator will close in 5 seconds
    // unless we FreeConsole() and ExitThread(0) below.
    // Too-early process termination can abort pending snapshot saves.
    // This is definitely a hack that can break in future Windows versions.
    //
    // Note: There is another ctrl handler |ctrlHandler| in
    // android/utils/exec.cpp that needs to be in sync with this one, so if the
    // end result on editing this function isn't what is expected, try changing
    // that one too.
    if (type == CTRL_CLOSE_EVENT) {
        FreeConsole();
        ExitThread(0);
    } else {
         /* Windows 7 kills application when the function returns.
            Sleep here to give QEMU a try for closing.
            Sleep period is 90s */
        Sleep(90000);
    }

    /* On timeout, return FALSE so the default handler
     * finished the process anyway */

    return FALSE;
}

static TIMECAPS mm_tc;

static void os_undo_timer_resolution(void)
{
    timeEndPeriod(mm_tc.wPeriodMin);
}

void os_setup_early_signal_handling(void)
{
    SetConsoleCtrlHandler(qemu_ctrl_handler, TRUE);
    timeGetDevCaps(&mm_tc, sizeof(mm_tc));
    timeBeginPeriod(mm_tc.wPeriodMin);
    atexit(os_undo_timer_resolution);
}

/* Look for support files in the same directory as the executable.  */
char *os_find_datadir(void)
{
    return qemu_get_exec_dir();
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

int qemu_create_pidfile(const char *filename)
{
    char buffer[128];
    int len;
    HANDLE file;
    OVERLAPPED overlap;
    BOOL ret;
    memset(&overlap, 0, sizeof(overlap));

    file = win32CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                      OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        return -1;
    }
    len = snprintf(buffer, sizeof(buffer), "%d\n", getpid());
    ret = WriteFile(file, (LPCVOID)buffer, (DWORD)len,
                    NULL, &overlap);
    CloseHandle(file);
    if (ret == 0) {
        return -1;
    }
    return 0;
}
