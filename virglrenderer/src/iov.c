/*
 * this code is taken from Michael - the qemu code is GPLv2 so I don't want
 * to reuse it.
 * I've adapted it to handle offsets and callback
 */

//
// iovec.c
//
// Scatter/gather utility routines
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#include <assert.h>
#include <string.h>
#include "vrend_iov.h"

size_t vrend_get_iovec_size(const struct iovec *iov, int iovlen) {
  size_t size = 0;

  while (iovlen > 0) {
    size += iov->iov_len;
    iov++;
    iovlen--;
  }

  return size;
}

size_t vrend_read_from_iovec(const struct iovec *iov, int iovlen,
			     size_t offset,
			     char *buf, size_t count)
{
  size_t read = 0;
  size_t len;

  while (count > 0 && iovlen > 0) {
    if (iov->iov_len > offset) {
      len = iov->iov_len - offset;
      
      if (count < iov->iov_len - offset) len = count;

      memcpy(buf, (char*)iov->iov_base + offset, len);
      read += len;

      buf += len;
      count -= len;
      offset = 0;
    } else {
      offset -= iov->iov_len;
    }

    iov++;
    iovlen--;
  }
    assert(offset == 0);
  return read;
}

size_t vrend_write_to_iovec(const struct iovec *iov, int iovlen,
			 size_t offset, const char *buf, size_t count)
{
  size_t written = 0;
  size_t len;

  while (count > 0 && iovlen > 0) {
    if (iov->iov_len > offset) {
      len = iov->iov_len - offset;

      if (count < iov->iov_len - offset) len = count;

      memcpy((char*)iov->iov_base + offset, buf, len);
      written += len;

      offset = 0;
      buf += len;
      count -= len;
    } else {
      offset -= iov->iov_len;
    }
    iov++;
    iovlen--;
  }
    assert(offset == 0);
  return written;
}

size_t vrend_read_from_iovec_cb(const struct iovec *iov, int iovlen,
				size_t offset, size_t count,
				iov_cb iocb, void *cookie)
{
  size_t read = 0;
  size_t len;

  while (count > 0 && iovlen > 0) {
    if (iov->iov_len > offset) {
      len = iov->iov_len;
      
      if (count < iov->iov_len - offset) len = count;

      (*iocb)(cookie, count, (char*)iov->iov_base + offset, len);
      read += len;

      count -= len;
      offset = 0;
    } else {
      offset -= iov->iov_len;
    }
    iov++;
    iovlen--;
  }
    assert(offset == 0);
  return read;


}
