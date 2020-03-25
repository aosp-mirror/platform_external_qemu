// Auto-generated with: android/scripts/gen-entries.py --mode=translator_passthrough android/android-emugl/host/libs/libOpenGLESDispatch/gles3_only.entries --output=android/android-emugl/host/libs/Translator/GLES_V2/GLESv30Imp.cpp
// This file is best left unedited.
// Try to make changes through gen_translator in gen-entries.py,
// and/or parcel out custom functionality in separate code.

EXTERN_PART GL_APICALL GLconstubyteptr GL_APIENTRY glGetStringi(GLenum name, GLint index) {
    GET_CTX_V2_RET(0);
    GLconstubyteptr glGetStringiRET = ctx->dispatcher().glGetStringi(name, index);
    return glGetStringiRET;
}

GL_APICALL void GL_APIENTRY glGenVertexArrays(GLsizei n, GLuint* arrays) {
    glGenVertexArraysOES(n, arrays);
}

GL_APICALL void GL_APIENTRY glBindVertexArray(GLuint array) {
    if (array) gles30usages->set_is_used(true);
    glBindVertexArrayOES(array);
}

GL_APICALL void GL_APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint * arrays) {
    glDeleteVertexArraysOES(n, arrays);
}

GL_APICALL GLboolean GL_APIENTRY glIsVertexArray(GLuint array) {
    return glIsVertexArrayOES(array);
}

GL_APICALL void * GL_APIENTRY glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    GET_CTX_V2_RET(0);
    RET_AND_SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM,0);
    gles30usages->set_is_used(true);
    void * glMapBufferRangeRET = ctx->dispatcher().glMapBufferRange(target, offset, length, access);
    return glMapBufferRangeRET;
}

GL_APICALL GLboolean GL_APIENTRY glUnmapBuffer(GLenum target) {
    GET_CTX_V2_RET(0);
    RET_AND_SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM,0);
    GLboolean glUnmapBufferRET = ctx->dispatcher().glUnmapBuffer(target);
    return glUnmapBufferRET;
}

GL_APICALL void GL_APIENTRY glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
    ctx->dispatcher().glFlushMappedBufferRange(target, offset, length);
}

GL_APICALL void GL_APIENTRY glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
    gles30usages->set_is_used(true);
    ctx->bindBuffer(target, buffer);
    ctx->bindIndexedBuffer(target, index, buffer, offset, size);
    if (ctx->shareGroup().get()) {
        const GLuint globalBufferName = ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, buffer);
        ctx->dispatcher().glBindBufferRange(target, index, globalBufferName, offset, size);
    }
}

GL_APICALL void GL_APIENTRY glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
    gles30usages->set_is_used(true);
    ctx->bindBuffer(target, buffer);
    ctx->bindIndexedBuffer(target, index, buffer);
    if (ctx->shareGroup().get()) {
        const GLuint globalBufferName = ctx->shareGroup()->getGlobalName(NamedObjectType::VERTEXBUFFER, buffer);
        ctx->dispatcher().glBindBufferBase(target, index, globalBufferName);
    }
}

GL_APICALL void GL_APIENTRY glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset, GLintptr writeoffset, GLsizeiptr size) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->dispatcher().glCopyBufferSubData(readtarget, writetarget, readoffset, writeoffset, size);
}

GL_APICALL void GL_APIENTRY glClearBufferiv(GLenum buffer, GLint drawBuffer, const GLint * value) {
    GET_CTX_V2();
    ctx->dispatcher().glClearBufferiv(buffer, drawBuffer, value);
}

GL_APICALL void GL_APIENTRY glClearBufferuiv(GLenum buffer, GLint drawBuffer, const GLuint * value) {
    GET_CTX_V2();
    ctx->dispatcher().glClearBufferuiv(buffer, drawBuffer, value);
}

GL_APICALL void GL_APIENTRY glClearBufferfv(GLenum buffer, GLint drawBuffer, const GLfloat * value) {
    GET_CTX_V2();
    ctx->dispatcher().glClearBufferfv(buffer, drawBuffer, value);
}

GL_APICALL void GL_APIENTRY glClearBufferfi(GLenum buffer, GLint drawBuffer, GLfloat depth, GLint stencil) {
    GET_CTX_V2();
    ctx->dispatcher().glClearBufferfi(buffer, drawBuffer, depth, stencil);
}

GL_APICALL void GL_APIENTRY glGetBufferParameteri64v(GLenum target, GLenum value, GLint64 * data) {
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
    ctx->dispatcher().glGetBufferParameteri64v(target, value, data);
}

GL_APICALL void GL_APIENTRY glGetBufferPointerv(GLenum target, GLenum pname, GLvoid ** params) {
    GET_CTX_V2();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
    ctx->dispatcher().glGetBufferPointerv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glUniformBlockBinding(globalProgramName, uniformBlockIndex, uniformBlockBinding);
    }
}

