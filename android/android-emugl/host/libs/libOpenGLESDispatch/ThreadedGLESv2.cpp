// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles2.json --custom_prefix=gles2 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles_common.entries

void gles2_glActiveTexture(GLenum texture) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glActiveTexture_underlying(texture);
    });
}


void gles2_glBindBuffer(GLenum target, GLuint buffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindBuffer_underlying(target, buffer);
    });
}


void gles2_glBindTexture(GLenum target, GLuint texture) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindTexture_underlying(target, texture);
    });
}


void gles2_glBlendFunc(GLenum sfactor, GLenum dfactor) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBlendFunc_underlying(sfactor, dfactor);
    });
}


void gles2_glBlendEquation(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBlendEquation_underlying(mode);
    });
}


void gles2_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBlendEquationSeparate_underlying(modeRGB, modeAlpha);
    });
}


void gles2_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBlendFuncSeparate_underlying(srcRGB, dstRGB, srcAlpha, dstAlpha);
    });
}


// void gles2_glBufferData(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glBufferData_underlying(target, size, data, usage);
//     });
// }
// 
// 
// void gles2_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glBufferSubData_underlying(target, offset, size, data);
//     });
// }


void gles2_glClear(GLbitfield mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glClear_underlying(mask);
    });
}


void gles2_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glClearColor_underlying(red, green, blue, alpha);
    });
}


void gles2_glClearDepth(GLclampd depth) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glClearDepth_underlying(depth);
    });
}


void gles2_glClearDepthf(GLclampf depth) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glClearDepthf_underlying(depth);
    });
}


void gles2_glClearStencil(GLint s) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glClearStencil_underlying(s);
    });
}


void gles2_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glColorMask_underlying(red, green, blue, alpha);
    });
}


void gles2_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glCompressedTexImage2D_underlying(target, level, internalformat, width, height, border, imageSize, data);
    });
}


void gles2_glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glCompressedTexSubImage2D_underlying(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    });
}


void gles2_glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glCopyTexImage2D_underlying(target, level, internalFormat, x, y, width, height, border);
    });
}


void gles2_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glCopyTexSubImage2D_underlying(target, level, xoffset, yoffset, x, y, width, height);
    });
}


void gles2_glCullFace(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glCullFace_underlying(mode);
    });
}


void gles2_glDeleteBuffers(GLsizei n, const GLuint * buffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteBuffers_underlying(n, buffers);
    });
}


void gles2_glDeleteTextures(GLsizei n, const GLuint * textures) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteTextures_underlying(n, textures);
    });
}


void gles2_glDepthFunc(GLenum func) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDepthFunc_underlying(func);
    });
}


void gles2_glDepthMask(GLboolean flag) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDepthMask_underlying(flag);
    });
}


void gles2_glDepthRange(GLclampd zNear, GLclampd zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDepthRange_underlying(zNear, zFar);
    });
}


void gles2_glDepthRangef(GLclampf zNear, GLclampf zFar) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDepthRangef_underlying(zNear, zFar);
    });
}


void gles2_glDisable(GLenum cap) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDisable_underlying(cap);
    });
}


void gles2_glDrawArrays(GLenum mode, GLint first, GLsizei count) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDrawArrays_underlying(mode, first, count);
    });
}


void gles2_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDrawElements_underlying(mode, count, type, indices);
    });
}


void gles2_glEnable(GLenum cap) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glEnable_underlying(cap);
    });
}


void gles2_glFinish() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFinish_underlying();
    });
}


void gles2_glFlush() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFlush_underlying();
    });
}


void gles2_glFrontFace(GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFrontFace_underlying(mode);
    });
}


void gles2_glGenBuffers(GLsizei n, GLuint * buffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenBuffers_underlying(n, buffers);
    });
}


void gles2_glGenTextures(GLsizei n, GLuint * textures) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenTextures_underlying(n, textures);
    });
}


void gles2_glGetBooleanv(GLenum pname, GLboolean * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetBooleanv_underlying(pname, params);
    });
}


void gles2_glGetBufferParameteriv(GLenum buffer, GLenum parameter, GLint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetBufferParameteriv_underlying(buffer, parameter, value);
    });
}


GLenum gles2_glGetError() { 
    return DriverThread::callOnDriverThread<GLenum>([&]() {
        return gles2_dispatch->glGetError_underlying();
    });
}


void gles2_glGetFloatv(GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetFloatv_underlying(pname, params);
    });
}


void gles2_glGetIntegerv(GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetIntegerv_underlying(pname, params);
    });
}


GLconstubyteptr gles2_glGetString(GLenum name) { 
    return DriverThread::callOnDriverThread<GLconstubyteptr>([&]() {
        return gles2_dispatch->glGetString_underlying(name);
    });
}


void gles2_glTexParameterf(GLenum target, GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glTexParameterf_underlying(target, pname, param);
    });
}


void gles2_glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glTexParameterfv_underlying(target, pname, params);
    });
}


