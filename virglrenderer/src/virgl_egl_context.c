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
/* create our own EGL offscreen rendering context via gbm and rendernodes */


/* if we are using EGL and rendernodes then we talk via file descriptors to the remote
   node */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define EGL_EGLEXT_PROTOTYPES
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "igl.h"
#include <gbm.h>
#include <xf86drm.h>
#include "virglrenderer.h"
#include "virgl_egl.h"

#include "virgl_hw.h"
struct virgl_egl {
   EGLDisplay egl_display;
   EGLConfig egl_conf;
   EGLContext egl_ctx;
};

static bool virgl_egl_has_extension_in_string(const char *haystack, const char *needle)
{
   const unsigned needle_len = strlen(needle);

   if (needle_len == 0)
      return false;

   while (true) {
      const char *const s = strstr(haystack, needle);

      if (s == NULL)
         return false;

      if (s[needle_len] == ' ' || s[needle_len] == '\0') {
         return true;
      }

      /* strstr found an extension whose name begins with
       * needle, but whose name is not equal to needle.
       * Restart the search at s + needle_len so that we
       * don't just find the same extension again and go
       * into an infinite loop.
       */
      haystack = s + needle_len;
   }

   return false;
}

struct virgl_egl *virgl_egl_init(void)
{
   static const EGLint conf_att[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_ALPHA_SIZE, 0,
      EGL_NONE,
   };
   static const EGLint ctx_att[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };
   EGLBoolean b;
   EGLenum api;
   EGLint major, minor, n;
   const char *extension_list;
   struct virgl_egl *d;

   d = malloc(sizeof(struct virgl_egl));
   if (!d)
      return NULL;

   const char *client_extensions = eglQueryString (NULL, EGL_EXTENSIONS);

   d->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

   if (!d->egl_display)
      goto fail;

   b = eglInitialize(d->egl_display, &major, &minor);
   if (!b)
      goto fail;

   extension_list = eglQueryString(d->egl_display, EGL_EXTENSIONS);
#ifdef VIRGL_EGL_DEBUG
   fprintf(stderr, "EGL major/minor: %d.%d\n", major, minor);
   fprintf(stderr, "EGL version: %s\n",
           eglQueryString(d->egl_display, EGL_VERSION));
   fprintf(stderr, "EGL vendor: %s\n",
           eglQueryString(d->egl_display, EGL_VENDOR));
   fprintf(stderr, "EGL extensions: %s\n", extension_list);
#endif
   /* require surfaceless context */
   if (!virgl_egl_has_extension_in_string(extension_list, "EGL_KHR_surfaceless_context"))
      goto fail;

   api = EGL_OPENGL_API;
   b = eglBindAPI(api);
   if (!b)
      goto fail;

   b = eglChooseConfig(d->egl_display, conf_att, &d->egl_conf,
                       1, &n);

   if (!b || n != 1)
      goto fail;

   d->egl_ctx = eglCreateContext(d->egl_display,
                                 d->egl_conf,
                                 EGL_NO_CONTEXT,
                                 ctx_att);
   if (!d->egl_ctx)
      goto fail;


   eglMakeCurrent(d->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                  d->egl_ctx);
   return d;
 fail:
   free(d);
   return NULL;
}

void virgl_egl_destroy(struct virgl_egl *d)
{
   eglMakeCurrent(d->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                  EGL_NO_CONTEXT);
   eglDestroyContext(d->egl_display, d->egl_ctx);
   eglTerminate(d->egl_display);
   free(d);
}

virgl_renderer_gl_context virgl_egl_create_context(struct virgl_egl *ve, struct virgl_gl_ctx_param *vparams)
{
   EGLContext eglctx;
   EGLint ctx_att[] = {
      EGL_CONTEXT_CLIENT_VERSION, vparams->major_ver,
      EGL_CONTEXT_MINOR_VERSION_KHR, vparams->minor_ver,
      EGL_NONE
   };
   eglctx = eglCreateContext(ve->egl_display,
                             ve->egl_conf,
                             vparams->shared ? eglGetCurrentContext() : EGL_NO_CONTEXT,
                             ctx_att);
   return (virgl_renderer_gl_context)eglctx;
}

void virgl_egl_destroy_context(struct virgl_egl *ve, virgl_renderer_gl_context virglctx)
{
   EGLContext eglctx = (EGLContext)virglctx;
   eglDestroyContext(ve->egl_display, eglctx);
}

int virgl_egl_make_context_current(struct virgl_egl *ve, virgl_renderer_gl_context virglctx)
{
   EGLContext eglctx = (EGLContext)virglctx;

   return eglMakeCurrent(ve->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         eglctx);
}

virgl_renderer_gl_context virgl_egl_get_current_context(struct virgl_egl *ve)
{
   EGLContext eglctx = eglGetCurrentContext();
   return (virgl_renderer_gl_context)eglctx;
}

int virgl_egl_get_fourcc_for_texture(struct virgl_egl *ve, uint32_t tex_id, uint32_t format, int *fourcc)
{
   *fourcc = virgl_egl_get_gbm_format(format);
   return 0;
}

uint32_t virgl_egl_get_gbm_format(uint32_t format)
{
   switch (format) {
   case VIRGL_FORMAT_B8G8R8X8_UNORM:
      return GBM_FORMAT_XRGB8888;
   case VIRGL_FORMAT_B8G8R8A8_UNORM:
      return GBM_FORMAT_ARGB8888;
   default:
      fprintf(stderr, "unknown format to convert to GBM %d\n", format);
      return 0;
   }
}