GL_APICALL GLuint GL_APIENTRY glGetUniformBlockIndex(GLuint program, const GLchar * uniformBlockName) {
    GET_CTX_V2_RET(0);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        GLuint glGetUniformBlockIndexRET = ctx->dispatcher().glGetUniformBlockIndex(globalProgramName, uniformBlockName);
    return glGetUniformBlockIndexRET;
    } else return 0;
}

EXTERN_PART GL_APICALL void GL_APIENTRY glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar ** uniformNames, GLuint * uniformIndices) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetUniformIndices(globalProgramName, uniformCount, uniformNames, uniformIndices);
    }
}

GL_APICALL void GL_APIENTRY glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetActiveUniformBlockiv(globalProgramName, uniformBlockIndex, pname, params);
    }
}

GL_APICALL void GL_APIENTRY glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetActiveUniformBlockName(globalProgramName, uniformBlockIndex, bufSize, length, uniformBlockName);
    }
}

GL_APICALL void GL_APIENTRY glUniform1ui(GLint location, GLuint v0) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform1ui(hostLoc, v0);
}

GL_APICALL void GL_APIENTRY glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform2ui(hostLoc, v0, v1);
}

GL_APICALL void GL_APIENTRY glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform3ui(hostLoc, v0, v1, v2);
}

EXTERN_PART GL_APICALL void GL_APIENTRY glUniform4ui(GLint location, GLint v0, GLuint v1, GLuint v2, GLuint v3) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform4ui(hostLoc, v0, v1, v2, v3);
}

GL_APICALL void GL_APIENTRY glUniform1uiv(GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform1uiv(hostLoc, count, value);
}

GL_APICALL void GL_APIENTRY glUniform2uiv(GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform2uiv(hostLoc, count, value);
}

GL_APICALL void GL_APIENTRY glUniform3uiv(GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform3uiv(hostLoc, count, value);
}

GL_APICALL void GL_APIENTRY glUniform4uiv(GLint location, GLsizei count, const GLuint * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniform4uiv(hostLoc, count, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniformMatrix2x3fv(hostLoc, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniformMatrix3x2fv(hostLoc, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniformMatrix2x4fv(hostLoc, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniformMatrix4x2fv(hostLoc, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniformMatrix3x4fv(hostLoc, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value) {
    GET_CTX_V2();
    int hostLoc = s_getHostLocOrSetError(ctx, location);
    SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
    ctx->dispatcher().glUniformMatrix4x3fv(hostLoc, count, transpose, value);
}

GL_APICALL void GL_APIENTRY glGetUniformuiv(GLuint program, GLint location, GLuint * params) {
    GET_CTX_V2();

    SET_ERROR_IF(location < 0,GL_INVALID_OPERATION);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);

        SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::SHADER_OR_PROGRAM, program);
        SET_ERROR_IF(objData->getDataType()!=PROGRAM_DATA,GL_INVALID_OPERATION);
        ProgramData* pData = (ProgramData *)objData;
        (void)pData;
#if !defined(TOLERATE_PROGRAM_LINK_ERROR) || !TOLERATE_PROGRAM_LINK_ERROR
        SET_ERROR_IF(!pData->getLinkStatus(), GL_INVALID_OPERATION);
#endif
        int hostLoc = s_getHostLocOrSetError(ctx, location);
        SET_ERROR_IF(hostLoc < -1, GL_INVALID_OPERATION);
        ctx->dispatcher().glGetUniformuiv(globalProgramName, hostLoc, params);
    }
}

GL_APICALL void GL_APIENTRY glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetActiveUniformsiv(globalProgramName, uniformCount, uniformIndices, pname, params);
    }
}

GL_APICALL void GL_APIENTRY glVertexAttribI4i(GLuint index, GLint v0, GLint v1, GLint v2, GLint v3) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->dispatcher().glVertexAttribI4i(index, v0, v1, v2, v3);
}

GL_APICALL void GL_APIENTRY glVertexAttribI4ui(GLuint index, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->dispatcher().glVertexAttribI4ui(index, v0, v1, v2, v3);
}

GL_APICALL void GL_APIENTRY glVertexAttribI4iv(GLuint index, const GLint * v) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->dispatcher().glVertexAttribI4iv(index, v);
}

GL_APICALL void GL_APIENTRY glVertexAttribI4uiv(GLuint index, const GLuint * v) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->dispatcher().glVertexAttribI4uiv(index, v);
}

GL_APICALL void  GL_APIENTRY glVertexAttribIPointerWithDataSize(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* ptr, GLsizei dataSize) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);

    s_glPrepareVertexAttribPointer(ctx, index, size, type, false, stride, ptr, dataSize, true);
    if (ctx->isBindedBuffer(GL_ARRAY_BUFFER)) {
        ctx->dispatcher().glVertexAttribIPointer(index, size, type, stride, ptr);
    }
}

GL_APICALL void GL_APIENTRY glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    s_glPrepareVertexAttribPointer(ctx, index, size, type, false, stride, pointer, 0, true);
    if (ctx->isBindedBuffer(GL_ARRAY_BUFFER)) {
        ctx->dispatcher().glVertexAttribIPointer(index, size, type, stride, pointer);
    }
}