void gles2_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetTexImage_underlying(target, level, format, type, pixels);
    });
}


void gles2_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetTexParameterfv_underlying(target, pname, params);
    });
}


void gles2_glGetTexParameteriv(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetTexParameteriv_underlying(target, pname, params);
    });
}


void gles2_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetTexLevelParameteriv_underlying(target, level, pname, params);
    });
}


void gles2_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetTexLevelParameterfv_underlying(target, level, pname, params);
    });
}


void gles2_glHint(GLenum target, GLenum mode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glHint_underlying(target, mode);
    });
}


GLboolean gles2_glIsBuffer(GLuint buffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsBuffer_underlying(buffer);
    });
}


GLboolean gles2_glIsEnabled(GLenum cap) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsEnabled_underlying(cap);
    });
}


GLboolean gles2_glIsTexture(GLuint texture) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsTexture_underlying(texture);
    });
}


void gles2_glLineWidth(GLfloat width) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glLineWidth_underlying(width);
    });
}


void gles2_glPolygonOffset(GLfloat factor, GLfloat units) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glPolygonOffset_underlying(factor, units);
    });
}


void gles2_glPixelStorei(GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glPixelStorei_underlying(pname, param);
    });
}


void gles2_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glReadPixels_underlying(x, y, width, height, format, type, pixels);
    });
}


void gles2_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glRenderbufferStorageMultisample_underlying(target, samples, internalformat, width, height);
    });
}


void gles2_glSampleCoverage(GLclampf value, GLboolean invert) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glSampleCoverage_underlying(value, invert);
    });
}


void gles2_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glScissor_underlying(x, y, width, height);
    });
}


void gles2_glStencilFunc(GLenum func, GLint ref, GLuint mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glStencilFunc_underlying(func, ref, mask);
    });
}


void gles2_glStencilMask(GLuint mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glStencilMask_underlying(mask);
    });
}


void gles2_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glStencilOp_underlying(fail, zfail, zpass);
    });
}


void gles2_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glTexImage2D_underlying(target, level, internalformat, width, height, border, format, type, pixels);
    });
}


void gles2_glTexParameteri(GLenum target, GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glTexParameteri_underlying(target, pname, param);
    });
}


void gles2_glTexParameteriv(GLenum target, GLenum pname, const GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glTexParameteriv_underlying(target, pname, params);
    });
}


void gles2_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glTexSubImage2D_underlying(target, level, xoffset, yoffset, width, height, format, type, pixels);
    });
}


void gles2_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glViewport_underlying(x, y, width, height);
    });
}


void gles2_glPushAttrib(GLbitfield mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glPushAttrib_underlying(mask);
    });
}


void gles2_glPushClientAttrib(GLbitfield mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glPushClientAttrib_underlying(mask);
    });
}


void gles2_glPopAttrib() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glPopAttrib_underlying();
    });
}


void gles2_glPopClientAttrib() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glPopClientAttrib_underlying();
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles2.json --custom_prefix=gles2 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles2_only.entries

void gles2_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBlendColor_underlying(red, green, blue, alpha);
    });
}


void gles2_glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glStencilFuncSeparate_underlying(face, func, ref, mask);
    });
}


void gles2_glStencilMaskSeparate(GLenum face, GLuint mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glStencilMaskSeparate_underlying(face, mask);
    });
}


void gles2_glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glStencilOpSeparate_underlying(face, fail, zfail, zpass);
    });
}


GLboolean gles2_glIsProgram(GLuint program) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsProgram_underlying(program);
    });
}


GLboolean gles2_glIsShader(GLuint shader) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsShader_underlying(shader);
    });
}


void gles2_glVertexAttrib1f(GLuint indx, GLfloat x) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib1f_underlying(indx, x);
    });
}


// void gles2_glVertexAttrib1fv(GLuint indx, const GLfloat* values) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glVertexAttrib1fv_underlying(indx, values);
//     });
// }


void gles2_glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib2f_underlying(indx, x, y);
    });
}


// void gles2_glVertexAttrib2fv(GLuint indx, const GLfloat* values) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glVertexAttrib2fv_underlying(indx, values);
//     });
// }


void gles2_glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib3f_underlying(indx, x, y, z);
    });
}


// void gles2_glVertexAttrib3fv(GLuint indx, const GLfloat* values) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glVertexAttrib3fv_underlying(indx, values);
//     });
// }


void gles2_glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttrib4f_underlying(indx, x, y, z, w);
    });
}


// void gles2_glVertexAttrib4fv(GLuint indx, const GLfloat* values) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glVertexAttrib4fv_underlying(indx, values);
//     });
// }


void gles2_glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glVertexAttribPointer_underlying(indx, size, type, normalized, stride, ptr);
    });
}


void gles2_glDisableVertexAttribArray(GLuint index) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDisableVertexAttribArray_underlying(index);
    });
}


void gles2_glEnableVertexAttribArray(GLuint index) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glEnableVertexAttribArray_underlying(index);
    });
}


