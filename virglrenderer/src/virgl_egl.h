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
#ifndef VIRGL_EGL_H
#define VIRGL_EGL_H

#include "vrend_renderer.h"
struct virgl_egl;

struct virgl_egl *virgl_egl_init(void);
void virgl_egl_destroy(struct virgl_egl *ve);

virgl_renderer_gl_context virgl_egl_create_context(struct virgl_egl *ve, struct virgl_gl_ctx_param *vparams);
void virgl_egl_destroy_context(struct virgl_egl *ve, virgl_renderer_gl_context virglctx);
int virgl_egl_make_context_current(struct virgl_egl *ve, virgl_renderer_gl_context virglctx);
virgl_renderer_gl_context virgl_egl_get_current_context(struct virgl_egl *ve);

int virgl_egl_get_fourcc_for_texture(struct virgl_egl *ve, uint32_t tex_id, uint32_t format, int *fourcc);
uint32_t virgl_egl_get_gbm_format(uint32_t format);
#endif