GL_APICALL void GL_APIENTRY glGetVertexAttribIiv(GLuint index, GLenum pname, GLint * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetVertexAttribIiv(index, pname, params);
}

GL_APICALL void GL_APIENTRY glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetVertexAttribIuiv(index, pname, params);
}

GL_APICALL void GL_APIENTRY glVertexAttribDivisor(GLuint index, GLuint divisor) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->setVertexAttribBindingIndex(index, index);
    ctx->setVertexAttribDivisor(index, divisor);
    ctx->dispatcher().glVertexAttribDivisor(index, divisor);
}

GL_APICALL void GL_APIENTRY glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode)),GL_INVALID_ENUM);
    if (ctx->vertexAttributesBufferBacked()) {
        s_glDrawPre(ctx, mode);
        ctx->dispatcher().glDrawArraysInstanced(mode, first, count, primcount);
        s_glDrawPost(ctx, mode);
    } else {
        ctx->drawWithEmulations(
                GLESv2Context::DrawCallCmd::ArraysInstanced,
                mode, first, count,
                0, nullptr, primcount, 0, 0 /* type, indices, start, end unused */);
    }
}

GL_APICALL void GL_APIENTRY glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void * indices, GLsizei primcount) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode) && GLESv2Validate::drawType(type)),GL_INVALID_ENUM);

    if (ctx->isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER) &&
        ctx->vertexAttributesBufferBacked()) {
        s_glDrawPre(ctx, mode, type);
        ctx->dispatcher().glDrawElementsInstanced(mode, count, type, indices, primcount);
        s_glDrawPost(ctx, mode);
    } else {
        ctx->drawWithEmulations(
                GLESv2Context::DrawCallCmd::ElementsInstanced,
                mode, 0 /* first (unused) */, count, type, indices, primcount,
                0, 0 /* start, end (unused) */);
    }
}

GL_APICALL void GL_APIENTRY glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode) && GLESv2Validate::drawType(type)),GL_INVALID_ENUM);

    if (ctx->isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER) &&
        ctx->vertexAttributesBufferBacked()) {
        s_glDrawPre(ctx, mode, type);
        ctx->dispatcher().glDrawRangeElements(mode, start, end, count, type, indices);
        s_glDrawPost(ctx, mode);
    } else {
        ctx->drawWithEmulations(
                GLESv2Context::DrawCallCmd::RangeElements,
                mode, 0 /* first (unused) */, count, type, indices, 0 /* primcount (unused) */,
                start, end);
    }
}

// GuestSyncs: Tracks the system-wide set of GL syncs, puts them in a
// distinguished namespace from EGL syncs. This is in its own class as opposed
// to a NameSpace because a) no real need to snapshot save/load those and b) it
// needs to span all contexts and give the same names across the entire system,
// as sync objects live outside of GL context state.
// This is to fix dEQP negative_api tests that delete sync object 0x1 but fail due
// to it colliding with an EGL fence name.
class GuestSyncs {
public:
    GuestSyncs() { }

    GLsync create(GLsync newHostSync) {
        GLsync res = (GLsync)(uintptr_t)mNameCounter;
        mSyncs[res] = newHostSync;
        mNameCounter++;
        if (!mNameCounter) mNameCounter = 0x1000;
        return res;
    }

    GLsync lookupWithError(GLsync guestSync, GLint* err) {
        *err = GL_NO_ERROR;
        GLsync host = (GLsync)0x0;

        const auto& it = mSyncs.find(guestSync);
        if (it == mSyncs.end()) {
            *err = GL_INVALID_VALUE;
            return host;
        } else {
            host = it->second;
        }
        return host;
    }

    GLsync removeWithError(GLsync guestSyncToDelete, GLint* err) {
        *err = GL_NO_ERROR;
        GLsync host = (GLsync)0x0;

        if (!guestSyncToDelete) {
            return host;
        }

        const auto& it = mSyncs.find(guestSyncToDelete);
        if (it == mSyncs.end()) {
            *err = GL_INVALID_VALUE;
            return host;
        } else {
            host = it->second;
        }

        mSyncs.erase(it);
        return host;
    }

    bool isSync(GLsync guestSync) {
        return mSyncs.find(guestSync) != mSyncs.end();
    }

    emugl::Mutex& lock() { return mLock; }

private:
    std::unordered_map<GLsync, GLsync> mSyncs;
    mutable emugl::Mutex mLock;
    uint32_t mNameCounter = 0x1;
};

static android::base::LazyInstance<GuestSyncs> sSyncs = LAZY_INSTANCE_INIT;

static GLsync internal_glFenceSync(GLenum condition, GLbitfield flags) {
    GET_CTX_V2_RET(0);
    if (!ctx->dispatcher().glFenceSync) {
        ctx->dispatcher().glFinish();
        return (GLsync)0x42;
    }
    GLsync glFenceSyncRET = ctx->dispatcher().glFenceSync(condition, flags);
    return glFenceSyncRET;
}

