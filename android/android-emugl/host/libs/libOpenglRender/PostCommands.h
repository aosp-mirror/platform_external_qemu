#pragma once

#include "android/skin/rect.h"

#include <vector>

#include <GLES2/gl2.h>

class ColorBuffer;

// Posting
enum class PostCmd {
    Post = 0,
    Viewport = 1,
    Compose = 2,
    Clear = 3,
    Screenshot = 4,
    Exit = 5,
};

struct Post {
    PostCmd cmd;
    int composeVersion;
    std::vector<char> composeBuffer;
    union {
        ColorBuffer* cb;
        struct {
            int width;
            int height;
        } viewport;
        struct {
            ColorBuffer* cb;
            int screenwidth;
            int screenheight;
            GLenum format;
            GLenum type;
            SkinRotation rotation;
            void* pixels;
            SkinRect rect;
        } screenshot;
    };
};