void gles2_glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetVertexAttribfv_underlying(index, pname, params);
    });
}


void gles2_glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetVertexAttribiv_underlying(index, pname, params);
    });
}


void gles2_glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetVertexAttribPointerv_underlying(index, pname, pointer);
    });
}


void gles2_glUniform1f(GLint location, GLfloat x) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform1f_underlying(location, x);
    });
}


// void gles2_glUniform1fv(GLint location, GLsizei count, const GLfloat* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform1fv_underlying(location, count, v);
//     });
// }


void gles2_glUniform1i(GLint location, GLint x) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform1i_underlying(location, x);
    });
}


// void gles2_glUniform1iv(GLint location, GLsizei count, const GLint* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform1iv_underlying(location, count, v);
//     });
// }


void gles2_glUniform2f(GLint location, GLfloat x, GLfloat y) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform2f_underlying(location, x, y);
    });
}


// void gles2_glUniform2fv(GLint location, GLsizei count, const GLfloat* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform2fv_underlying(location, count, v);
//     });
// }


void gles2_glUniform2i(GLint location, GLint x, GLint y) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform2i_underlying(location, x, y);
    });
}


// void gles2_glUniform2iv(GLint location, GLsizei count, const GLint* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform2iv_underlying(location, count, v);
//     });
// }


void gles2_glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform3f_underlying(location, x, y, z);
    });
}


// void gles2_glUniform3fv(GLint location, GLsizei count, const GLfloat* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform3fv_underlying(location, count, v);
//     });
// }


void gles2_glUniform3i(GLint location, GLint x, GLint y, GLint z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform3i_underlying(location, x, y, z);
    });
}


// void gles2_glUniform3iv(GLint location, GLsizei count, const GLint* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform3iv_underlying(location, count, v);
//     });
// }


void gles2_glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform4f_underlying(location, x, y, z, w);
    });
}


// void gles2_glUniform4fv(GLint location, GLsizei count, const GLfloat* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform4fv_underlying(location, count, v);
//     });
// }


void gles2_glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform4i_underlying(location, x, y, z, w);
    });
}


// void gles2_glUniform4iv(GLint location, GLsizei count, const GLint* v) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniform4iv_underlying(location, count, v);
//     });
// }


// void gles2_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniformMatrix2fv_underlying(location, count, transpose, value);
//     });
// }
// 
// 
// void gles2_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniformMatrix3fv_underlying(location, count, transpose, value);
//     });
// }
// 
// 
// void gles2_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glUniformMatrix4fv_underlying(location, count, transpose, value);
//     });
// }


void gles2_glAttachShader(GLuint program, GLuint shader) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glAttachShader_underlying(program, shader);
    });
}


void gles2_glBindAttribLocation(GLuint program, GLuint index, const GLchar* name) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glBindAttribLocation_underlying(program, index, name);
    });
}


void gles2_glCompileShader(GLuint shader) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glCompileShader_underlying(shader);
    });
}


GLuint gles2_glCreateProgram() { 
    return DriverThread::callOnDriverThread<GLuint>([&]() {
        return gles2_dispatch->glCreateProgram_underlying();
    });
}


GLuint gles2_glCreateShader(GLenum type) { 
    return DriverThread::callOnDriverThread<GLuint>([&]() {
        return gles2_dispatch->glCreateShader_underlying(type);
    });
}


void gles2_glDeleteProgram(GLuint program) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDeleteProgram_underlying(program);
    });
}


void gles2_glDeleteShader(GLuint shader) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDeleteShader_underlying(shader);
    });
}


void gles2_glDetachShader(GLuint program, GLuint shader) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDetachShader_underlying(program, shader);
    });
}


void gles2_glLinkProgram(GLuint program) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glLinkProgram_underlying(program);
    });
}


void gles2_glUseProgram(GLuint program) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUseProgram_underlying(program);
    });
}


void gles2_glValidateProgram(GLuint program) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glValidateProgram_underlying(program);
    });
}


void gles2_glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetActiveAttrib_underlying(program, index, bufsize, length, size, type, name);
    });
}


void gles2_glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetActiveUniform_underlying(program, index, bufsize, length, size, type, name);
    });
}


void gles2_glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetAttachedShaders_underlying(program, maxcount, count, shaders);
    });
}


int gles2_glGetAttribLocation(GLuint program, const GLchar* name) { 
    return DriverThread::callOnDriverThread<int>([&]() {
        return gles2_dispatch->glGetAttribLocation_underlying(program, name);
    });
}


void gles2_glGetProgramiv(GLuint program, GLenum pname, GLint* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramiv_underlying(program, pname, params);
    });
}


void gles2_glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramInfoLog_underlying(program, bufsize, length, infolog);
    });
}


void gles2_glGetShaderiv(GLuint shader, GLenum pname, GLint* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetShaderiv_underlying(shader, pname, params);
    });
}