static GLenum internal_glClientWaitSync(GLsync wait_on, GLbitfield flags, GLuint64 timeout) {
    GET_CTX_V2_RET(GL_WAIT_FAILED);
    if (!ctx->dispatcher().glFenceSync) {
        return GL_ALREADY_SIGNALED;
    }
    GLenum glClientWaitSyncRET = ctx->dispatcher().glClientWaitSync(wait_on, flags, timeout);
    return glClientWaitSyncRET;
}

static void internal_glWaitSync(GLsync wait_on, GLbitfield flags, GLuint64 timeout) {
    GET_CTX_V2();
    if (!ctx->dispatcher().glFenceSync) {
        return;
    }
    ctx->dispatcher().glWaitSync(wait_on, flags, timeout);
}

static void internal_glDeleteSync(GLsync to_delete) {
    GET_CTX_V2();
    if (!ctx->dispatcher().glFenceSync) {
        return;
    }
    ctx->dispatcher().glDeleteSync(to_delete);
}

static void internal_glGetSynciv(GLsync sync, GLenum pname, GLsizei bufsize, GLsizei *length, GLint *values) {
    GET_CTX_V2();
    if (!ctx->dispatcher().glGetSynciv) {
        if (bufsize < sizeof(GLint)) return;
        switch (pname) {
            case GL_OBJECT_TYPE:
                if (length) *length = sizeof(GLint);
                *values = GL_SYNC_FENCE;
                break;
            case GL_SYNC_CONDITION:
                if (length) *length = sizeof(GLint);
                *values = GL_SYNC_GPU_COMMANDS_COMPLETE;
                break;
            case GL_SYNC_FLAGS:
                // Not supported
                break;
            case GL_SYNC_STATUS:
                if (length) *length = sizeof(GLint);
                *values = GL_SIGNALED;
                break;
            default:
                break;
        }
        return;
    }
    ctx->dispatcher().glGetSynciv(sync, pname, bufsize, length, values);
}

GL_APICALL GLsync GL_APIENTRY glFenceSync(GLenum condition, GLbitfield flags) {
    GET_CTX_V2_RET(0);
    gles30usages->set_is_used(true);
    gles30usages->set_fence_sync(true);

    emugl::Mutex::AutoLock lock(sSyncs->lock());
    GLsync hostSync = internal_glFenceSync(condition, flags);
    GLsync guestSync = sSyncs->create(hostSync);
    return guestSync;
}

GL_APICALL GLenum GL_APIENTRY glClientWaitSync(GLsync wait_on, GLbitfield flags, GLuint64 timeout) {
    GET_CTX_V2_RET(GL_WAIT_FAILED);
    gles30usages->set_is_used(true);
    GLint err = GL_NO_ERROR;

    emugl::Mutex::AutoLock lock(sSyncs->lock());
    GLsync hostSync = sSyncs->lookupWithError(wait_on, &err);
    RET_AND_SET_ERROR_IF(err != GL_NO_ERROR, err, GL_WAIT_FAILED);
    return internal_glClientWaitSync(hostSync, flags, timeout);
}

GL_APICALL void GL_APIENTRY glWaitSync(GLsync wait_on, GLbitfield flags, GLuint64 timeout) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    GLint err = GL_NO_ERROR;

    emugl::Mutex::AutoLock lock(sSyncs->lock());
    GLsync hostSync = sSyncs->lookupWithError(wait_on, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    internal_glWaitSync(hostSync, flags, timeout);
}

GL_APICALL void GL_APIENTRY glDeleteSync(GLsync to_delete) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    GLint err = GL_NO_ERROR;

    emugl::Mutex::AutoLock lock(sSyncs->lock());
    GLsync hostSync = sSyncs->removeWithError(to_delete, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    internal_glDeleteSync(hostSync);
}

GL_APICALL GLboolean GL_APIENTRY glIsSync(GLsync sync) {
    GET_CTX_V2_RET(0);

    emugl::Mutex::AutoLock lock(sSyncs->lock());
    gles30usages->set_is_used(true);
    return sSyncs->isSync(sync) ? GL_TRUE : GL_FALSE;
}

GL_APICALL void GL_APIENTRY glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei * length, GLint * values) {
    GET_CTX_V2();
    GLint err = GL_NO_ERROR;

    emugl::Mutex::AutoLock lock(sSyncs->lock());
    GLsync hostSync = sSyncs->lookupWithError(sync, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    ctx->dispatcher().glGetSynciv(hostSync, pname, bufSize, length, values);
}

GL_APICALL void GL_APIENTRY glDrawBuffers(GLsizei n, const GLenum * bufs) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    if (ctx->isDefaultFBOBound(GL_DRAW_FRAMEBUFFER)) {
        SET_ERROR_IF(n != 1 || (bufs[0] != GL_NONE && bufs[0] != GL_BACK),
                GL_INVALID_OPERATION);
        GLenum emulatedBufs = bufs[0] == GL_NONE ? GL_NONE
                                                 : GL_COLOR_ATTACHMENT0;
        ctx->setDefaultFBODrawBuffer(emulatedBufs);
        ctx->dispatcher().glDrawBuffers(1, &emulatedBufs);
    } else {
        GLuint framebuffer = ctx->getFramebufferBinding(GL_DRAW_FRAMEBUFFER);
        FramebufferData* fbObj = ctx->getFBOData(framebuffer);
        fbObj->setDrawBuffers(n, bufs);
        ctx->dispatcher().glDrawBuffers(n, bufs);
    }
}

