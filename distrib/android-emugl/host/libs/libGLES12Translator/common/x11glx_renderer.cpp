#include "common/x11glx_renderer.h"

#include <cstdlib>

namespace emugl {
    X11Renderer::X11Renderer() {

        int x11_params_width = 1080;
        int x11_params_height = 1920;
        int x11_params_display_density = 300;
        int x11_params_vsync_period = 60;

        x11_dpy = XOpenDisplay(NULL);

        if(x11_dpy == NULL) { 
            printf("can't connect to X server\n");
            exit(1);
        }

        x11_root = DefaultRootWindow(x11_dpy);

        XVisualInfo xvi_template;
        int num_vis;
        x11_vi = XGetVisualInfo(x11_dpy, VisualIDMask, &xvi_template, &num_vis);
        x11_cmap = XCreateColormap(x11_dpy, RootWindow(x11_dpy, 0), x11_vi->visual, AllocNone);

        // set window attrs
        x11_swa.colormap = x11_cmap;
        x11_swa.event_mask = ExposureMask | KeyPressMask;

        x11_win = XCreateWindow(x11_dpy, x11_root, 0, 0, x11_params_width, x11_params_height, 0, x11_vi->depth, InputOutput, x11_vi->visual, CWColormap | CWEventMask, &x11_swa);
    }

void X11Renderer::GetRenderParams(RenderParams* params) const {
    params->width = x11_params_width;
    params->height = x11_params_height;
    params->display_density = x11_params_display_density;
    params->vsync_period = x11_params_vsync_period;
    params->device_render_to_view_pixels = 0.f;
    params->crx_render_to_view_pixels = 0.f;
}

glxContextGPU* X11Renderer::CreateContext(const Attributes& attribs, glxContextGPU* shared_context) {
  // turn attribs into a fbconfig object
  int* fbconfig_attribs = new int[attribs.size()];
  for (int i = 0; i < attribs.size(); i++) {
      fbconfig_attribs[i] = attribs[i];
  }

  int n;
  GLXFBConfig* fbs = glXChooseFBConfig(x11_dpy, 0, fbconfig_attribs, &n);
  if (n < 1) {
      delete [] fbconfig_attribs;
      return NULL;
  }

  GLXContext glxc = glXCreateNewContext(x11_dpy, fbs[0], GLX_RGBA_TYPE, (shared_context != NULL) ? shared_context->glxc : NULL, true);
  glxContextGPU* res = new glxContextGPU(glxc, shared_context->surf);

  delete [] fbconfig_attribs;
  return res;
}

// This function is still a mystery.
bool X11Renderer::BindContext(glxContextGPU* context) {
    return true;
}


bool X11Renderer::SwapBuffers(glxContextGPU* context) {
    glXSwapBuffers(x11_dpy, context->surf);
    return true;
}

void X11Renderer:: DestroyContext(glxContextGPU* context) {
    // FIXME: implement DestroyContext
    return;
}



} // namespace egl
