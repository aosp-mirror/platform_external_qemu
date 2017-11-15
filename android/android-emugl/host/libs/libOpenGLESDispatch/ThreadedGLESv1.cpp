// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles1.json --custom_prefix=gles1 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles_common.entries

void gles1_glActiveTexture(GLenum texture) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glActiveTexture_underlying(texture);
    });
}


void gles1_glBindBuffer(GLenum target, GLuint buffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBindBuffer_underlying(target, buffer);
    });
}


void gles1_glBindTexture(GLenum target, GLuint texture) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBindTexture_underlying(target, texture);
    });
}


void gles1_glBlendFunc(GLenum sfactor, GLenum dfactor) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBlendFunc_underlying(sfactor, dfactor);
    });
}


void gles1_glBlendEquation(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBlendEquation_underlying(mode);
    });
}


void gles1_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBlendEquationSeparate_underlying(modeRGB, modeAlpha);
    });
}


void gles1_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBlendFuncSeparate_underlying(srcRGB, dstRGB, srcAlpha, dstAlpha);
    });
}


// void gles1_glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles1_dispatch->glBufferData_underlying(target, size, data, usage);
//     });
// }
// 
// 
// void gles1_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles1_dispatch->glBufferSubData_underlying(target, offset, size, data);
//     });
// }


void gles1_glClear(GLbitfield mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClear_underlying(mask);
    });
}


void gles1_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClearColor_underlying(red, green, blue, alpha);
    });
}


void gles1_glClearDepth(GLclampd depth) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClearDepth_underlying(depth);
    });
}


void gles1_glClearDepthf(GLclampf depth) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClearDepthf_underlying(depth);
    });
}


void gles1_glClearStencil(GLint s) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClearStencil_underlying(s);
    });
}


void gles1_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glColorMask_underlying(red, green, blue, alpha);
    });
}


void gles1_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glCompressedTexImage2D_underlying(target, level, internalformat, width, height, border, imageSize, data);
    });
}


void gles1_glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glCompressedTexSubImage2D_underlying(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    });
}


void gles1_glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glCopyTexImage2D_underlying(target, level, internalFormat, x, y, width, height, border);
    });
}


void gles1_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glCopyTexSubImage2D_underlying(target, level, xoffset, yoffset, x, y, width, height);
    });
}


void gles1_glCullFace(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glCullFace_underlying(mode);
    });
}


void gles1_glDeleteBuffers(GLsizei n, const GLuint * buffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDeleteBuffers_underlying(n, buffers);
    });
}


void gles1_glDeleteTextures(GLsizei n, const GLuint * textures) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDeleteTextures_underlying(n, textures);
    });
}


void gles1_glDepthFunc(GLenum func) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDepthFunc_underlying(func);
    });
}


void gles1_glDepthMask(GLboolean flag) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDepthMask_underlying(flag);
    });
}


void gles1_glDepthRange(GLclampd zNear, GLclampd zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDepthRange_underlying(zNear, zFar);
    });
}


void gles1_glDepthRangef(GLclampf zNear, GLclampf zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDepthRangef_underlying(zNear, zFar);
    });
}


void gles1_glDisable(GLenum cap) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDisable_underlying(cap);
    });
}


void gles1_glDrawArrays(GLenum mode, GLint first, GLsizei count) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDrawArrays_underlying(mode, first, count);
    });
}


void gles1_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDrawElements_underlying(mode, count, type, indices);
    });
}


void gles1_glEnable(GLenum cap) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glEnable_underlying(cap);
    });
}


void gles1_glFinish() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFinish_underlying();
    });
}


void gles1_glFlush() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFlush_underlying();
    });
}


void gles1_glFrontFace(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFrontFace_underlying(mode);
    });
}


void gles1_glGenBuffers(GLsizei n, GLuint * buffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGenBuffers_underlying(n, buffers);
    });
}


void gles1_glGenTextures(GLsizei n, GLuint * textures) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGenTextures_underlying(n, textures);
    });
}