void gles2_glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetShaderInfoLog_underlying(shader, bufsize, length, infolog);
    });
}


void gles2_glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetShaderSource_underlying(shader, bufsize, length, source);
    });
}


void gles2_glGetUniformfv(GLuint program, GLint location, GLfloat* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetUniformfv_underlying(program, location, params);
    });
}


void gles2_glGetUniformiv(GLuint program, GLint location, GLint* params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetUniformiv_underlying(program, location, params);
    });
}


int gles2_glGetUniformLocation(GLuint program, const GLchar* name) { 
    return DriverThread::callOnDriverThread<int>([&]() {
        return gles2_dispatch->glGetUniformLocation_underlying(program, name);
    });
}


void gles2_glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glShaderSource_underlying(shader, count, string, length);
    });
}


void gles2_glBindFramebuffer(GLenum target, GLuint framebuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindFramebuffer_underlying(target, framebuffer);
    });
}


void gles2_glGenFramebuffers(GLsizei n, GLuint* framebuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenFramebuffers_underlying(n, framebuffers);
    });
}


void gles2_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferTexture2D_underlying(target, attachment, textarget, texture, level);
    });
}


GLenum gles2_glCheckFramebufferStatus(GLenum target) { 
    return DriverThread::callOnDriverThread<GLenum>([&]() {
        return gles2_dispatch->glCheckFramebufferStatus_underlying(target);
    });
}


GLboolean gles2_glIsFramebuffer(GLuint framebuffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsFramebuffer_underlying(framebuffer);
    });
}


void gles2_glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteFramebuffers_underlying(n, framebuffers);
    });
}


GLboolean gles2_glIsRenderbuffer(GLuint renderbuffer) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsRenderbuffer_underlying(renderbuffer);
    });
}


void gles2_glBindRenderbuffer(GLenum target, GLuint renderbuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindRenderbuffer_underlying(target, renderbuffer);
    });
}


void gles2_glDeleteRenderbuffers(GLsizei n, const GLuint * renderbuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteRenderbuffers_underlying(n, renderbuffers);
    });
}


void gles2_glGenRenderbuffers(GLsizei n, GLuint * renderbuffers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenRenderbuffers_underlying(n, renderbuffers);
    });
}


void gles2_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glRenderbufferStorage_underlying(target, internalformat, width, height);
    });
}


void gles2_glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetRenderbufferParameteriv_underlying(target, pname, params);
    });
}


void gles2_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferRenderbuffer_underlying(target, attachment, renderbuffertarget, renderbuffer);
    });
}


void gles2_glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetFramebufferAttachmentParameteriv_underlying(target, attachment, pname, params);
    });
}


void gles2_glGenerateMipmap(GLenum target) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glGenerateMipmap_underlying(target);
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles2.json --custom_prefix=gles2 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles2_extensions.entries

void gles2_glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetShaderPrecisionFormat_underlying(shadertype, precisiontype, range, precision);
    });
}


void gles2_glReleaseShaderCompiler() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glReleaseShaderCompiler_underlying();
    });
}


void gles2_glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glShaderBinary_underlying(n, shaders, binaryformat, binary, length);
    });
}


void gles2_glVertexAttribPointerWithDataSize(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr, GLsizei dataSize) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glVertexAttribPointerWithDataSize_underlying(indx, size, type, normalized, stride, ptr, dataSize);
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles2.json --custom_prefix=gles2 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles3_only.entries

GLconstubyteptr gles2_glGetStringi(GLenum name, GLint index) { 
    return DriverThread::callOnDriverThread<GLconstubyteptr>([&]() {
        return gles2_dispatch->glGetStringi_underlying(name, index);
    });
}


void gles2_glGenVertexArrays(GLsizei n, GLuint* arrays) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenVertexArrays_underlying(n, arrays);
    });
}


void gles2_glBindVertexArray(GLuint array) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindVertexArray_underlying(array);
    });
}


void gles2_glDeleteVertexArrays(GLsizei n, const GLuint * arrays) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteVertexArrays_underlying(n, arrays);
    });
}


GLboolean gles2_glIsVertexArray(GLuint array) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsVertexArray_underlying(array);
    });
}


void * gles2_glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) { 
    return DriverThread::callOnDriverThread<void *>([&]() {
        return gles2_dispatch->glMapBufferRange_underlying(target, offset, length, access);
    });
}


GLboolean gles2_glUnmapBuffer(GLenum target) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glUnmapBuffer_underlying(target);
    });
}


void gles2_glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFlushMappedBufferRange_underlying(target, offset, length);
    });
}


void gles2_glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindBufferRange_underlying(target, index, buffer, offset, size);
    });
}


void gles2_glBindBufferBase(GLenum target, GLuint index, GLuint buffer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindBufferBase_underlying(target, index, buffer);
    });
}


void gles2_glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glCopyBufferSubData_underlying(readtarget, writetarget, readoffset, writeoffset, size);
    });
}


