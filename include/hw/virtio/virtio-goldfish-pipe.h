#ifndef VIRTIO_GOLDFISH_PIPE
#define VIRTIO_GOLDFISH_PIPE

/* An override of virtio-gpu-3d (virgl) that runs goldfish pipe.  One could
 * implement an actual virtio goldfish pipe, but this hijacking of virgl  is
 * done in order to avoid any guest kernel changes. */

#ifdef CONFIG_VIRGL
#include <virglrenderer.h>

struct virgl_renderer_virtio_interface*
    get_goldfish_pipe_virgl_renderer_virtio_interface();
struct virgl_renderer_callbacks*
    get_goldfish_pipe_virgl_renderer_callbacks();
#endif

#endif