void gles1_glGetBooleanv(GLenum pname, GLboolean * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetBooleanv_underlying(pname, params);
    });
}


void gles1_glGetBufferParameteriv(GLenum buffer, GLenum parameter, GLint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetBufferParameteriv_underlying(buffer, parameter, value);
    });
}


GLenum gles1_glGetError() { 
    return DriverThread::callOnDriverThread<GLenum>([&]() {
        return gles1_dispatch->glGetError_underlying();
    });
}


void gles1_glGetFloatv(GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetFloatv_underlying(pname, params);
    });
}


void gles1_glGetIntegerv(GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetIntegerv_underlying(pname, params);
    });
}


GLconstubyteptr gles1_glGetString(GLenum name) { 
    return DriverThread::callOnDriverThread<GLconstubyteptr>([&]() {
        return gles1_dispatch->glGetString_underlying(name);
    });
}


void gles1_glTexParameterf(GLenum target, GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexParameterf_underlying(target, pname, param);
    });
}


void gles1_glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexParameterfv_underlying(target, pname, params);
    });
}


void gles1_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexImage_underlying(target, level, format, type, pixels);
    });
}


void gles1_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexParameterfv_underlying(target, pname, params);
    });
}


void gles1_glGetTexParameteriv(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexParameteriv_underlying(target, pname, params);
    });
}


void gles1_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexLevelParameteriv_underlying(target, level, pname, params);
    });
}


void gles1_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexLevelParameterfv_underlying(target, level, pname, params);
    });
}


void gles1_glHint(GLenum target, GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glHint_underlying(target, mode);
    });
}


GLboolean gles1_glIsBuffer(GLuint buffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles1_dispatch->glIsBuffer_underlying(buffer);
    });
}


GLboolean gles1_glIsEnabled(GLenum cap) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles1_dispatch->glIsEnabled_underlying(cap);
    });
}


GLboolean gles1_glIsTexture(GLuint texture) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles1_dispatch->glIsTexture_underlying(texture);
    });
}


void gles1_glLineWidth(GLfloat width) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLineWidth_underlying(width);
    });
}


void gles1_glPolygonOffset(GLfloat factor, GLfloat units) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPolygonOffset_underlying(factor, units);
    });
}


void gles1_glPixelStorei(GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPixelStorei_underlying(pname, param);
    });
}


void gles1_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glReadPixels_underlying(x, y, width, height, format, type, pixels);
    });
}


void gles1_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glRenderbufferStorageMultisample_underlying(target, samples, internalformat, width, height);
    });
}


void gles1_glSampleCoverage(GLclampf value, GLboolean invert) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glSampleCoverage_underlying(value, invert);
    });
}


void gles1_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glScissor_underlying(x, y, width, height);
    });
}


void gles1_glStencilFunc(GLenum func, GLint ref, GLuint mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glStencilFunc_underlying(func, ref, mask);
    });
}


void gles1_glStencilMask(GLuint mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glStencilMask_underlying(mask);
    });
}


void gles1_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glStencilOp_underlying(fail, zfail, zpass);
    });
}


void gles1_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexImage2D_underlying(target, level, internalformat, width, height, border, format, type, pixels);
    });
}


void gles1_glTexParameteri(GLenum target, GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexParameteri_underlying(target, pname, param);
    });
}


void gles1_glTexParameteriv(GLenum target, GLenum pname, const GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexParameteriv_underlying(target, pname, params);
    });
}


void gles1_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexSubImage2D_underlying(target, level, xoffset, yoffset, width, height, format, type, pixels);
    });
}


void gles1_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glViewport_underlying(x, y, width, height);
    });
}


void gles1_glPushAttrib(GLbitfield mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPushAttrib_underlying(mask);
    });
}


void gles1_glPushClientAttrib(GLbitfield mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPushClientAttrib_underlying(mask);
    });
}


void gles1_glPopAttrib() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPopAttrib_underlying();
    });
}


