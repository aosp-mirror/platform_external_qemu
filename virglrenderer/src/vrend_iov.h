/**************************************************************************
 *
 * Copyright (C) 2014 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
#ifndef VREND_IOV_H
#define VREND_IOV_H

#include "config.h"

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#else
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#endif

typedef void (*iov_cb)(void *cookie, unsigned int doff, void *src, int len);

size_t vrend_get_iovec_size(const struct iovec *iov, int iovlen);
size_t vrend_read_from_iovec(const struct iovec *iov, int iov_cnt,
                             size_t offset, char *buf, size_t bytes);
size_t vrend_write_to_iovec(const struct iovec *iov, int iov_cnt,
                            size_t offset, const char *buf, size_t bytes);

size_t vrend_read_from_iovec_cb(const struct iovec *iov, int iov_cnt,
                          size_t offset, size_t bytes, iov_cb iocb, void *cookie);

#endif
