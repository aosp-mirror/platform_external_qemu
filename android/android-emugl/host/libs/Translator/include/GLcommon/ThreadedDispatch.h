#pragma once

#include "GLcommon/GLLibrary.h"

typedef GlLibrary::GlFunctionPointer GL_FUNC_PTR;

class ThreadedDispatch {
public:
    ThreadedDispatch();
    static GL_FUNC_PTR registerAndGetGLFuncAddress(const char* funcName, GlLibrary* glLib);
};
