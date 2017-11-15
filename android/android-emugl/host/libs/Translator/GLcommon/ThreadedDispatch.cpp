#include "GLcommon/ThreadedDispatch.h"
#include "GLcommon/ThreadedDispatch.h"

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#include "OpenGLESDispatch/gldefs.h"
#include "OpenGLESDispatch/gles_functions.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/threads/WorkerThread.h"

#ifdef __linux__
#include <GL/glx.h>
#elif defined(WIN32)
#include <windows.h>
#endif

using android::base::LazyInstance;
using android::base::WorkerThread;
using android::base::WorkerProcessingResult;

class GlLibraryCaller {
public:
};

class ThreadedFunctionTable {
};

ThreadedDispatch::ThreadedDispatch() {
    // sTable.get();
}

// static
// GL_FUNC_PTR ThreadedDispatch::registerAndGetGLFuncAddress(const char* funcName, GlLibrary* glLib) {
// }