GL_APICALL void GL_APIENTRY glReadBuffer(GLenum src) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    // if default fbo is bound and src is GL_BACK,
    // use GL_COLOR_ATTACHMENT0 all of a sudden.
    // bc we are using fbo emulation.
    if (ctx->isDefaultFBOBound(GL_READ_FRAMEBUFFER)) {
        SET_ERROR_IF(src != GL_NONE && src != GL_BACK, GL_INVALID_OPERATION);
        GLenum emulatedSrc = src == GL_NONE ? GL_NONE
                                            : GL_COLOR_ATTACHMENT0;
        ctx->setDefaultFBOReadBuffer(emulatedSrc);
        ctx->dispatcher().glReadBuffer(emulatedSrc);
    } else {
        GLuint framebuffer = ctx->getFramebufferBinding(GL_READ_FRAMEBUFFER);
        FramebufferData* fbObj = ctx->getFBOData(framebuffer);
        fbObj->setReadBuffers(src);
        ctx->dispatcher().glReadBuffer(src);
    }
}

GL_APICALL void GL_APIENTRY glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->dispatcher().glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

static std::vector<GLenum> sGetEmulatedAttachmentList(GLESv2Context* ctx, GLenum target,
                                                      GLsizei numAttachments, const GLenum* attachments) {
    std::vector<GLenum> res(numAttachments);
    memcpy(&res[0], attachments, numAttachments * sizeof(GLenum));

    if (!ctx->hasEmulatedDefaultFBO() ||
        !ctx->isDefaultFBOBound(target)) return res;


    for (int i = 0; i < numAttachments; i++) {
        if (attachments[i] == GL_COLOR) res[i] = GL_COLOR_ATTACHMENT0;
        if (attachments[i] == GL_DEPTH) res[i] = GL_DEPTH_ATTACHMENT;
        if (attachments[i] == GL_STENCIL) res[i] = GL_STENCIL_ATTACHMENT;
    }

    return res;
}

GL_APICALL void GL_APIENTRY glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF(target != GL_FRAMEBUFFER &&
                 target != GL_READ_FRAMEBUFFER &&
                 target != GL_DRAW_FRAMEBUFFER, GL_INVALID_ENUM);

    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);

    for (int i = 0; i < numAttachments; i++) {
        if (attachments[i] >= GL_COLOR_ATTACHMENT0 &&
            attachments[i] <= GL_COLOR_ATTACHMENT15) {
                SET_ERROR_IF((GLint)(attachments[i] - GL_COLOR_ATTACHMENT0 + 1) >
                             maxColorAttachments, GL_INVALID_OPERATION);
        }
    }

    std::vector<GLenum> emulatedAttachments = sGetEmulatedAttachmentList(ctx, target, numAttachments, attachments);
    if (ctx->dispatcher().glInvalidateFramebuffer) {
        ctx->dispatcher().glInvalidateFramebuffer(target, numAttachments, &emulatedAttachments[0]);
    } else {
        // If we are missing glInvalidateFramebuffer, just don't do anything and hope things work out.
    }
}

GL_APICALL void GL_APIENTRY glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum * attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF(target != GL_FRAMEBUFFER &&
            target != GL_READ_FRAMEBUFFER &&
            target != GL_DRAW_FRAMEBUFFER, GL_INVALID_ENUM);

    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);

    for (int i = 0; i < numAttachments; i++) {
        if (attachments[i] >= GL_COLOR_ATTACHMENT0 &&
                attachments[i] <= GL_COLOR_ATTACHMENT15) {
            SET_ERROR_IF((GLint)(attachments[i] - GL_COLOR_ATTACHMENT0 + 1) >
                         maxColorAttachments, GL_INVALID_OPERATION);
        }
    }

    std::vector<GLenum> emulatedAttachments = sGetEmulatedAttachmentList(ctx, target, numAttachments, attachments);
    if (ctx->dispatcher().glInvalidateSubFramebuffer) {
        ctx->dispatcher().glInvalidateSubFramebuffer(target, numAttachments, &emulatedAttachments[0], x, y, width, height);
    } else {
        // If we are missing glInvalidateSubFramebuffer, just don't do anything and hope things work out.
    }
}

