/* Copyright (C) 2007-2013 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "qemu/osdep.h"
#include "framebuffer.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "ui/console.h"
#include "ui/pixel_ops.h"
#include "trace.h"
#include "exec/address-spaces.h"
#include "hw/display/goldfish_fb.h"
#include "migration/register.h"

#include <inttypes.h>

static int s_use_host_gpu = 0;
static int s_display_bpp = 32;

void goldfish_fb_set_use_host_gpu(int enabled) {
    s_use_host_gpu = enabled;
}

void goldfish_fb_set_display_depth(int depth) {
    s_display_bpp = depth;
}

#define DEST_BITS 8
#define SOURCE_BITS 16
#include "goldfish_fb_template.h"
#define DEST_BITS 15
#define SOURCE_BITS 16
#include "goldfish_fb_template.h"
#define DEST_BITS 16
#define SOURCE_BITS 16
#include "goldfish_fb_template.h"
#define DEST_BITS 24
#define SOURCE_BITS 16
#include "goldfish_fb_template.h"
#define DEST_BITS 32
#define SOURCE_BITS 16
#include "goldfish_fb_template.h"
#define DEST_BITS 8
#define SOURCE_BITS 32
#include "goldfish_fb_template.h"
#define DEST_BITS 15
#define SOURCE_BITS 32
#include "goldfish_fb_template.h"
#define DEST_BITS 16
#define SOURCE_BITS 32
#include "goldfish_fb_template.h"
#define DEST_BITS 24
#define SOURCE_BITS 32
#include "goldfish_fb_template.h"
#define DEST_BITS 32
#define SOURCE_BITS 32
#include "goldfish_fb_template.h"

#define TYPE_GOLDFISH_FB "goldfish_fb"
#define GOLDFISH_FB(obj) OBJECT_CHECK(struct goldfish_fb_state, (obj), TYPE_GOLDFISH_FB)
/* These values *must* match the platform definitions found under
 * <system/graphics.h>
 */
enum {
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    HAL_PIXEL_FORMAT_RGB_888            = 3,
    HAL_PIXEL_FORMAT_RGB_565            = 4,
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,
};

enum {
    FB_GET_WIDTH        = 0x00,
    FB_GET_HEIGHT       = 0x04,
    FB_INT_STATUS       = 0x08,
    FB_INT_ENABLE       = 0x0c,
    FB_SET_BASE         = 0x10,
    FB_SET_ROTATION     = 0x14, /* DEPRECATED */
    FB_SET_BLANK        = 0x18,
    FB_GET_PHYS_WIDTH   = 0x1c,
    FB_GET_PHYS_HEIGHT  = 0x20,
    FB_GET_FORMAT       = 0x24,

    FB_INT_VSYNC             = 1U << 0,
    FB_INT_BASE_UPDATE_DONE  = 1U << 1
};

struct goldfish_fb_state {
    SysBusDevice parent;

    QemuConsole *con;
    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t fb_base;
    uint32_t base_valid : 1;
    uint32_t need_update : 1;
    uint32_t need_int : 1;
    uint32_t blank : 1;
    uint32_t int_status;
    uint32_t int_enable;
    int      rotation;   /* 0, 1, 2 or 3 */
    int      dpi;
    int      format;

    MemoryRegionSection fbsection;
};

#define  GOLDFISH_FB_SAVE_VERSION  3

/* Console hooks */
void goldfish_fb_set_rotation(int rotation)
{
    DeviceState *dev = qdev_find_recursive(sysbus_get_default(), TYPE_GOLDFISH_FB);
    if (dev) {
        struct goldfish_fb_state *s = GOLDFISH_FB(dev);
        DisplaySurface *ds = qemu_console_surface(s->con);
        s->rotation = rotation;
        s->need_update = 1;
        qemu_console_resize(s->con, surface_height(ds), surface_width(ds));
    } else {
        fprintf(stderr,"%s: unable to find FB dev\n", __func__);
    }
}