void gles2_glClearBufferiv(GLenum buffer, GLint drawBuffer, const GLint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glClearBufferiv_underlying(buffer, drawBuffer, value);
    });
}


void gles2_glClearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glClearBufferuiv_underlying(buffer, drawBuffer, value);
    });
}


void gles2_glClearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glClearBufferfv_underlying(buffer, drawBuffer, value);
    });
}


void gles2_glClearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glClearBufferfi_underlying(buffer, drawBuffer, depth, stencil);
    });
}


void gles2_glGetBufferParameteri64v(GLenum target, GLenum value, GLint64 * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetBufferParameteri64v_underlying(target, value, data);
    });
}


void gles2_glGetBufferPointerv(GLenum target, GLenum pname, GLvoid ** params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetBufferPointerv_underlying(target, pname, params);
    });
}


void gles2_glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniformBlockBinding_underlying(program, uniformBlockIndex, uniformBlockBinding);
    });
}


GLuint gles2_glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName) { 
    return DriverThread::callOnDriverThread<GLuint>([&]() {
        return gles2_dispatch->glGetUniformBlockIndex_underlying(program, uniformBlockName);
    });
}


void gles2_glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar ** uniformNames, GLuint * uniformIndices) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetUniformIndices_underlying(program, uniformCount, uniformNames, uniformIndices);
    });
}


void gles2_glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetActiveUniformBlockiv_underlying(program, uniformBlockIndex, pname, params);
    });
}


void gles2_glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetActiveUniformBlockName_underlying(program, uniformBlockIndex, bufSize, length, uniformBlockName);
    });
}


void gles2_glUniform1ui(GLint location, GLuint v0) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform1ui_underlying(location, v0);
    });
}


void gles2_glUniform2ui(GLint location, GLuint v0, GLuint v1) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform2ui_underlying(location, v0, v1);
    });
}


void gles2_glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform3ui_underlying(location, v0, v1, v2);
    });
}


void gles2_glUniform4ui(GLint location, GLint v0, GLuint v1, GLuint v2, GLuint v3) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUniform4ui_underlying(location, v0, v1, v2, v3);
    });
}


void gles2_glUniform1uiv(GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniform1uiv_underlying(location, count, value);
    });
}


void gles2_glUniform2uiv(GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniform2uiv_underlying(location, count, value);
    });
}


void gles2_glUniform3uiv(GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniform3uiv_underlying(location, count, value);
    });
}


void gles2_glUniform4uiv(GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniform4uiv_underlying(location, count, value);
    });
}


void gles2_glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniformMatrix2x3fv_underlying(location, count, transpose, value);
    });
}


void gles2_glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniformMatrix3x2fv_underlying(location, count, transpose, value);
    });
}


void gles2_glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniformMatrix2x4fv_underlying(location, count, transpose, value);
    });
}


void gles2_glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniformMatrix4x2fv_underlying(location, count, transpose, value);
    });
}


void gles2_glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniformMatrix3x4fv_underlying(location, count, transpose, value);
    });
}


void gles2_glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glUniformMatrix4x3fv_underlying(location, count, transpose, value);
    });
}


void gles2_glGetUniformuiv(GLuint program, GLint location, GLuint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetUniformuiv_underlying(program, location, params);
    });
}


void gles2_glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetActiveUniformsiv_underlying(program, uniformCount, uniformIndices, pname, params);
    });
}


void gles2_glVertexAttribI4i(GLuint index, GLint v0, GLint v1, GLint v2, GLint v3) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttribI4i_underlying(index, v0, v1, v2, v3);
    });
}


void gles2_glVertexAttribI4ui(GLuint index, GLuint v0, GLuint v1, GLuint v2, GLuint v3) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttribI4ui_underlying(index, v0, v1, v2, v3);
    });
}


void gles2_glVertexAttribI4iv(GLuint index, const GLint * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glVertexAttribI4iv_underlying(index, v);
    });
}


void gles2_glVertexAttribI4uiv(GLuint index, const GLuint * v) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glVertexAttribI4uiv_underlying(index, v);
    });
}


void gles2_glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glVertexAttribIPointer_underlying(index, size, type, stride, pointer);
    });
}


void gles2_glGetVertexAttribIiv(GLuint index, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetVertexAttribIiv_underlying(index, pname, params);
    });
}


void gles2_glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetVertexAttribIuiv_underlying(index, pname, params);
    });
}


void gles2_glVertexAttribDivisor(GLuint index, GLuint divisor) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttribDivisor_underlying(index, divisor);
    });
}


void gles2_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDrawArraysInstanced_underlying(mode, first, count, primcount);
    });
}


void gles2_glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei primcount) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDrawElementsInstanced_underlying(mode, count, type, indices, primcount);
    });
}


void gles2_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDrawRangeElements_underlying(mode, start, end, count, type, indices);
    });
}


