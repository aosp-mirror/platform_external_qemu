#include "GrallocDispatch.h"

#include "emugl/common/shared_library.h"

void init_gralloc_dispatch(const char* libName, GrallocDispatch* dispatch_out, void* libOut) {

    auto lib = emugl::SharedLibrary::open(libName);

    if (!lib) {
        fprintf(stderr, "%s: could not find gralloc library\n", __func__);
        return;
    }

    *(void**)libOut = lib;

    memset(dispatch_out, 0, sizeof(GrallocDispatch));

    dispatch_out->open_device = (gralloc_open_device_t)lib->findSymbol("grallocOpenDevice");
    dispatch_out->alloc = (gralloc_alloc_t)lib->findSymbol("grallocAlloc");
    dispatch_out->free = (gralloc_free_t)lib->findSymbol("grallocFree");
    dispatch_out->register_buffer = (gralloc_register_t)lib->findSymbol("grallocRegisterBuffer");
    dispatch_out->unregister_buffer = (gralloc_unregister_t)lib->findSymbol("grallocUnregisterBuffer");
    dispatch_out->lock = (gralloc_lock_t)lib->findSymbol("grallocLock");
    dispatch_out->unlock = (gralloc_unlock_t)lib->findSymbol("grallocUnlock");
    dispatch_out->fbpost = (gralloc_unlock_t)lib->findSymbol("fbPost");

    if (!dispatch_out->open_device) {
        fprintf(stderr, "%s: could not find grallocOpenDevice\n", __func__);
    }

    if (!dispatch_out->alloc) {
        fprintf(stderr, "%s: could not find grallocAlloc\n", __func__);
    }

    if (!dispatch_out->free) {
        fprintf(stderr, "%s: could not find grallocFree\n", __func__);
    }

    if (!dispatch_out->register_buffer) {
        fprintf(stderr, "%s: could not find grallocRegisterBuffer\n", __func__);
    }

    if (!dispatch_out->unregister_buffer) {
        fprintf(stderr, "%s: could not find grallocUnregisterBuffer\n", __func__);
    }

    if (!dispatch_out->lock) {
        fprintf(stderr, "%s: could not find grallocLock\n", __func__);
    }

    if (!dispatch_out->unlock) {
        fprintf(stderr, "%s: could not find grallocUnlock\n", __func__);
    }

    if (!dispatch_out->fbpost) {
        fprintf(stderr, "%s: could not find fbpost\n", __func__);
    }
}

