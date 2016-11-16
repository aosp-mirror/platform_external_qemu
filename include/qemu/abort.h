/*
 * QEMU abort handler
 *
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef QEMU_ABORT_H
#define QEMU_ABORT_H

#include "qemu/compiler.h"

#include <stdarg.h>
#include <stdbool.h>

/* Abort the current program, |format| and following arguments are a printf-
 * style formatting string explaining the reason for the failure */
void QEMU_NORETURN qemu_abort(const char* format, ...)
        GCC_FMT_ATTR(1, 2);

/* Pointer to a function that takes a formatted string that explains why
 * QEMU needs to abort. |format| is a printf-style string, followed by
 * optional arguments in |args|.
 */
typedef void QEMU_NORETURN (*QemuAbortHandler)(const char *format,
                                               va_list args);

/* Change the QEMU abort handler to a specific implementation. The defaults
 * prints the output to stderr() then calls abort(). NOTE: The |abort_handler|
 * should not return. */
void qemu_abort_set_handler(QemuAbortHandler abort_handler);

/* Return true iff qemu_abort_set_handler() was called with a non-NULL
 * |abort_handler| value. */
bool qemu_abort_has_custom_handler(void);

#endif  /* QEMU_ABORT_H */