GLsync gles2_glFenceSync(GLenum condition, GLbitfield flags) { 
    return DriverThread::callOnDriverThread<GLsync>([&]() {
        return gles2_dispatch->glFenceSync_underlying(condition, flags);
    });
}


GLenum gles2_glClientWaitSync(GLsync wait_on, GLbitfield flags, GLuint64 timeout) { 
    return DriverThread::callOnDriverThread<GLenum>([&]() {
        return gles2_dispatch->glClientWaitSync_underlying(wait_on, flags, timeout);
    });
}


void gles2_glWaitSync(GLsync wait_on, GLbitfield flags, GLuint64 timeout) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glWaitSync_underlying(wait_on, flags, timeout);
    });
}


void gles2_glDeleteSync(GLsync to_delete) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDeleteSync_underlying(to_delete);
    });
}


GLboolean gles2_glIsSync(GLsync sync) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsSync_underlying(sync);
    });
}


void gles2_glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei * length, GLint * values) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetSynciv_underlying(sync, pname, bufSize, length, values);
    });
}


void gles2_glDrawBuffers(GLsizei n, const GLenum * bufs) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDrawBuffers_underlying(n, bufs);
    });
}


void gles2_glReadBuffer(GLenum src) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glReadBuffer_underlying(src);
    });
}


void gles2_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBlitFramebuffer_underlying(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    });
}


void gles2_glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glInvalidateFramebuffer_underlying(target, numAttachments, attachments);
    });
}


void gles2_glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glInvalidateSubFramebuffer_underlying(target, numAttachments, attachments, x, y, width, height);
    });
}


void gles2_glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferTextureLayer_underlying(target, attachment, texture, level, layer);
    });
}


void gles2_glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetInternalformativ_underlying(target, internalformat, pname, bufSize, params);
    });
}


void gles2_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glTexStorage2D_underlying(target, levels, internalformat, width, height);
    });
}


void gles2_glBeginTransformFeedback(GLenum primitiveMode) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBeginTransformFeedback_underlying(primitiveMode);
    });
}


void gles2_glEndTransformFeedback() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glEndTransformFeedback_underlying();
    });
}


void gles2_glGenTransformFeedbacks(GLsizei n, GLuint * ids) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenTransformFeedbacks_underlying(n, ids);
    });
}


void gles2_glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteTransformFeedbacks_underlying(n, ids);
    });
}


void gles2_glBindTransformFeedback(GLenum target, GLuint id) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindTransformFeedback_underlying(target, id);
    });
}


void gles2_glPauseTransformFeedback() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glPauseTransformFeedback_underlying();
    });
}


void gles2_glResumeTransformFeedback() { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glResumeTransformFeedback_underlying();
    });
}


GLboolean gles2_glIsTransformFeedback(GLuint id) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsTransformFeedback_underlying(id);
    });
}


void gles2_glTransformFeedbackVaryings(GLuint program, GLsizei count, const char ** varyings, GLenum bufferMode) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glTransformFeedbackVaryings_underlying(program, count, varyings, bufferMode);
    });
}


void gles2_glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, char * name) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetTransformFeedbackVarying_underlying(program, index, bufSize, length, size, type, name);
    });
}


void gles2_glGenSamplers(GLsizei n, GLuint * samplers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenSamplers_underlying(n, samplers);
    });
}


void gles2_glDeleteSamplers(GLsizei n, const GLuint * samplers) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteSamplers_underlying(n, samplers);
    });
}


void gles2_glBindSampler(GLuint unit, GLuint sampler) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindSampler_underlying(unit, sampler);
    });
}


void gles2_glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glSamplerParameterf_underlying(sampler, pname, param);
    });
}


void gles2_glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glSamplerParameteri_underlying(sampler, pname, param);
    });
}


void gles2_glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glSamplerParameterfv_underlying(sampler, pname, params);
    });
}


void gles2_glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glSamplerParameteriv_underlying(sampler, pname, params);
    });
}


void gles2_glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetSamplerParameterfv_underlying(sampler, pname, params);
    });
}


void gles2_glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetSamplerParameteriv_underlying(sampler, pname, params);
    });
}


GLboolean gles2_glIsSampler(GLuint sampler) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsSampler_underlying(sampler);
    });
}


void gles2_glGenQueries(GLsizei n, GLuint * queries) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenQueries_underlying(n, queries);
    });
}


void gles2_glDeleteQueries(GLsizei n, const GLuint * queries) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteQueries_underlying(n, queries);
    });
}


void gles2_glBeginQuery(GLenum target, GLuint query) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBeginQuery_underlying(target, query);
    });
}


void gles2_glEndQuery(GLenum target) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glEndQuery_underlying(target);
    });
}


void gles2_glGetQueryiv(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetQueryiv_underlying(target, pname, params);
    });
}


void gles2_glGetQueryObjectuiv(GLuint query, GLenum pname, GLuint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetQueryObjectuiv_underlying(query, pname, params);
    });
}