static void goldfish_fb_save(QEMUFile*  f, void*  opaque)
{
    struct goldfish_fb_state*  s = opaque;

    DisplaySurface *ds = qemu_console_surface(s->con);

    qemu_put_be32(f, surface_width(ds));
    qemu_put_be32(f, surface_height(ds));
    qemu_put_be32(f, surface_stride(ds));
    qemu_put_byte(f, 0);

    qemu_put_be32(f, s->fb_base);
    qemu_put_byte(f, s->base_valid);
    qemu_put_byte(f, s->need_update);
    qemu_put_byte(f, s->need_int);
    qemu_put_byte(f, s->blank);
    qemu_put_be32(f, s->int_status);
    qemu_put_be32(f, s->int_enable);
    qemu_put_be32(f, s->rotation);
    qemu_put_be32(f, s->dpi);
    qemu_put_be32(f, s->format);
}

static int  goldfish_fb_load(QEMUFile*  f, void*  opaque, int  version_id)
{
    struct goldfish_fb_state*  s   = opaque;
    int                        ret = -1;
    int                        ds_w, ds_h, ds_pitch, ds_rot;

    if (version_id != GOLDFISH_FB_SAVE_VERSION)
        goto Exit;

    ds_w     = qemu_get_be32(f);
    ds_h     = qemu_get_be32(f);
    ds_pitch = qemu_get_be32(f);
    ds_rot   = qemu_get_byte(f);

    DisplaySurface *ds = qemu_console_surface(s->con);

    if (surface_width(ds) != ds_w ||
        surface_height(ds) != ds_h ||
        surface_stride(ds) != ds_pitch ||
        ds_rot != 0)
    {
        /* XXX: We should be able to force a resize/rotation from here ? */
        fprintf(stderr, "%s: framebuffer dimensions mismatch\n", __FUNCTION__);
        goto Exit;
    }

    s->fb_base      = qemu_get_be32(f);
    s->base_valid   = qemu_get_byte(f);
    s->need_update  = qemu_get_byte(f);
    s->need_int     = qemu_get_byte(f);
    s->blank        = qemu_get_byte(f);
    s->int_status   = qemu_get_be32(f);
    s->int_enable   = qemu_get_be32(f);
    s->rotation     = qemu_get_be32(f);
    s->dpi          = qemu_get_be32(f);
    s->format       = qemu_get_be32(f);

    /* force a refresh */
    s->need_update = 1;

    ret = 0;
Exit:
    return ret;
}

static int
pixels_to_mm(int  pixels, int dpi)
{
    /* dpi = dots / inch
    ** inch = dots / dpi
    ** mm / 25.4 = dots / dpi
    ** mm = (dots * 25.4)/dpi
    */
    return (int)(0.5 + 25.4 * pixels  / dpi);
}


#define  STATS  0

#if STATS
static int   stats_counter;
static long  stats_total;
static int   stats_full_updates;
static long  stats_total_full_updates;
#endif