void gles1_glPopClientAttrib() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPopClientAttrib_underlying();
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles1.json --custom_prefix=gles1 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles_extensions.entries

GLboolean gles1_glIsRenderbufferEXT(GLuint renderbuffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles1_dispatch->glIsRenderbufferEXT_underlying(renderbuffer);
    });
}


void gles1_glBindRenderbufferEXT(GLenum target, GLuint renderbuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBindRenderbufferEXT_underlying(target, renderbuffer);
    });
}


void gles1_glDeleteRenderbuffersEXT(GLsizei n, const GLuint * renderbuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDeleteRenderbuffersEXT_underlying(n, renderbuffers);
    });
}


void gles1_glGenRenderbuffersEXT(GLsizei n, GLuint * renderbuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGenRenderbuffersEXT_underlying(n, renderbuffers);
    });
}


void gles1_glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glRenderbufferStorageEXT_underlying(target, internalformat, width, height);
    });
}


void gles1_glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetRenderbufferParameterivEXT_underlying(target, pname, params);
    });
}


GLboolean gles1_glIsFramebufferEXT(GLuint framebuffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles1_dispatch->glIsFramebufferEXT_underlying(framebuffer);
    });
}


void gles1_glBindFramebufferEXT(GLenum target, GLuint framebuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBindFramebufferEXT_underlying(target, framebuffer);
    });
}


void gles1_glDeleteFramebuffersEXT(GLsizei n, const GLuint * framebuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDeleteFramebuffersEXT_underlying(n, framebuffers);
    });
}


void gles1_glGenFramebuffersEXT(GLsizei n, GLuint * framebuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGenFramebuffersEXT_underlying(n, framebuffers);
    });
}


GLenum gles1_glCheckFramebufferStatusEXT(GLenum target) { 
    return DriverThread::callOnDriverThread<GLenum>([&]() {
        return gles1_dispatch->glCheckFramebufferStatusEXT_underlying(target);
    });
}


void gles1_glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFramebufferTexture1DEXT_underlying(target, attachment, textarget, texture, level);
    });
}


void gles1_glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFramebufferTexture2DEXT_underlying(target, attachment, textarget, texture, level);
    });
}


void gles1_glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFramebufferTexture3DEXT_underlying(target, attachment, textarget, texture, level, zoffset);
    });
}


void gles1_glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFramebufferRenderbufferEXT_underlying(target, attachment, renderbuffertarget, renderbuffer);
    });
}


void gles1_glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetFramebufferAttachmentParameterivEXT_underlying(target, attachment, pname, params);
    });
}


void gles1_glGenerateMipmapEXT(GLenum target) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glGenerateMipmapEXT_underlying(target);
    });
}


void gles1_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glEGLImageTargetTexture2DOES_underlying(target, image);
    });
}


void gles1_glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glEGLImageTargetRenderbufferStorageOES_underlying(target, image);
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles1.json --custom_prefix=gles1 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles1_only.entries

void gles1_glAlphaFunc(GLenum func, GLclampf ref) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glAlphaFunc_underlying(func, ref);
    });
}


void gles1_glBegin(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glBegin_underlying(mode);
    });
}


void gles1_glClientActiveTexture(GLenum texture) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClientActiveTexture_underlying(texture);
    });
}


void gles1_glClipPlane(GLenum plane, const GLdouble * equation) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glClipPlane_underlying(plane, equation);
    });
}


void gles1_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glColor4d_underlying(red, green, blue, alpha);
    });
}


void gles1_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glColor4f_underlying(red, green, blue, alpha);
    });
}


void gles1_glColor4fv(const GLfloat * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glColor4fv_underlying(v);
    });
}


void gles1_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glColor4ub_underlying(red, green, blue, alpha);
    });
}


void gles1_glColor4ubv(const GLubyte * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glColor4ubv_underlying(v);
    });
}


void gles1_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glColorPointer_underlying(size, type, stride, pointer);
    });
}


void gles1_glDisableClientState(GLenum array) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDisableClientState_underlying(array);
    });
}