GL_APICALL void GL_APIENTRY glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    gles30usages->set_framebuffer_texture_layer(true);
    GLenum textarget = GL_TEXTURE_2D_ARRAY;
    SET_ERROR_IF(!(GLESv2Validate::framebufferTarget(ctx, target) &&
                   GLESv2Validate::framebufferAttachment(ctx, attachment)), GL_INVALID_ENUM);
    SET_ERROR_IF(ctx->isDefaultFBOBound(target), GL_INVALID_OPERATION);
    if (texture) {
        if (!ctx->shareGroup()->isObject(NamedObjectType::TEXTURE, texture)) {
            ctx->shareGroup()->genName(NamedObjectType::TEXTURE, texture);
        }
        TextureData* texData = getTextureData(texture);
        textarget = texData->target;
    }
    if (ctx->shareGroup().get()) {
        const GLuint globalTextureName = ctx->shareGroup()->getGlobalName(NamedObjectType::TEXTURE, texture);
        ctx->dispatcher().glFramebufferTextureLayer(target, attachment, globalTextureName, level, layer);
    }

    GLuint fbName = ctx->getFramebufferBinding(target);
    auto fbObj = ctx->getFBOData(fbName);
    if (fbObj) {
        fbObj->setAttachment(
            ctx, attachment, textarget, texture, ObjectDataPtr());
    }

}

GL_APICALL void GL_APIENTRY glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    gles30usages->set_renderbuffer_storage_multisample(true);
    GLint err = GL_NO_ERROR;
    internalformat = sPrepareRenderbufferStorage(internalformat, width, height, samples, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    ctx->dispatcher().glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

GL_APICALL void GL_APIENTRY glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetInternalformativ(target, internalformat, pname, bufSize, params);
}

GL_APICALL void GL_APIENTRY glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    GLint err = GL_NO_ERROR;
    GLenum format, type;
    GLESv2Validate::getCompatibleFormatTypeForInternalFormat(internalformat, &format, &type);
    sPrepareTexImage2D(target, 0, (GLint)internalformat, width, height, 0, format, type, 0, NULL, &type, (GLint*)&internalformat, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
    TextureData *texData = getTextureTargetData(target);
    texData->texStorageLevels = levels;
    ctx->dispatcher().glTexStorage2D(target, levels, internalformat, width, height);
}

GL_APICALL void GL_APIENTRY glBeginTransformFeedback(GLenum primitiveMode) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->boundTransformFeedback()->mIsActive = true;
    ctx->dispatcher().glBeginTransformFeedback(primitiveMode);
}

GL_APICALL void GL_APIENTRY glEndTransformFeedback() {
    GET_CTX_V2();
    ctx->boundTransformFeedback()->mIsActive = false;
    ctx->dispatcher().glEndTransformFeedback();
}

GL_APICALL void GL_APIENTRY glGenTransformFeedbacks(GLsizei n, GLuint * ids) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);
    if (n) {
        gles30usages->set_is_used(true);
        gles30usages->set_gen_transform_feedbacks(true);
    }
    for (int i = 0; i < n; i++) {
        ids[i] = ctx->genTransformFeedbackName(0, true);
    }
}

GL_APICALL void GL_APIENTRY glDeleteTransformFeedbacks(GLsizei n, const GLuint * ids) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);
    ObjectLocalName boundTransformFeedback = ctx->getTransformFeedbackBinding();
    TransformFeedbackData* tfData = ctx->boundTransformFeedback();
    if (boundTransformFeedback) {
        for (GLsizei i = 0; i < n; i++) {
            SET_ERROR_IF(ids[i] == boundTransformFeedback && tfData->mIsActive,
                         GL_INVALID_OPERATION);
        }
    }
    for (GLsizei i = 0; i < n; i++) {
        if (ids[i]) {
            if (boundTransformFeedback == ids[i]) {
                assert(!tfData->mIsActive);
                ctx->bindTransformFeedback(0);
            }
            ctx->deleteTransformFeedback(ids[i]);
        }
    }
}

GL_APICALL void GL_APIENTRY glBindTransformFeedback(GLenum target, GLuint id) {
    GET_CTX_V2();
    if (id) {
        gles30usages->set_is_used(true);
        gles30usages->set_gen_transform_feedbacks(true);
    }
    unsigned int globalName = ctx->getTransformFeedbackGlobalName(id);
    SET_ERROR_IF(id != 0 && globalName == 0, GL_INVALID_OPERATION);
    ctx->bindTransformFeedback(id);
    ctx->dispatcher().glBindTransformFeedback(target, globalName);
}

GL_APICALL void GL_APIENTRY glPauseTransformFeedback() {
    GET_CTX_V2();
    ctx->dispatcher().glPauseTransformFeedback();
}

GL_APICALL void GL_APIENTRY glResumeTransformFeedback() {
    GET_CTX_V2();
    ctx->dispatcher().glResumeTransformFeedback();
}

GL_APICALL GLboolean GL_APIENTRY glIsTransformFeedback(GLuint id) {
    GET_CTX_V2_RET(0);
    return ctx->hasBoundTransformFeedback(id);
}

EXTERN_PART GL_APICALL void GL_APIENTRY glTransformFeedbackVaryings(GLuint program, GLsizei count, const char ** varyings, GLenum bufferMode) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glTransformFeedbackVaryings(globalProgramName, count, varyings, bufferMode);
    }
}

GL_APICALL void GL_APIENTRY glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, char * name) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetTransformFeedbackVarying(globalProgramName, index, bufSize, length, size, type, name);
    }
}

