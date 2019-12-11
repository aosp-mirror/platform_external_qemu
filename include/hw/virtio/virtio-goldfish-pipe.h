#ifndef VIRTIO_GOLDFISH_PIPE
#define VIRTIO_GOLDFISH_PIPE

/* An override of virtio-gpu-3d (virgl) that runs goldfish pipe.  One could
 * implement an actual virtio goldfish pipe, but this hijacking of virgl  is
 * done in order to avoid any guest kernel changes. */

#include <virglrenderer.h>

#ifdef __cplusplus
extern "C" {
#endif
struct virgl_renderer_virtio_interface*
    get_goldfish_pipe_virgl_renderer_virtio_interface(void);
struct virgl_renderer_callbacks*
    get_goldfish_pipe_virgl_renderer_callbacks(void);

/* Needed for goldfish pipe */
void virgl_write_fence(void *opaque, uint32_t fence);

void virtio_goldfish_pipe_reset(void* hwpipe, void* hostpipe);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