static void goldfish_fb_update_display(void *opaque)
{
    struct goldfish_fb_state *s = (struct goldfish_fb_state *)opaque;
    DisplaySurface *ds = qemu_console_surface(s->con);
    int full_update = 0;

    if (!s || !s->con || surface_bits_per_pixel(ds) == 0 || !s->fb_base)
        return;

    if((s->int_enable & FB_INT_VSYNC) && !(s->int_status & FB_INT_VSYNC)) {
        s->int_status |= FB_INT_VSYNC;
        qemu_irq_raise(s->irq);
    }

    if(s->need_update) {
        full_update = 1;
        if(s->need_int) {
            s->int_status |= FB_INT_BASE_UPDATE_DONE;
            if(s->int_enable & FB_INT_BASE_UPDATE_DONE)
                qemu_irq_raise(s->irq);
        }
        s->need_int = 0;
        s->need_update = 0;
    }

    int dest_width = surface_width(ds);
    int dest_height = surface_height(ds);
    int dest_pitch = surface_stride(ds);
    int ymin, ymax;

#if STATS
    if (full_update)
        stats_full_updates += 1;
    if (++stats_counter == 120) {
        stats_total               += stats_counter;
        stats_total_full_updates  += stats_full_updates;

        trace_goldfish_fb_update_stats(stats_full_updates*100.0/stats_counter,
                stats_total_full_updates*100.0/stats_total );

        stats_counter      = 0;
        stats_full_updates = 0;
    }
#endif /* STATS */

    if (s->blank)
    {
        void *dst_line = surface_data(ds);
        memset( dst_line, 0, dest_height*dest_pitch );
        ymin = 0;
        ymax = dest_height-1;
    }
    else
    {
        int src_width, src_height;
        int dest_row_pitch, dest_col_pitch;
        drawfn fn;

        /* The source framebuffer is always read in a linear fashion,
         * we achieve rotation by altering the destination
         * step-per-pixel.
         */
        switch (s->rotation) {
        case 0: /* Normal, native landscape view */
            src_width = dest_width;
            src_height = dest_height;
            dest_row_pitch = surface_stride(ds);
            dest_col_pitch = surface_bytes_per_pixel(ds);
            break;
        case 1: /* 90 degree, portrait view */
            src_width = dest_height;
            src_height = dest_width;
            dest_row_pitch = -surface_bytes_per_pixel(ds);
            dest_col_pitch = surface_stride(ds);
            break;
        case 2: /* 180 degree, inverted landscape view */
            src_width = dest_width;
            src_height = dest_height;
            dest_row_pitch = -surface_stride(ds);
            dest_col_pitch = -surface_bytes_per_pixel(ds);
            break;
        case 3: /* 270 degree, mirror portrait view */
            src_width = dest_height;
            src_height = dest_width;
            dest_row_pitch = surface_bytes_per_pixel(ds);
            dest_col_pitch = -surface_stride(ds);
            break;
        default:
            g_assert_not_reached();
        }

        int source_bytes_per_pixel = 2;

        switch (s->format) { /* source format */
        case HAL_PIXEL_FORMAT_RGB_565:
            source_bytes_per_pixel = 2;
            switch (surface_bits_per_pixel(ds)) { /* dest format */
            case 8:  fn = draw_line_16_8;  break;
            case 15: fn = draw_line_16_15; break;
            case 16: fn = draw_line_16_16; break;
            case 24: fn = draw_line_16_24; break;
            case 32: fn = draw_line_16_32; break;
            default:
                hw_error("goldfish_fb: bad dest color depth\n");
                return;
            }
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            source_bytes_per_pixel = 4;
            switch (surface_bits_per_pixel(ds)) { /* dest format */
            case 8:  fn = draw_line_32_8;  break;
            case 15: fn = draw_line_32_15; break;
            case 16: fn = draw_line_32_16; break;
            case 24: fn = draw_line_32_24; break;
            case 32: fn = draw_line_32_32; break;
            default:
                hw_error("goldfish_fb: bad dest color depth\n");
                return;
            }
            break;
        default:
            hw_error("goldfish_fb: bad source color format\n");
            return;
        }

        ymin = 0;
        // with -gpu on, the following check and return will save 2%
        // CPU time on OSX; saving on other platforms may differ.
        if (s_use_host_gpu) return;

        if (full_update) {
            framebuffer_update_memory_section(
                    &s->fbsection, get_system_memory(), s->fb_base,
                    src_height, src_width * source_bytes_per_pixel);
        }
        framebuffer_update_display(ds, &s->fbsection,
                                   src_width, src_height,
                                   src_width * source_bytes_per_pixel,
                                   dest_row_pitch, dest_col_pitch,
                                   full_update,
                                   fn, ds, &ymin, &ymax);
    }

    ymax += 1;
    if (ymin >= 0) {
        if (s->rotation % 2) {
            /* In portrait mode we are drawing "sideways" so always
             * need to update the whole screen */
            trace_goldfish_fb_update_display(0, dest_height, 0, dest_width);
            dpy_gfx_update(s->con, 0, 0, dest_width, dest_height);

        } else {
            trace_goldfish_fb_update_display(ymin, ymax-ymin, 0, dest_width);
            dpy_gfx_update(s->con, 0, ymin, dest_width, ymax-ymin);
        }
    }
}

static void goldfish_fb_invalidate_display(void * opaque)
{
    // is this called?
    struct goldfish_fb_state *s = (struct goldfish_fb_state *)opaque;
    s->need_update = 1;
}

