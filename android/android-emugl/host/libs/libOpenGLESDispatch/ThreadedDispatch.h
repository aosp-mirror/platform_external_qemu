#pragma once

class EGLDispatch;
class GLESv1Dispatch;
class GLESv2Dispatch;

void init_egl_threaded_dispatch(EGLDispatch* table);
void init_gles1_threaded_dispatch(GLESv1Dispatch* table);
void init_gles2_threaded_dispatch(GLESv2Dispatch* table);