void gles1_glEnableClientState(GLenum array) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glEnableClientState_underlying(array);
    });
}


void gles1_glEnd() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glEnd_underlying();
    });
}


void gles1_glFogf(GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFogf_underlying(pname, param);
    });
}


void gles1_glFogfv(GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glFogfv_underlying(pname, params);
    });
}


void gles1_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFrustum_underlying(left, right, bottom, top, zNear, zFar);
    });
}


void gles1_glGetClipPlane(GLenum plane, GLdouble * equation) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetClipPlane_underlying(plane, equation);
    });
}


void gles1_glGetDoublev(GLenum pname, GLdouble * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetDoublev_underlying(pname, params);
    });
}


void gles1_glGetLightfv(GLenum light, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetLightfv_underlying(light, pname, params);
    });
}


void gles1_glGetMaterialfv(GLenum face, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetMaterialfv_underlying(face, pname, params);
    });
}


void gles1_glGetPointerv(GLenum pname, GLvoid* * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetPointerv_underlying(pname, params);
    });
}


void gles1_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexEnvfv_underlying(target, pname, params);
    });
}


void gles1_glGetTexEnviv(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexEnviv_underlying(target, pname, params);
    });
}


void gles1_glLightf(GLenum light, GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLightf_underlying(light, pname, param);
    });
}


void gles1_glLightfv(GLenum light, GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glLightfv_underlying(light, pname, params);
    });
}


void gles1_glLightModelf(GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLightModelf_underlying(pname, param);
    });
}


void gles1_glLightModelfv(GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glLightModelfv_underlying(pname, params);
    });
}


void gles1_glLoadIdentity() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLoadIdentity_underlying();
    });
}


void gles1_glLoadMatrixf(const GLfloat * m) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glLoadMatrixf_underlying(m);
    });
}


void gles1_glLogicOp(GLenum opcode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLogicOp_underlying(opcode);
    });
}


void gles1_glMaterialf(GLenum face, GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glMaterialf_underlying(face, pname, param);
    });
}


void gles1_glMaterialfv(GLenum face, GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMaterialfv_underlying(face, pname, params);
    });
}


void gles1_glMultiTexCoord2fv(GLenum target, const GLfloat * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultiTexCoord2fv_underlying(target, v);
    });
}


void gles1_glMultiTexCoord2sv(GLenum target, const GLshort * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultiTexCoord2sv_underlying(target, v);
    });
}


void gles1_glMultiTexCoord3fv(GLenum target, const GLfloat * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultiTexCoord3fv_underlying(target, v);
    });
}


void gles1_glMultiTexCoord3sv(GLenum target, const GLshort * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultiTexCoord3sv_underlying(target, v);
    });
}


void gles1_glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glMultiTexCoord4f_underlying(target, s, t, r, q);
    });
}


void gles1_glMultiTexCoord4fv(GLenum target, const GLfloat * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultiTexCoord4fv_underlying(target, v);
    });
}


void gles1_glMultiTexCoord4sv(GLenum target, const GLshort * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultiTexCoord4sv_underlying(target, v);
    });
}


void gles1_glMultMatrixf(const GLfloat * m) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultMatrixf_underlying(m);
    });
}


void gles1_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glNormal3f_underlying(nx, ny, nz);
    });
}


void gles1_glNormal3fv(const GLfloat * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glNormal3fv_underlying(v);
    });
}


void gles1_glNormal3sv(const GLshort * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glNormal3sv_underlying(v);
    });
}


void gles1_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glOrtho_underlying(left, right, bottom, top, zNear, zFar);
    });
}


void gles1_glPointParameterf(GLenum param, GLfloat value) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPointParameterf_underlying(param, value);
    });
}


void gles1_glPointParameterfv(GLenum param, const GLfloat * values) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glPointParameterfv_underlying(param, values);
    });
}


void gles1_glPointSize(GLfloat size) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPointSize_underlying(size);
    });
}


void gles1_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glRotatef_underlying(angle, x, y, z);
    });
}


