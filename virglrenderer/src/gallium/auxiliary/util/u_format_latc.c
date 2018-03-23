/**************************************************************************
 *
 * Copyright (C) 2011 Red Hat Inc.
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

#include <stdio.h>
#include "u_math.h"
#include "u_format.h"
#include "u_format_rgtc.h"
#include "u_format_latc.h"
#include "util/rgtc.h"

void
util_format_latc1_unorm_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
}

void
util_format_latc1_unorm_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_rgtc1_unorm_unpack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_latc1_unorm_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row,
					 unsigned src_stride, unsigned width, unsigned height)
{
   util_format_rgtc1_unorm_pack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_latc1_unorm_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
}

void
util_format_latc1_unorm_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_rgtc1_unorm_pack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_latc1_unorm_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
}

void
util_format_latc1_snorm_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   fprintf(stderr,"%s\n", __func__);
}

void
util_format_latc1_snorm_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   fprintf(stderr,"%s\n", __func__);
}

void
util_format_latc1_snorm_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   fprintf(stderr,"%s\n", __func__);
}

void
util_format_latc1_snorm_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
}

void
util_format_latc1_snorm_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
}

void
util_format_latc1_snorm_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
}


void
util_format_latc2_unorm_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
}

void
util_format_latc2_unorm_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_rgtc2_unorm_unpack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_latc2_unorm_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_rgtc2_unorm_pack_rgba_8unorm(dst_row, dst_stride, src_row, src_stride, width, height);
}

void
util_format_latc2_unorm_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_rxtc2_unorm_pack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height, 3);
}

void
util_format_latc2_unorm_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
}

void
util_format_latc2_unorm_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
}


void
util_format_latc2_snorm_fetch_rgba_8unorm(uint8_t *dst, const uint8_t *src, unsigned i, unsigned j)
{
   fprintf(stderr,"%s\n", __func__);
}

void
util_format_latc2_snorm_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   fprintf(stderr,"%s\n", __func__);
}

void
util_format_latc2_snorm_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   fprintf(stderr,"%s\n", __func__);
}

void
util_format_latc2_snorm_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
}

void
util_format_latc2_snorm_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   util_format_rxtc2_snorm_pack_rgba_float(dst_row, dst_stride, src_row, src_stride, width, height, 3);
}

void
util_format_latc2_snorm_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
}

