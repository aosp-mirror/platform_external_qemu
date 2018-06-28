#include "GoldfishOpenGLDispatch.h"

#include "android/base/memory/LazyInstance.h"

#include "emugl/common/OpenGLDispatchLoader.h"

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
    }

    const DispatchTables* tables() const {
        return &mTables;
    }

private:
    DispatchTables mTables;
};

static LazyInstance<GoldfishOpenGLStore> sGoldfishOpenGLStore = LAZY_INSTANCE_INIT;

const DispatchTables* loadGoldfishOpenGL() {
    return sGoldfishOpenGLStore->tables();
}

} // namespace emugl