void gles1_glScalef(GLfloat x, GLfloat y, GLfloat z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glScalef_underlying(x, y, z);
    });
}


void gles1_glTexEnvf(GLenum target, GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexEnvf_underlying(target, pname, param);
    });
}


void gles1_glTexEnvfv(GLenum target, GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexEnvfv_underlying(target, pname, params);
    });
}


void gles1_glMatrixMode(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glMatrixMode_underlying(mode);
    });
}


void gles1_glNormalPointer(GLenum type, GLsizei stride, const GLvoid * pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glNormalPointer_underlying(type, stride, pointer);
    });
}


void gles1_glPopMatrix() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPopMatrix_underlying();
    });
}


void gles1_glPushMatrix() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPushMatrix_underlying();
    });
}


void gles1_glShadeModel(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glShadeModel_underlying(mode);
    });
}


void gles1_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexCoordPointer_underlying(size, type, stride, pointer);
    });
}


void gles1_glTexEnvi(GLenum target, GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexEnvi_underlying(target, pname, param);
    });
}


void gles1_glTexEnviv(GLenum target, GLenum pname, const GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexEnviv_underlying(target, pname, params);
    });
}


void gles1_glTranslatef(GLfloat x, GLfloat y, GLfloat z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTranslatef_underlying(x, y, z);
    });
}


void gles1_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glVertexPointer_underlying(size, type, stride, pointer);
    });
}


void gles1_glTexEnvx(GLenum target, GLenum pname, GLfixed param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexEnvx_underlying(target, pname, param);
    });
}


void gles1_glTexEnvxv(GLenum target, GLenum pname, const GLfixed* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexEnvxv_underlying(target, pname, params);
    });
}


void gles1_glGetTexEnvxv(GLenum env, GLenum pname, GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexEnvxv_underlying(env, pname, params);
    });
}


void gles1_glTexParameterx(GLenum target, GLenum pname, GLfixed param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexParameterx_underlying(target, pname, param);
    });
}


void gles1_glTexParameterxv(GLenum target, GLenum pname, const GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexParameterxv_underlying(target, pname, params);
    });
}


void gles1_glGetTexParameterxv(GLenum target, GLenum pname, GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexParameterxv_underlying(target, pname, params);
    });
}


void gles1_glFogx(GLenum pname, GLfixed param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFogx_underlying(pname, param);
    });
}


void gles1_glFogxv(GLenum pname, const GLfixed* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glFogxv_underlying(pname, params);
    });
}


void gles1_glGetLightxv(GLenum light, GLenum pname, GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetLightxv_underlying(light, pname, params);
    });
}


void gles1_glGetMaterialxv(GLenum face, GLenum pname, GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetMaterialxv_underlying(face, pname, params);
    });
}


void gles1_glLightModelx(GLenum pname, GLfixed param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLightModelx_underlying(pname, param);
    });
}


void gles1_glLightModelxv(GLenum pname, const GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glLightModelxv_underlying(pname, params);
    });
}


void gles1_glLightx(GLenum light, GLenum pname, GLfixed param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLightx_underlying(light, pname, param);
    });
}


void gles1_glLightxv(GLenum light, GLenum pname, const GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glLightxv_underlying(light, pname, params);
    });
}


void gles1_glMaterialx(GLenum face, GLenum pname, GLfixed param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glMaterialx_underlying(face, pname, param);
    });
}


void gles1_glMaterialxv(GLenum face, GLenum pname, const GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMaterialxv_underlying(face, pname, params);
    });
}


void gles1_glPointParameterx(GLenum pname, GLfixed param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPointParameterx_underlying(pname, param);
    });
}


void gles1_glPointParameterxv(GLenum pname, const GLfixed * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glPointParameterxv_underlying(pname, params);
    });
}


void gles1_glLineWidthx(GLfixed width) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glLineWidthx_underlying(width);
    });
}


void gles1_glLoadMatrixx(const GLfixed * m) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glLoadMatrixx_underlying(m);
    });
}


