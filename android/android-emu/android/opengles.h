/* Copyright (C) 2011 The Android Open Source Project
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

#pragma once

#include <stddef.h>

#include "android/base/export.h"
#include "android/emulation/control/multi_display_agent.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/opengl/virtio_gpu_ops.h"
#include "android/utils/compiler.h"

#ifdef __cplusplus
#include "OpenglRender/Renderer.h"
#endif

ANDROID_BEGIN_HEADER

/* Call this function to initialize the hardware opengles emulation.
 * This function will abort if we can't find the corresponding host
 * libraries through dlopen() or equivalent.
 */
AEMU_EXPORT int android_initOpenglesEmulation(void);

/* Tries to start the renderer process. Returns 0 on success, -1 on error.
 * At the moment, this must be done before the VM starts. The onPost callback
 * may be NULL.
 *
 * width and height: the framebuffer dimensions that will be reported
 *                   to the guest display driver.
 * guestApiLevel: API level of guest image (23 for mnc, 24 for nyc, etc)
 */
AEMU_EXPORT int android_startOpenglesRenderer(int width, int height,
                                              bool isPhone, int guestApiLevel,
                                              const QAndroidVmOperations *vm_operations,
                                              const QAndroidEmulatorWindowAgent *window_agent,
                                              const QAndroidMultiDisplayAgent *multi_display_agent,
                                              int* glesMajorVersion_out,
                                              int* glesMinorVersion_out);

AEMU_EXPORT bool android_asyncReadbackSupported();

/* See the description in render_api.h. */
typedef void (*OnPostFunc)(void* context, uint32_t displayId, int width,
                           int height, int ydir, int format, int type,
                           unsigned char* pixels);
AEMU_EXPORT void android_setPostCallback(OnPostFunc onPost,
                             void* onPostContext,
                             bool useBgraReadback,
                             uint32_t displayId);

typedef void (*ReadPixelsFunc)(void* pixels, uint32_t bytes, uint32_t displayId);
AEMU_EXPORT ReadPixelsFunc android_getReadPixelsFunc();


typedef void (*FlushReadPixelPipeline)(int displayId);

/* Gets the function that can be used to make sure no
 * frames are left in the video producer pipeline.
 * This can result in a post callback.
 */
FlushReadPixelPipeline android_getFlushReadPixelPipeline();

/* Retrieve the Vendor/Renderer/Version strings describing the underlying GL
 * implementation. The call only works while the renderer is started.
 *
 * Expects |*vendor|, |*renderer| and |*version| to be NULL.
 *
 * On exit, sets |*vendor|, |*renderer| and |*version| to point to new
 * heap-allocated strings (that must be freed by the caller) which represent the
 * OpenGL hardware vendor name, driver name and version, respectively.
 * In case of error, |*vendor| etc. are set to NULL.
 */
AEMU_EXPORT void android_getOpenglesHardwareStrings(char** vendor,
                                                    char** renderer,
                                                    char** version);

AEMU_EXPORT int android_showOpenglesWindow(void* window,
                                           int wx,
                                           int wy,
                                           int ww,
                                           int wh,
                                           int fbw,
                                           int fbh,
                                           float dpr,
                                           float rotation,
                                           bool deleteExisting,
                                           bool hideWindow);

AEMU_EXPORT int android_hideOpenglesWindow(void);

AEMU_EXPORT void android_setOpenglesTranslation(float px, float py);

AEMU_EXPORT void android_setOpenglesScreenMask(int width, int height, const unsigned char* rgbaData);

AEMU_EXPORT void android_redrawOpenglesWindow(void);

AEMU_EXPORT bool android_hasGuestPostedAFrame(void);
AEMU_EXPORT void android_resetGuestPostedAFrame(void);

typedef bool (*ScreenshotFunc)(const char* dirname, uint32_t displayId);
AEMU_EXPORT void android_registerScreenshotFunc(ScreenshotFunc f);
AEMU_EXPORT bool android_screenShot(const char* dirname, uint32_t displayId);

/* Stop the renderer process */
AEMU_EXPORT void android_stopOpenglesRenderer(bool wait);

/* Finish all renderer work, deleting current
 * render threads. Renderer is allowed to get
 * new render threads after that. */
AEMU_EXPORT void android_finishOpenglesRenderer();

/* set to TRUE if you want to use fast GLES pipes, 0 if you want to
 * fallback to local TCP ones
 */
AEMU_EXPORT extern int  android_gles_fast_pipes;

AEMU_EXPORT void android_cleanupProcGLObjects(uint64_t puid);

#ifdef __cplusplus
AEMU_EXPORT const emugl::RendererPtr& android_getOpenglesRenderer();
#endif

AEMU_EXPORT struct AndroidVirtioGpuOps* android_getVirtioGpuOps(void);

/* Get EGL/GLESv2 dispatch tables */
AEMU_EXPORT const void* android_getEGLDispatch();
AEMU_EXPORT const void* android_getGLESv2Dispatch();

ANDROID_END_HEADER
