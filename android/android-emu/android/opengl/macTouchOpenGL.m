#include "android/opengl/macTouchOpenGL.h"

#include <Cocoa/Cocoa.h>
#include <OpenGL/OpenGL.h>

static NSOpenGLPixelFormatAttribute testAttrs[] =
{
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAWindow,
    NSOpenGLPFAPixelBuffer,
    NSOpenGLPFAColorSize   ,32,
    NSOpenGLPFADepthSize   ,24,
    NSOpenGLPFAStencilSize ,8,
    0
};

// In order for IOKit to know the latest GPU selection status, we need
// to touch OpenGL a little bit. Here, we allocate and initialize a pixel
// format, which seems to be enough.
void macTouchOpenGL() {
    void* res = [[NSOpenGLPixelFormat alloc] initWithAttributes:testAttrs];
    [res release];
}
