extern "C" {
#include "hw/misc/goldfish_pipe.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
#include "virgl_hw.h"
}  // extern "C"

enum BackendFlags {
    GFXSTREAM_BACKEND_FLAGS_NO_VK_BIT = 1 << 0,
    GFXSTREAM_BACKEND_FLAGS_EGL2EGL_BIT = 1 << 1,
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

// Get the gfxstream backend render information.
// example:
//      /* Get the render string size */
//      size_t size = 0
//      gfxstream_backend_getrender(nullptr, 0, &size);
//
//      /* add extra space for '\0' */
//      char * buf = malloc(size + 1);
//
//      /* Get the result render string */
//      gfxstream_backend_getrender(buf, size+1, nullptr);
//
// if bufSize is less or equal the render string length, only bufSize-1 char copied.
extern "C" VG_EXPORT void gfxstream_backend_getrender(
      char* buf,
      size_t bufSize,
      size_t* size);
