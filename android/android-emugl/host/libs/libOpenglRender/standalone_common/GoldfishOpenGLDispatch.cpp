#include "GoldfishOpenGLDispatch.h"

#include "HostGoldfishSync.h"
#include "Standalone.h"

#include "android/base/memory/LazyInstance.h"
#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/emulation/testing/TestVmLock.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"

#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/shared_library.h"

#include "GrallocDispatch.h"

#include <functional>
#include <string.h>

using android::base::LazyInstance;

class GoldfishTables {
public:
    DispatchTables eglGles;
    GrallocDispatch gralloc;
    void* grallocLib;
};

namespace emugl {

class GoldfishOpenGLStore {
public:
    GoldfishOpenGLStore() {
        setupStandaloneLibrarySearchPaths();

        auto egl = LazyLoadedEGLDispatch::get();
        egl->eglUseOsEglApi(!shouldUseHostGpu());

        LazyLoadedGLESv2Dispatch::get();
        bool useHostGpu = shouldUseHostGpu();
        FrameBuffer::initialize(
                256, 256,
                false,
                !useHostGpu /* egl2egl */);
        
        
        android::TestVmLock* testVmLock = android::TestVmLock::getInstance();
        HostPipeDispatch dispatch = make_host_pipe_dispatch();
        host_goldfish_sync_init();
        android_init_opengles_pipe();

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

        const DispatchTables* tables =
            LazyLoadedEGLDispatch::getLibrariesFrom(
                "libEGL_emulation.dylib",
                "libGLESv1_CM_emulation.dylib",
                "libGLESv2_emulation.dylib", &dispatch);
        memcpy(&mTables.eglGles, tables, sizeof(DispatchTables));
        delete tables;

        // emugl::SharedLibrary* eglLib = (emugl::SharedLibrary*)mTables.eglGles.eglLib; 
        // mRegisterFunc = (registerFunc_t)(eglLib->findSymbol("registerCustomGLESLibraries"));

        // if (mRegisterFunc) {
            // mRegisterFunc(mTables.eglGles.gles1Lib, mTables.eglGles.gles2Lib, &dispatch);
        // }

        init_gralloc_dispatch("gralloc_ranchu.dylib", &mTables.gralloc, &mTables.grallocLib);

        mRegisterFuncGralloc = (registerFuncGralloc_t)(((emugl::SharedLibrary*)mTables.grallocLib)->findSymbol("registerPipeDispatch"));

        if (mRegisterFuncGralloc) {
            mRegisterFuncGralloc(&dispatch);
        }
    }

    const GoldfishTables* tables() const {
        return &mTables;
    }

private:

    typedef void (*registerFunc_t)(void*,void*,void*);
    typedef void (*registerFuncGralloc_t)(void*);

    registerFunc_t mRegisterFunc;
    registerFuncGralloc_t mRegisterFuncGralloc;
    GoldfishTables mTables;
};

static LazyInstance<GoldfishOpenGLStore> sGoldfishOpenGLStore = LAZY_INSTANCE_INIT;

const DispatchTables* loadGoldfishOpenGL() {
    return &sGoldfishOpenGLStore->tables()->eglGles;
}

const GrallocDispatch* loadGoldfishGralloc() {
    return &sGoldfishOpenGLStore->tables()->gralloc;
}

} // namespace emugl
