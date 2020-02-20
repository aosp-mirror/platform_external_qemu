GFXSTREAM_CRATE_LOC=~/virtio-gpu-gfxstream-backend-rs
SRC_LIB_LOC=objs/distribution/emulator/lib64

cp $SRC_LIB_LOC/libgfxstream_backend.so $GFXSTREAM_CRATE_LOC
cp $SRC_LIB_LOC/libandroid-emu-shared.so $GFXSTREAM_CRATE_LOC
cp $SRC_LIB_LOC/libOpenglRender.so $GFXSTREAM_CRATE_LOC
cp $SRC_LIB_LOC/libEGL_translator.so $GFXSTREAM_CRATE_LOC
cp $SRC_LIB_LOC/libGLES_CM_translator.so $GFXSTREAM_CRATE_LOC
cp $SRC_LIB_LOC/libGLES_V2_translator.so $GFXSTREAM_CRATE_LOC
cp $SRC_LIB_LOC/libemugl_common.so $GFXSTREAM_CRATE_LOC
cp $SRC_LIB_LOC/libc++.so.1 $GFXSTREAM_CRATE_LOC