GLboolean gles2_glIsQuery(GLuint query) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsQuery_underlying(query);
    });
}


void gles2_glProgramParameteri(GLuint program, GLenum pname, GLint value) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramParameteri_underlying(program, pname, value);
    });
}


void gles2_glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramBinary_underlying(program, binaryFormat, binary, length);
    });
}


void gles2_glGetProgramBinary(GLuint program, GLsizei bufsize, GLsizei * length, GLenum * binaryFormat, void * binary) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramBinary_underlying(program, bufsize, length, binaryFormat, binary);
    });
}


GLint gles2_glGetFragDataLocation(GLuint program, const char * name) { 
    return DriverThread::callOnDriverThread<GLint>([&]() {
        return gles2_dispatch->glGetFragDataLocation_underlying(program, name);
    });
}


void gles2_glGetInteger64v(GLenum pname, GLint64 * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetInteger64v_underlying(pname, data);
    });
}


void gles2_glGetIntegeri_v(GLenum target, GLuint index, GLint * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetIntegeri_v_underlying(target, index, data);
    });
}


void gles2_glGetInteger64i_v(GLenum target, GLuint index, GLint64 * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetInteger64i_v_underlying(target, index, data);
    });
}


void gles2_glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glTexImage3D_underlying(target, level, internalFormat, width, height, depth, border, format, type, data);
    });
}


void gles2_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glTexStorage3D_underlying(target, levels, internalformat, width, height, depth);
    });
}


void gles2_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glTexSubImage3D_underlying(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
    });
}


void gles2_glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glCompressedTexImage3D_underlying(target, level, internalformat, width, height, depth, border, imageSize, data);
    });
}


void gles2_glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glCompressedTexSubImage3D_underlying(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    });
}


void gles2_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glCopyTexSubImage3D_underlying(target, level, xoffset, yoffset, zoffset, x, y, width, height);
    });
}

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles2.json --custom_prefix=gles2 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles3_extensions.entries

// void gles2_glVertexAttribIPointerWithDataSize(GLuint indx, GLint size, GLenum type, GLsizei stride, const GLvoid* ptr, GLsizei dataSize) { 
//     DriverThread::callOnDriverThreadSync([&]() {
//         gles2_dispatch->glVertexAttribIPointerWithDataSize_underlying(indx, size, type, stride, ptr, dataSize);
//     });
// }


// void gles2_glPrimitiveRestartIndex(GLuint index) { 
//     DriverThread::callOnDriverThreadAsync<void>([=]() {
//         gles2_dispatch->glPrimitiveRestartIndex_underlying(index);
//     });
// }

// Auto-generated with: android/scripts/gen-entries.py --gldefs=android/android-emu/android/opengl/defs/gles2.json --custom_prefix=gles2 --mode=threaded_impl android/android-emugl/host/libs/libOpenGLESDispatch/gles31_only.entries

void gles2_glGetBooleani_v(GLenum target, GLuint index, GLboolean * data) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetBooleani_v_underlying(target, index, data);
    });
}


void gles2_glMemoryBarrier(GLbitfield barriers) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glMemoryBarrier_underlying(barriers);
    });
}


void gles2_glMemoryBarrierByRegion(GLbitfield barriers) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glMemoryBarrierByRegion_underlying(barriers);
    });
}


void gles2_glGenProgramPipelines(GLsizei n, GLuint * pipelines) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGenProgramPipelines_underlying(n, pipelines);
    });
}


void gles2_glDeleteProgramPipelines(GLsizei n, const GLuint * pipelines) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDeleteProgramPipelines_underlying(n, pipelines);
    });
}


void gles2_glBindProgramPipeline(GLuint pipeline) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindProgramPipeline_underlying(pipeline);
    });
}


void gles2_glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramPipelineiv_underlying(pipeline, pname, params);
    });
}


void gles2_glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei * length, GLchar * infoLog) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramPipelineInfoLog_underlying(pipeline, bufSize, length, infoLog);
    });
}


void gles2_glValidateProgramPipeline(GLuint pipeline) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glValidateProgramPipeline_underlying(pipeline);
    });
}


GLboolean gles2_glIsProgramPipeline(GLuint pipeline) { 
    return DriverThread::callOnDriverThread<GLboolean>([&]() {
        return gles2_dispatch->glIsProgramPipeline_underlying(pipeline);
    });
}


void gles2_glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glUseProgramStages_underlying(pipeline, stages, program);
    });
}


void gles2_glActiveShaderProgram(GLuint pipeline, GLuint program) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glActiveShaderProgram_underlying(pipeline, program);
    });
}


GLuint gles2_glCreateShaderProgramv(GLenum type, GLsizei count, const char ** strings) { 
    return DriverThread::callOnDriverThread<GLuint>([&]() {
        return gles2_dispatch->glCreateShaderProgramv_underlying(type, count, strings);
    });
}


void gles2_glProgramUniform1f(GLuint program, GLint location, GLfloat v0) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform1f_underlying(program, location, v0);
    });
}