GL_APICALL void GL_APIENTRY glGenSamplers(GLsizei n, GLuint * samplers) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);

    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            samplers[i] = ctx->shareGroup()->genName(NamedObjectType::SAMPLER,
                                                     0, true);
            ctx->shareGroup()->setObjectData(NamedObjectType::SAMPLER,
                    samplers[i], ObjectDataPtr(new SamplerData()));
        }
    }
}

GL_APICALL void GL_APIENTRY glDeleteSamplers(GLsizei n, const GLuint * samplers) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);

    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            ctx->shareGroup()->deleteName(NamedObjectType::SAMPLER, samplers[i]);
        }
    }
}

GL_APICALL void GL_APIENTRY glBindSampler(GLuint unit, GLuint sampler) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
        SET_ERROR_IF(sampler && !globalSampler, GL_INVALID_OPERATION);
        ctx->setBindSampler(unit, sampler);
        ctx->dispatcher().glBindSampler(unit, globalSampler);
    }
}

GL_APICALL void GL_APIENTRY glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(
                NamedObjectType::SAMPLER, sampler);
        SET_ERROR_IF(!globalSampler, GL_INVALID_OPERATION);
        SamplerData* samplerData = (SamplerData*)ctx->shareGroup()->getObjectData(
                NamedObjectType::SAMPLER, sampler);
        samplerData->setParamf(pname, param);
        ctx->dispatcher().glSamplerParameterf(globalSampler, pname, param);
    }
}

GL_APICALL void GL_APIENTRY glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(
        NamedObjectType::SAMPLER, sampler);
        SET_ERROR_IF(!globalSampler, GL_INVALID_OPERATION);
        SamplerData* samplerData = (SamplerData*)ctx->shareGroup()->getObjectData(
                NamedObjectType::SAMPLER, sampler);
        samplerData->setParami(pname, param);
        ctx->dispatcher().glSamplerParameteri(globalSampler, pname, param);
    }
}

GL_APICALL void GL_APIENTRY glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
        ctx->dispatcher().glSamplerParameterfv(globalSampler, pname, params);
    }
}

GL_APICALL void GL_APIENTRY glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
        ctx->dispatcher().glSamplerParameteriv(globalSampler, pname, params);
    }
}

GL_APICALL void GL_APIENTRY glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
        ctx->dispatcher().glGetSamplerParameterfv(globalSampler, pname, params);
    }
}

GL_APICALL void GL_APIENTRY glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
        ctx->dispatcher().glGetSamplerParameteriv(globalSampler, pname, params);
    }
}

GL_APICALL GLboolean GL_APIENTRY glIsSampler(GLuint sampler) {
    GET_CTX_V2_RET(0);
    if (ctx->shareGroup().get()) {
        const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
        GLboolean glIsSamplerRET = ctx->dispatcher().glIsSampler(globalSampler);
    return glIsSamplerRET;
    } else return 0;
}

GL_APICALL void GL_APIENTRY glGenQueries(GLsizei n, GLuint * queries) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);

    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            queries[i] = ctx->shareGroup()->genName(NamedObjectType::QUERY,
                                                     0, true);
        }
    }
}

GL_APICALL void GL_APIENTRY glDeleteQueries(GLsizei n, const GLuint * queries) {
    GET_CTX_V2();
    SET_ERROR_IF(n < 0,GL_INVALID_VALUE);

    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            ctx->shareGroup()->deleteName(NamedObjectType::QUERY, queries[i]);
        }
    }
}

GL_APICALL void GL_APIENTRY glBeginQuery(GLenum target, GLuint query) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    gles30usages->set_begin_query(true);
    if (ctx->shareGroup().get()) {
        const GLuint globalQuery = ctx->shareGroup()->getGlobalName(NamedObjectType::QUERY, query);
        ctx->dispatcher().glBeginQuery(target, globalQuery);
    }
}

GL_APICALL void GL_APIENTRY glEndQuery(GLenum target) {
    GET_CTX_V2();
    ctx->dispatcher().glEndQuery(target);
}

GL_APICALL void GL_APIENTRY glGetQueryiv(GLenum target, GLenum pname, GLint * params) {
    GET_CTX_V2();
    ctx->dispatcher().glGetQueryiv(target, pname, params);
}

GL_APICALL void GL_APIENTRY glGetQueryObjectuiv(GLuint query, GLenum pname, GLuint * params) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalQuery = ctx->shareGroup()->getGlobalName(NamedObjectType::QUERY, query);
        ctx->dispatcher().glGetQueryObjectuiv(globalQuery, pname, params);
    }
}

GL_APICALL GLboolean GL_APIENTRY glIsQuery(GLuint query) {
    GET_CTX_V2_RET(0);
    if (ctx->shareGroup().get()) {
        const GLuint globalQuery = ctx->shareGroup()->getGlobalName(NamedObjectType::QUERY, query);
        GLboolean glIsQueryRET = ctx->dispatcher().glIsQuery(globalQuery);
    return glIsQueryRET;
    } else return 0;
}

