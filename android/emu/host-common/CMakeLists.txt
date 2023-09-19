# host-common header library
add_library(qemu-host-common-headers INTERFACE)
target_include_directories(qemu-host-common-headers INTERFACE include)
target_link_libraries(
    qemu-host-common-headers
    INTERFACE
    aemu-host-common.headers
    gfxstream-gl-host-common.headers)
if(OPTION_GFXSTREAM_BACKEND)
    target_link_libraries(
        qemu-host-common-headers
        INTERFACE
        gfxstream_backend_headers)
endif()
target_compile_options(qemu-host-common-headers INTERFACE -Wno-extern-c-compat)
android_license(TARGET "qemu-host-common-headers" LIBNAME None SPDX None LICENSE None LOCAL None)
