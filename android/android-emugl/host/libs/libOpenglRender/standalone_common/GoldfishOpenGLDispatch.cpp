#include "GoldfishOpenGLDispatch.h"

#include "android/base/memory/LazyInstance.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"

#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/shared_library.h"

#include <functional>

#include <string.h>

using android::base::LazyInstance;

namespace emugl {

class GoldfishOpenGLStore {
public:
    GoldfishOpenGLStore() {
        const DispatchTables* tables =
            LazyLoadedEGLDispatch::getLibrariesFrom(
                "libEGL_emulation.dylib",
                "libGLESv1_CM_emulation.dylib",
                "libGLESv2_emulation.dylib");
        memcpy(&mTables, tables, sizeof(DispatchTables));
        delete tables;

        emugl::SharedLibrary* eglLib = (emugl::SharedLibrary*)mTables.eglLib;

        mRegisterFunc = (registerFunc_t)(eglLib->findSymbol("registerCustomGLESLibraries"));

        int gles_major_version = 0;
        int gles_minor_version = 0;
        android_initOpenglesEmulation();
        android_startOpenglesRenderer(
                256,
                256,
                0 /* AVD_PHONE */,
                28,
                &gles_major_version,
                &gles_minor_version);
        
        HostPipeDispatch dispatch = make_host_pipe_dispatch();

        if (mRegisterFunc) {
            android_init_opengles_pipe();
            mRegisterFunc(mTables.gles1Lib, mTables.gles2Lib, &dispatch);
        }
    }

    const DispatchTables* tables() const {
        return &mTables;
    }

private:

    typedef void (*registerFunc_t)(void*,void*,void*);

    registerFunc_t mRegisterFunc;
    DispatchTables mTables;
};

static LazyInstance<GoldfishOpenGLStore> sGoldfishOpenGLStore = LAZY_INSTANCE_INIT;

const DispatchTables* loadGoldfishOpenGL() {
    return sGoldfishOpenGLStore->tables();
}

} // namespace emugl