void gles2_glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform2f_underlying(program, location, v0, v1);
    });
}


void gles2_glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform3f_underlying(program, location, v0, v1, v2);
    });
}


void gles2_glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform4f_underlying(program, location, v0, v1, v2, v3);
    });
}


void gles2_glProgramUniform1i(GLuint program, GLint location, GLint v0) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform1i_underlying(program, location, v0);
    });
}


void gles2_glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform2i_underlying(program, location, v0, v1);
    });
}


void gles2_glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform3i_underlying(program, location, v0, v1, v2);
    });
}


void gles2_glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform4i_underlying(program, location, v0, v1, v2, v3);
    });
}


void gles2_glProgramUniform1ui(GLuint program, GLint location, GLuint v0) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform1ui_underlying(program, location, v0);
    });
}


void gles2_glProgramUniform2ui(GLuint program, GLint location, GLint v0, GLuint v1) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform2ui_underlying(program, location, v0, v1);
    });
}


void gles2_glProgramUniform3ui(GLuint program, GLint location, GLint v0, GLint v1, GLuint v2) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform3ui_underlying(program, location, v0, v1, v2);
    });
}


void gles2_glProgramUniform4ui(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLuint v3) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glProgramUniform4ui_underlying(program, location, v0, v1, v2, v3);
    });
}


void gles2_glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform1fv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform2fv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform3fv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform4fv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform1iv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform2iv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform3iv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform4iv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform1uiv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform2uiv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform3uiv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniform4uiv_underlying(program, location, count, value);
    });
}


void gles2_glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix2fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix3fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix4fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix2x3fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix3x2fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix2x4fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix4x2fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix3x4fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glProgramUniformMatrix4x3fv_underlying(program, location, count, transpose, value);
    });
}


void gles2_glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramInterfaceiv_underlying(program, programInterface, pname, params);
    });
}


void gles2_glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum * props, GLsizei bufSize, GLsizei * length, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramResourceiv_underlying(program, programInterface, index, propCount, props, bufSize, length, params);
    });
}


GLuint gles2_glGetProgramResourceIndex(GLuint program, GLenum programInterface, const char * name) { 
    return DriverThread::callOnDriverThread<GLuint>([&]() {
        return gles2_dispatch->glGetProgramResourceIndex_underlying(program, programInterface, name);
    });
}


GLint gles2_glGetProgramResourceLocation(GLuint program, GLenum programInterface, const char * name) { 
    return DriverThread::callOnDriverThread<GLint>([&]() {
        return gles2_dispatch->glGetProgramResourceLocation_underlying(program, programInterface, name);
    });
}


void gles2_glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei * length, char * name) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetProgramResourceName_underlying(program, programInterface, index, bufSize, length, name);
    });
}


void gles2_glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindImageTexture_underlying(unit, texture, level, layered, layer, access, format);
    });
}


void gles2_glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDispatchCompute_underlying(num_groups_x, num_groups_y, num_groups_z);
    });
}


void gles2_glDispatchComputeIndirect(GLintptr indirect) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glDispatchComputeIndirect_underlying(indirect);
    });
}


void gles2_glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLintptr stride) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glBindVertexBuffer_underlying(bindingindex, buffer, offset, stride);
    });
}


void gles2_glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttribBinding_underlying(attribindex, bindingindex);
    });
}


void gles2_glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttribFormat_underlying(attribindex, size, type, normalized, relativeoffset);
    });
}


void gles2_glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexAttribIFormat_underlying(attribindex, size, type, relativeoffset);
    });
}


void gles2_glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glVertexBindingDivisor_underlying(bindingindex, divisor);
    });
}


void gles2_glDrawArraysIndirect(GLenum mode, const void * indirect) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDrawArraysIndirect_underlying(mode, indirect);
    });
}


void gles2_glDrawElementsIndirect(GLenum mode, GLenum type, const void * indirect) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glDrawElementsIndirect_underlying(mode, type, indirect);
    });
}


void gles2_glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glTexStorage2DMultisample_underlying(target, samples, internalformat, width, height, fixedsamplelocations);
    });
}


void gles2_glSampleMaski(GLuint maskNumber, GLbitfield mask) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glSampleMaski_underlying(maskNumber, mask);
    });
}


void gles2_glGetMultisamplefv(GLenum pname, GLuint index, GLfloat * val) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetMultisamplefv_underlying(pname, index, val);
    });
}


void gles2_glFramebufferParameteri(GLenum target, GLenum pname, GLint param) { 
    DriverThread::callOnDriverThreadAsync<void>([=]() {
        gles2_dispatch->glFramebufferParameteri_underlying(target, pname, param);
    });
}


void gles2_glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint * params) { 
    DriverThread::callOnDriverThreadSync([&]() {
        gles2_dispatch->glGetFramebufferParameteriv_underlying(target, pname, params);
    });
}

