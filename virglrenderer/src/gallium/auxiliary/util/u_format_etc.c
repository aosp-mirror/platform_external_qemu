#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "u_format_etc.h"

/* define etc1_parse_block and etc. */

void
util_format_etc1_rgb8_unpack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
}

void
util_format_etc1_rgb8_pack_rgba_8unorm(uint8_t *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   assert(0);
}

void
util_format_etc1_rgb8_unpack_rgba_float(float *dst_row, unsigned dst_stride, const uint8_t *src_row, unsigned src_stride, unsigned width, unsigned height)
{
}

void
util_format_etc1_rgb8_pack_rgba_float(uint8_t *dst_row, unsigned dst_stride, const float *src_row, unsigned src_stride, unsigned width, unsigned height)
{
   assert(0);
}

void
util_format_etc1_rgb8_fetch_rgba_float(float *dst, const uint8_t *src, unsigned i, unsigned j)
{
}