void gles1_glMultMatrixx(const GLfixed * m) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMultMatrixx_underlying(m);
    });
}


void gles1_glMultiTexCoord4x(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glMultiTexCoord4x_underlying(target, s, t, r, q);
    });
}


void gles1_glNormal3x(GLfixed nx, GLfixed ny, GLfixed nz) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glNormal3x_underlying(nx, ny, nz);
    });
}


void gles1_glOrthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glOrthox_underlying(left, right, bottom, top, zNear, zFar);
    });
}


void gles1_glPointSizex(GLfixed size) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPointSizex_underlying(size);
    });
}


void gles1_glPolygonOffsetx(GLfixed factor, GLfixed units) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glPolygonOffsetx_underlying(factor, units);
    });
}


void gles1_glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glRotatex_underlying(angle, x, y, z);
    });
}


void gles1_glSampleCoveragex(GLclampx value, GLboolean invert) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glSampleCoveragex_underlying(value, invert);
    });
}


void gles1_glScalex(GLfixed x, GLfixed y, GLfixed z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glScalex_underlying(x, y, z);
    });
}


void gles1_glTranslatex(GLfixed x, GLfixed y, GLfixed z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTranslatex_underlying(x, y, z);
    });
}


void gles1_glAlphaFuncx(GLenum func, GLclampx ref) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glAlphaFuncx_underlying(func, ref);
    });
}


void gles1_glClearColorx(GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClearColorx_underlying(red, green, blue, alpha);
    });
}


void gles1_glClearDepthx(GLclampx depth) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glClearDepthx_underlying(depth);
    });
}


void gles1_glClipPlanex(GLenum plane, const GLfixed * equation) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glClipPlanex_underlying(plane, equation);
    });
}


void gles1_glColor4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glColor4x_underlying(red, green, blue, alpha);
    });
}


void gles1_glDepthRangex(GLclampx zNear, GLclampx zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDepthRangex_underlying(zNear, zFar);
    });
}


void gles1_glFrustumx(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glFrustumx_underlying(left, right, bottom, top, zNear, zFar);
    });
}


void gles1_glDrawTexsOES(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDrawTexsOES_underlying(x, y, z, width, height);
    });
}


void gles1_glDrawTexiOES(GLint x, GLint y, GLint z, GLint width, GLint height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDrawTexiOES_underlying(x, y, z, width, height);
    });
}


void gles1_glDrawTexfOES(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDrawTexfOES_underlying(x, y, z, width, height);
    });
}


void gles1_glDrawTexxOES(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glDrawTexxOES_underlying(x, y, z, width, height);
    });
}


void gles1_glDrawTexsvOES(const GLshort * coords) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDrawTexsvOES_underlying(coords);
    });
}


void gles1_glDrawTexivOES(const GLint * coords) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDrawTexivOES_underlying(coords);
    });
}


void gles1_glDrawTexfvOES(const GLfloat * coords) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDrawTexfvOES_underlying(coords);
    });
}


void gles1_glDrawTexxvOES(const GLfixed * coords) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glDrawTexxvOES_underlying(coords);
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles1.json --custom_prefix=gles1 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles1_extensions.entries

void gles1_glCurrentPaletteMatrixARB(GLint index) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glCurrentPaletteMatrixARB_underlying(index);
    });
}


void gles1_glMatrixIndexuivARB(GLint size, GLuint * indices) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMatrixIndexuivARB_underlying(size, indices);
    });
}


void gles1_glMatrixIndexPointerARB(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glMatrixIndexPointerARB_underlying(size, type, stride, pointer);
    });
}


void gles1_glWeightPointerARB(GLint size, GLenum type, GLsizei stride, const GLvoid* pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glWeightPointerARB_underlying(size, type, stride, pointer);
    });
}


void gles1_glTexGenf(GLenum coord, GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexGenf_underlying(coord, pname, param);
    });
}


void gles1_glTexGeni(GLenum coord, GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles1_dispatch->glTexGeni_underlying(coord, pname, param);
    });
}


