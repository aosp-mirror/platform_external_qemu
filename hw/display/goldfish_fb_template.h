/*
 *  QEMU model of the Goldfish framebuffer: drawing templates.
 *
 *  Copyright (c) 2014 Linaro Limited
 *
 * Based heavily on the milkymist drawing templates, which are:
 *
 *  Copyright (c) 2010 Michael Walle <michael@walle.cc>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#if DEST_BITS == 8
#define COPY_PIXEL(to, r, g, b)                    \
    do {                                           \
        *to = rgb_to_pixel8(r, g, b);              \
    } while (0)
#elif DEST_BITS == 15
#define COPY_PIXEL(to, r, g, b)                    \
    do {                                           \
        *(uint16_t *)to = rgb_to_pixel15(r, g, b); \
    } while (0)
#elif DEST_BITS == 16
#define COPY_PIXEL(to, r, g, b)                    \
    do {                                           \
        *(uint16_t *)to = rgb_to_pixel16(r, g, b); \
    } while (0)
#elif DEST_BITS == 24
#define COPY_PIXEL(to, r, g, b)                    \
    do {                                           \
        uint32_t tmp = rgb_to_pixel24(r, g, b);    \
        to[0] =         tmp & 0xff;              \
        to[1] =  (tmp >> 8) & 0xff;              \
        to[2] = (tmp >> 16) & 0xff;              \
    } while (0)
#elif DEST_BITS == 32
#define COPY_PIXEL(to, r, g, b)                    \
    do {                                           \
        *(uint32_t *)to = rgb_to_pixel32(r, g, b); \
    } while (0)
#else
#error unknown dest bit format
#endif

static void glue(glue(draw_line_, SOURCE_BITS), glue(_, DEST_BITS))(void *opaque, uint8_t *d, const uint8_t *s,
        int width, int deststep)
{
#if SOURCE_BITS == 16
    uint16_t rgb565;
    uint8_t r, g, b;

    while (width--) {
        rgb565 = lduw_le_p(s);
        r = ((rgb565 >> 11) & 0x1f) << 3;
        g = ((rgb565 >>  5) & 0x3f) << 2;
        b = ((rgb565 >>  0) & 0x1f) << 3;
        COPY_PIXEL(d, r, g, b);
        d += deststep;
        s += 2;
    }
#elif SOURCE_BITS == 32
    uint32_t rgbx8888;
    uint8_t r, g, b;

    while (width--) {
        rgbx8888 = *(uint32_t*)(s);
        b = ((rgbx8888 >> 16) & 0xff);
        g = ((rgbx8888 >>  8) & 0xff);
        r = ((rgbx8888 >>  0) & 0xff);
        COPY_PIXEL(d, r, g, b);
        d += deststep;
        s += 4;
    }
#else
#error unknown source bit format
#endif
}

#undef DEST_BITS
#undef SOURCE_BITS
#undef COPY_PIXEL