GL_APICALL void GL_APIENTRY glProgramParameteri(GLuint program, GLenum pname, GLint value) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramParameteri(globalProgramName, pname, value);
    }
}

GL_APICALL void GL_APIENTRY glProgramBinary(GLuint program, GLenum binaryFormat, const void * binary, GLsizei length) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glProgramBinary(globalProgramName, binaryFormat, binary, length);
    }
}

GL_APICALL void GL_APIENTRY glGetProgramBinary(GLuint program, GLsizei bufsize, GLsizei * length, GLenum * binaryFormat, void * binary) {
    GET_CTX_V2();
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        ctx->dispatcher().glGetProgramBinary(globalProgramName, bufsize, length, binaryFormat, binary);
    }
}

GL_APICALL GLint GL_APIENTRY glGetFragDataLocation(GLuint program, const char * name) {
    GET_CTX_V2_RET(0);
    if (ctx->shareGroup().get()) {
        const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
        GLint glGetFragDataLocationRET = ctx->dispatcher().glGetFragDataLocation(globalProgramName, name);
    return glGetFragDataLocationRET;
    } else return 0;
}

GL_APICALL void GL_APIENTRY glGetInteger64v(GLenum pname, GLint64 * data) {
    GET_CTX_V2();
    s_glStateQueryTv<GLint64>(true, pname, data, s_glGetInteger64v_wrapper);
}

GL_APICALL void GL_APIENTRY glGetIntegeri_v(GLenum target, GLuint index, GLint * data) {
    GET_CTX_V2();
    s_glStateQueryTi_v<GLint>(target, index, data, s_glGetIntegeri_v_wrapper);
}

GL_APICALL void GL_APIENTRY glGetInteger64i_v(GLenum target, GLuint index, GLint64 * data) {
    GET_CTX_V2();
    s_glStateQueryTi_v<GLint64>(target, index, data, s_glGetInteger64i_v_wrapper);
}

GL_APICALL void GL_APIENTRY glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * data) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    SET_ERROR_IF(!GLESv2Validate::pixelItnlFrmt(ctx,internalFormat), GL_INVALID_VALUE);
    SET_ERROR_IF(!GLESv2Validate::isCompressedFormat(internalFormat) &&
                 !GLESv2Validate::pixelSizedFrmt(ctx, internalFormat, format, type),
                 GL_INVALID_OPERATION);

    s_glInitTexImage3D(target, level, internalFormat, width, height, depth,
            border, format, type);
    // Desktop OpenGL doesn't support GL_BGRA_EXT as internal format.
    if (!isGles2Gles() && type == GL_UNSIGNED_BYTE && format == GL_BGRA_EXT &&
        internalFormat == GL_BGRA_EXT) {
        internalFormat = GL_RGBA;
    }

    if (isCoreProfile()) {
        GLEScontext::prepareCoreProfileEmulatedTexture(
            getTextureTargetData(target),
            true, target, format, type,
            &internalFormat, &format);
    }

    ctx->dispatcher().glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, data);
}

GL_APICALL void GL_APIENTRY glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    GLenum format, type;
    GLESv2Validate::getCompatibleFormatTypeForInternalFormat(internalformat, &format, &type);
    s_glInitTexImage3D(target, 0, internalformat, width, height, depth, 0,
            format, type);
    // Desktop OpenGL doesn't support GL_BGRA_EXT as internal format.
    if (!isGles2Gles() && type == GL_UNSIGNED_BYTE && format == GL_BGRA_EXT &&
        internalformat == GL_BGRA8_EXT) {
        internalformat = GL_RGBA8;
    }
    TextureData *texData = getTextureTargetData(target);
    texData->texStorageLevels = levels;
    ctx->dispatcher().glTexStorage3D(target, levels, internalformat, width, height, depth);
}

GL_APICALL void GL_APIENTRY glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * data) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    if (isCoreProfile() &&
        isCoreProfileEmulatedFormat(format)) {
        format = getCoreProfileEmulatedFormat(format);
    }
    TextureData* texData = getTextureTargetData(target);
    if (texData) {
        texData->setMipmapLevelAtLeast(level);
        texData->makeDirty();
    }
    ctx->dispatcher().glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}

GL_APICALL void GL_APIENTRY glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    ctx->dispatcher().glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
    if (ctx->shareGroup().get()) {
        TextureData *texData = getTextureTargetData(target);

        if (texData) {
            texData->hasStorage = true;
            texData->compressed = true;
            texData->compressedFormat = internalformat;
            texData->makeDirty();
        }
    }
}

GL_APICALL void GL_APIENTRY glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    TextureData* texData = getTextureTargetData(target);
    if (texData) {
        texData->makeDirty();
    }
    ctx->dispatcher().glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

GL_APICALL void GL_APIENTRY glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GET_CTX_V2();
    gles30usages->set_is_used(true);
    TextureData* texData = getTextureTargetData(target);
    if (texData) {
        texData->makeDirty();
    }
    ctx->dispatcher().glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}