void gles1_glTexGenfv(GLenum coord, GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexGenfv_underlying(coord, pname, params);
    });
}


void gles1_glTexGeniv(GLenum coord, GLenum pname, const GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexGeniv_underlying(coord, pname, params);
    });
}


void gles1_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexGenfv_underlying(coord, pname, params);
    });
}


void gles1_glGetTexGeniv(GLenum coord, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glGetTexGeniv_underlying(coord, pname, params);
    });
}


void gles1_glColorPointerWithDataSize(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, GLsizei dataSize) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glColorPointerWithDataSize_underlying(size, type, stride, pointer, dataSize);
    });
}


void gles1_glNormalPointerWithDataSize(GLenum type, GLsizei stride, const GLvoid * pointer, GLsizei dataSize) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glNormalPointerWithDataSize_underlying(type, stride, pointer, dataSize);
    });
}


void gles1_glTexCoordPointerWithDataSize(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, GLsizei dataSize) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glTexCoordPointerWithDataSize_underlying(size, type, stride, pointer, dataSize);
    });
}


void gles1_glVertexPointerWithDataSize(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer, GLsizei dataSize) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles1_dispatch->glVertexPointerWithDataSize_underlying(size, type, stride, pointer, dataSize);
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles2.json --custom_prefix=gles2 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles_extensions.entries

GLboolean gles2_glIsRenderbufferEXT(GLuint renderbuffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsRenderbufferEXT_underlying(renderbuffer);
    });
}


void gles2_glBindRenderbufferEXT(GLenum target, GLuint renderbuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindRenderbufferEXT_underlying(target, renderbuffer);
    });
}


void gles2_glDeleteRenderbuffersEXT(GLsizei n, const GLuint * renderbuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteRenderbuffersEXT_underlying(n, renderbuffers);
    });
}


void gles2_glGenRenderbuffersEXT(GLsizei n, GLuint * renderbuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenRenderbuffersEXT_underlying(n, renderbuffers);
    });
}


void gles2_glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glRenderbufferStorageEXT_underlying(target, internalformat, width, height);
    });
}


void gles2_glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetRenderbufferParameterivEXT_underlying(target, pname, params);
    });
}


GLboolean gles2_glIsFramebufferEXT(GLuint framebuffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsFramebufferEXT_underlying(framebuffer);
    });
}


void gles2_glBindFramebufferEXT(GLenum target, GLuint framebuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindFramebufferEXT_underlying(target, framebuffer);
    });
}


void gles2_glDeleteFramebuffersEXT(GLsizei n, const GLuint * framebuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteFramebuffersEXT_underlying(n, framebuffers);
    });
}


void gles2_glGenFramebuffersEXT(GLsizei n, GLuint * framebuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenFramebuffersEXT_underlying(n, framebuffers);
    });
}


GLenum gles2_glCheckFramebufferStatusEXT(GLenum target) { 
    return DriverThread::callOnDriverThread<GLenum>([&]() {
        return gles2_dispatch->glCheckFramebufferStatusEXT_underlying(target);
    });
}


void gles2_glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferTexture1DEXT_underlying(target, attachment, textarget, texture, level);
    });
}


void gles2_glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferTexture2DEXT_underlying(target, attachment, textarget, texture, level);
    });
}


void gles2_glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferTexture3DEXT_underlying(target, attachment, textarget, texture, level, zoffset);
    });
}


void gles2_glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferRenderbufferEXT_underlying(target, attachment, renderbuffertarget, renderbuffer);
    });
}


void gles2_glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetFramebufferAttachmentParameterivEXT_underlying(target, attachment, pname, params);
    });
}


void gles2_glGenerateMipmapEXT(GLenum target) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glGenerateMipmapEXT_underlying(target);
    });
}


void gles2_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glEGLImageTargetTexture2DOES_underlying(target, image);
    });
}


void gles2_glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glEGLImageTargetRenderbufferStorageOES_underlying(target, image);
    });
}