static uint64_t goldfish_fb_read(void *opaque, hwaddr offset, unsigned size)
{
    uint64_t ret = 0;
    struct goldfish_fb_state *s = opaque;
    DisplaySurface *ds = qemu_console_surface(s->con);

    if (!s_display_bpp) {
        s_display_bpp = surface_bits_per_pixel(ds);
    }

    switch(offset) {
        case FB_GET_WIDTH:
            ret = surface_width(ds);
            break;

        case FB_GET_HEIGHT:
            ret = surface_height(ds);
            break;

        case FB_INT_STATUS:
            ret = s->int_status & s->int_enable;
            if(ret) {
                s->int_status &= ~ret;
                qemu_irq_lower(s->irq);
            }
            break;

        case FB_GET_PHYS_WIDTH:
            ret = pixels_to_mm( surface_width(ds), s->dpi );
            break;

        case FB_GET_PHYS_HEIGHT:
            ret = pixels_to_mm( surface_height(ds), s->dpi );
            break;

        case FB_GET_FORMAT:
            /* A kernel making this query supports high color and true color */
            switch (s_display_bpp) {   /* hw.lcd.depth */
            case 32:
            case 24:
               ret = HAL_PIXEL_FORMAT_RGBX_8888;
               break;
            case 16:
               ret = HAL_PIXEL_FORMAT_RGB_565;
               break;
            default:
               error_report("goldfish_fb_read: Bad display bit depth %d",
                       s_display_bpp);
               break;
            }
            s->format = ret;
            break;

        default:
            error_report("goldfish_fb_read: Bad offset 0x" TARGET_FMT_plx,
                    offset);
            break;
    }

    trace_goldfish_fb_memory_read(offset, ret);
    return ret;
}

static void goldfish_fb_write(void *opaque, hwaddr offset, uint64_t val,
        unsigned size)
{
    struct goldfish_fb_state *s = opaque;

    trace_goldfish_fb_memory_write(offset, val);

    switch(offset) {
        case FB_INT_ENABLE:
            s->int_enable = val;
            qemu_set_irq(s->irq, s->int_status & s->int_enable);
            break;
        case FB_SET_BASE:
            s->fb_base = val;
            s->int_status &= ~FB_INT_BASE_UPDATE_DONE;
            s->need_update = 1;
            s->need_int = 1;
            s->base_valid = 1;
            /* The guest is waiting for us to complete an update cycle
             * and notify it, so make sure we do a redraw immediately.
             */
            if (s_use_host_gpu) return;

            graphic_hw_update(s->con);
            qemu_set_irq(s->irq, s->int_status & s->int_enable);
            break;
        case FB_SET_ROTATION:
            error_report("%s: use of deprecated FB_SET_ROTATION %" PRIu64,
                         __func__, val);
            break;
        case FB_SET_BLANK:
            s->blank = val;
            s->need_update = 1;
            break;
        default:
            error_report("goldfish_fb_write: Bad offset 0x" TARGET_FMT_plx,
                    offset);
    }
}

static const MemoryRegionOps goldfish_fb_iomem_ops = {
    .read = goldfish_fb_read,
    .write = goldfish_fb_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static const GraphicHwOps goldfish_fb_ops = {
    .invalidate = goldfish_fb_invalidate_display,
    .gfx_update = goldfish_fb_update_display,
};

static SaveVMHandlers goldfish_vmhandlers= {
    .save_state = goldfish_fb_save,
    .load_state = goldfish_fb_load
};


static int goldfish_fb_init(SysBusDevice *sbdev)
{
    DeviceState *dev = DEVICE(sbdev);
    struct goldfish_fb_state *s = GOLDFISH_FB(dev);

    dev->id = g_strdup(TYPE_GOLDFISH_FB);

    sysbus_init_irq(sbdev, &s->irq);

    s->con = graphic_console_init(dev, 0, &goldfish_fb_ops, s);

    s->dpi = 165;  /* TODO: Find better way to get actual value ! */

    s->format = HAL_PIXEL_FORMAT_RGB_565;

    memory_region_init_io(&s->iomem, OBJECT(s), &goldfish_fb_iomem_ops, s,
            "goldfish_fb", 0x100);
    sysbus_init_mmio(sbdev, &s->iomem);

    register_savevm_live(dev, "goldfish_fb", 0, GOLDFISH_FB_SAVE_VERSION,
                     &goldfish_vmhandlers, s);

    return 0;
}

static void goldfish_fb_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SysBusDeviceClass *k = SYS_BUS_DEVICE_CLASS(klass);

    k->init = goldfish_fb_init;
    dc->desc = "goldfish framebuffer";
}

static const TypeInfo goldfish_fb_info = {
    .name          = TYPE_GOLDFISH_FB,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct goldfish_fb_state),
    .class_init    = goldfish_fb_class_init,
};

static void goldfish_fb_register(void)
{
    type_register_static(&goldfish_fb_info);
}

type_init(goldfish_fb_register);
