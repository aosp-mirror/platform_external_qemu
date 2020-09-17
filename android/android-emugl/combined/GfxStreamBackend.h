extern "C" {
#include "hw/misc/goldfish_pipe.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
#include "virgl_hw.h"
}  // extern "C"

enum BackendFlags {
    GFXSTREAM_BACKEND_FLAGS_NO_VK_BIT = 1 << 0,
    GFXSTREAM_BACKEND_FLAGS_EGL2EGL_BIT = 1 << 1,
};

// based on VIRGL_RENDERER_USE* and friends
enum RendererFlags {
    GFXSTREAM_RENDERER_FLAGS_USE_EGL_BIT = 1 << 0,
    GFXSTREAM_RENDERER_FLAGS_THREAD_SYNC = 1 << 1,
    GFXSTREAM_RENDERER_FLAGS_USE_GLX_BIT = 1 << 2,
    GFXSTREAM_RENDERER_FLAGS_USE_SURFACELESS_BIT = 1 << 3,
    GFXSTREAM_RENDERER_FLAGS_USE_GLES_BIT = 1 << 4,
    GFXSTREAM_RENDERER_FLAGS_NO_VK_BIT = 1 << 5, // for disabling vk

    GFXSTREAM_RENDERER_FLAGS_NO_SYNCFD_BIT = 1 << 20, // for disabling syncfd
};

extern "C" VG_EXPORT void gfxstream_backend_init(
    uint32_t display_width,
    uint32_t display_height,
    uint32_t display_type,
    void* renderer_cookie,
    int renderer_flags,
    struct virgl_renderer_callbacks* virglrenderer_callbacks);

extern "C" VG_EXPORT void gfxstream_backend_setup_window(
        void* native_window_handle,
        int32_t window_x,
        int32_t window_y,
        int32_t window_width,
        int32_t window_height,
        int32_t fb_width,
        int32_t fb_height);

extern "C" VG_EXPORT void gfxstream_backend_teardown(void);