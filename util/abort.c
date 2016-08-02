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

#include "qemu/abort.h"
#include <stdio.h>
#include <stdlib.h>

static QemuAbortHandler qemu_abort_handler = NULL;

void QEMU_NORETURN qemu_abort(const char* format, ...) {
    va_list args;
    va_start(args, format);
    if (qemu_abort_handler) {
        (*qemu_abort_handler)(format, args);
    } else {
        vfprintf(stderr, format, args);
    }
    va_end(args);
    abort();
}

void qemu_abort_set_handler(QemuAbortHandler abort_handler) {
    qemu_abort_handler = abort_handler;
}

bool qemu_abort_has_custom_handler(void) {
    return qemu_abort_handler != NULL;
}
