// ANGLE_robust_client_memory

GL_APICALL void GL_APIENTRY glGetBooleanvRobustANGLE(GLenum pname, GLsizei bufSize, GLsizei * length, GLboolean * data) {
    GET_CTX_V2();
    SET_ERROR_IF(paramValuesCount(pname) > bufSize, GL_INVALID_OPERATION);
    (void)length;
    ctx->dispatcher().glGetBooleanv(pname, data);
}

// GL_APICALL void GL_APIENTRY glGetBufferParameterivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
//     ctx->dispatcher().glGetBufferParameterivRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetFloatvRobustANGLE(GLenum pname, GLsizei bufSize, GLsizei * length, float * data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetFloatvRobustANGLE(pname, bufSize, length, data);
// }
// 
// GL_APICALL void GL_APIENTRY glGetFramebufferAttachmentParameterivRobustANGLE(GLenum target, GLenum attachment, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetFramebufferAttachmentParameterivRobustANGLE(target, attachment, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetIntegervRobustANGLE(GLenum pname, GLsizei bufSize, GLsizei * length, int * data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetIntegervRobustANGLE(pname, bufSize, length, data);
// }
// 
// GL_APICALL void GL_APIENTRY glGetProgramivRobustANGLE(GLuint program, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetProgramivRobustANGLE(globalProgramName, pname, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetRenderbufferParameterivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetRenderbufferParameterivRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetShaderivRobustANGLE(GLuint shader, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetShaderivRobustANGLE(shader, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetTexParameterfvRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, float * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetTexParameterfvRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetTexParameterivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetTexParameterivRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetUniformfvRobustANGLE(GLuint program, int location, GLsizei bufSize, GLsizei * length, float * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetUniformfvRobustANGLE(globalProgramName, location, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetUniformivRobustANGLE(GLuint program, int location, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetUniformivRobustANGLE(globalProgramName, location, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetVertexAttribfvRobustANGLE(GLuint index, GLenum pname, GLsizei bufSize, GLsizei * length, float * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetVertexAttribfvRobustANGLE(index, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetVertexAttribivRobustANGLE(GLuint index, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetVertexAttribivRobustANGLE(index, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetVertexAttribPointervRobustANGLE(GLuint index, GLenum pname, GLsizei bufSize, GLsizei * length, void ** pointer) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetVertexAttribPointervRobustANGLE(index, pname, bufSize, length, pointer);
// }
// 
// GL_APICALL void GL_APIENTRY glReadPixelsRobustANGLE(int x, int y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, GLsizei * length, GLsizei * columns, GLsizei * rows, void * pixels) {
//     GET_CTX_V2();
//     ctx->dispatcher().glReadPixelsRobustANGLE(x, y, width, height, format, type, bufSize, length, columns, rows, pixels);
// }
// 
// GL_APICALL void GL_APIENTRY glTexImage2DRobustANGLE(GLenum target, int level, int internalformat, GLsizei width, GLsizei height, int border, GLenum format, GLenum type, GLsizei bufSize, const void * pixels) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexImage2DRobustANGLE(target, level, internalformat, width, height, border, format, type, bufSize, pixels);
// }
// 
// GL_APICALL void GL_APIENTRY glTexParameterfvRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, const GLfloat * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexParameterfvRobustANGLE(target, pname, bufSize, params);
// }
// 
// GL_APICALL void GL_APIENTRY glTexParameterivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, const GLint * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexParameterivRobustANGLE(target, pname, bufSize, params);
// }
// 
// GL_APICALL void GL_APIENTRY glTexSubImage2DRobustANGLE(GLenum target, int level, int xoffset, int yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, const void * pixels) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexSubImage2DRobustANGLE(target, level, xoffset, yoffset, width, height, format, type, bufSize, pixels);
// }
// 
// GL_APICALL void GL_APIENTRY glCompressedTexImage2DRobustANGLE(GLenum target, int level, GLenum internalformat, GLsizei width, GLsizei height, int border, GLsizei imageSize, GLsizei bufSize, const void* data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glCompressedTexImage2DRobustANGLE(target, level, internalformat, width, height, border, imageSize, bufSize, data);
// }
// 
// GL_APICALL void GL_APIENTRY glCompressedTexSubImage2DRobustANGLE(GLenum target, int level, int xoffset, int yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, GLsizei bufSize, const void* data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glCompressedTexSubImage2DRobustANGLE(target, level, xoffset, yoffset, width, height, format, imageSize, bufSize, data);
// }
// 
// GL_APICALL void GL_APIENTRY glCompressedTexImage3DRobustANGLE(GLenum target, int level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, int border, GLsizei imageSize, GLsizei bufSize, const void* data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glCompressedTexImage3DRobustANGLE(target, level, internalformat, width, height, depth, border, imageSize, bufSize, data);
// }
// 
// GL_APICALL void GL_APIENTRY glCompressedTexSubImage3DRobustANGLE(GLenum target, int level, int xoffset, int yoffset, int zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, GLsizei bufSize, const void* data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glCompressedTexSubImage3DRobustANGLE(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bufSize, data);
// }
// 
// GL_APICALL void GL_APIENTRY glTexImage3DRobustANGLE(GLenum target, int level, int internalformat, GLsizei width, GLsizei height, GLsizei depth, int border, GLenum format, GLenum type, GLsizei bufSize, const void * pixels) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexImage3DRobustANGLE(target, level, internalformat, width, height, depth, border, format, type, bufSize, pixels);
// }
// 
// GL_APICALL void GL_APIENTRY glTexSubImage3DRobustANGLE(GLenum target, int level, int xoffset, int yoffset, int zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, const void * pixels) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexSubImage3DRobustANGLE(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels);
// }
// 
// GL_APICALL void GL_APIENTRY glGetQueryivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetQueryivRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetQueryObjectuivRobustANGLE(GLuint id, GLenum pname, GLsizei bufSize, GLsizei * length, GLuint * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetQueryObjectuivRobustANGLE(id, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetBufferPointervRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, void ** params) {
//     GET_CTX_V2();
//     SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
//     ctx->dispatcher().glGetBufferPointervRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetIntegeri_vRobustANGLE(GLenum target, GLuint index, GLsizei bufSize, GLsizei * length, int * data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetIntegeri_vRobustANGLE(target, index, bufSize, length, data);
// }
// 
// GL_APICALL void GL_APIENTRY glGetInternalformativRobustANGLE(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetInternalformativRobustANGLE(target, internalformat, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetVertexAttribIivRobustANGLE(GLuint index, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetVertexAttribIivRobustANGLE(index, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetVertexAttribIuivRobustANGLE(GLuint index, GLenum pname, GLsizei bufSize, GLsizei * length, GLuint * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetVertexAttribIuivRobustANGLE(index, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetUniformuivRobustANGLE(GLuint program, int location, GLsizei bufSize, GLsizei * length, GLuint * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetUniformuivRobustANGLE(globalProgramName, location, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetActiveUniformBlockivRobustANGLE(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetActiveUniformBlockivRobustANGLE(globalProgramName, uniformBlockIndex, pname, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetInteger64vRobustANGLE(GLenum pname, GLsizei bufSize, GLsizei * length, GLint64 * data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetInteger64vRobustANGLE(pname, bufSize, length, data);
// }
// 
// GL_APICALL void GL_APIENTRY glGetInteger64i_vRobustANGLE(GLenum target, GLuint index, GLsizei bufSize, GLsizei * length, GLint64 * data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetInteger64i_vRobustANGLE(target, index, bufSize, length, data);
// }
// 
// GL_APICALL void GL_APIENTRY glGetBufferParameteri64vRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, GLint64 * params) {
//     GET_CTX_V2();
//     SET_ERROR_IF(!GLESv2Validate::bufferTarget(ctx, target),GL_INVALID_ENUM);
//     ctx->dispatcher().glGetBufferParameteri64vRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glSamplerParameterivRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, const GLint * param) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glSamplerParameterivRobustANGLE(globalSampler, pname, bufSize, param);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glSamplerParameterfvRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, const GLfloat * param) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glSamplerParameterfvRobustANGLE(globalSampler, pname, bufSize, param);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetSamplerParameterivRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glGetSamplerParameterivRobustANGLE(globalSampler, pname, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetSamplerParameterfvRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, GLsizei * length, float * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glGetSamplerParameterfvRobustANGLE(globalSampler, pname, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetFramebufferParameterivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetFramebufferParameterivRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetProgramInterfaceivRobustANGLE(GLuint program, GLenum programInterface, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetProgramInterfaceivRobustANGLE(globalProgramName, programInterface, pname, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetBooleani_vRobustANGLE(GLenum target, GLuint index, GLsizei bufSize, GLsizei * length, GLboolean * data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetBooleani_vRobustANGLE(target, index, bufSize, length, data);
// }
// 
// GL_APICALL void GL_APIENTRY glGetMultisamplefvRobustANGLE(GLenum pname, GLuint index, GLsizei bufSize, GLsizei * length, float * val) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetMultisamplefvRobustANGLE(pname, index, bufSize, length, val);
// }
// 
// GL_APICALL void GL_APIENTRY glGetTexLevelParameterivRobustANGLE(GLenum target, int level, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetTexLevelParameterivRobustANGLE(target, level, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetTexLevelParameterfvRobustANGLE(GLenum target, int level, GLenum pname, GLsizei bufSize, GLsizei * length, float * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetTexLevelParameterfvRobustANGLE(target, level, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetPointervRobustANGLE(GLenum pname, GLsizei bufSize, GLsizei * length, void ** params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetPointervRobustANGLE(pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glReadnPixelsRobustANGLE(int x, int y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, GLsizei * length, GLsizei * columns, GLsizei * rows, void * data) {
//     GET_CTX_V2();
//     ctx->dispatcher().glReadnPixelsRobustANGLE(x, y, width, height, format, type, bufSize, length, columns, rows, data);
// }
// 
// GL_APICALL void GL_APIENTRY glGetnUniformfvRobustANGLE(GLuint program, int location, GLsizei bufSize, GLsizei * length, float * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetnUniformfvRobustANGLE(globalProgramName, location, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetnUniformivRobustANGLE(GLuint program, int location, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetnUniformivRobustANGLE(globalProgramName, location, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetnUniformuivRobustANGLE(GLuint program, int location, GLsizei bufSize, GLsizei * length, GLuint * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalProgramName = ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER_OR_PROGRAM, program);
//    
//     ctx->dispatcher().glGetnUniformuivRobustANGLE(globalProgramName, location, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glTexParameterIivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, const GLint * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexParameterIivRobustANGLE(target, pname, bufSize, params);
// }
// 
// GL_APICALL void GL_APIENTRY glTexParameterIuivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, const GLuint * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glTexParameterIuivRobustANGLE(target, pname, bufSize, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetTexParameterIivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetTexParameterIivRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetTexParameterIuivRobustANGLE(GLenum target, GLenum pname, GLsizei bufSize, GLsizei * length, GLuint * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetTexParameterIuivRobustANGLE(target, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glSamplerParameterIivRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, const GLint * param) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glSamplerParameterIivRobustANGLE(globalSampler, pname, bufSize, param);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glSamplerParameterIuivRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, const GLuint * param) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glSamplerParameterIuivRobustANGLE(globalSampler, pname, bufSize, param);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetSamplerParameterIivRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glGetSamplerParameterIivRobustANGLE(globalSampler, pname, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetSamplerParameterIuivRobustANGLE(GLuint sampler, GLenum pname, GLsizei bufSize, GLsizei * length, GLuint * params) {
//     GET_CTX_V2();
//     if (ctx->shareGroup().get()) {
//         const GLuint globalSampler = ctx->shareGroup()->getGlobalName(NamedObjectType::SAMPLER, sampler);
//    
//     ctx->dispatcher().glGetSamplerParameterIuivRobustANGLE(globalSampler, pname, bufSize, length, params);
//     }
// }
// 
// GL_APICALL void GL_APIENTRY glGetQueryObjectivRobustANGLE(GLuint id, GLenum pname, GLsizei bufSize, GLsizei * length, int * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetQueryObjectivRobustANGLE(id, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetQueryObjecti64vRobustANGLE(GLuint id, GLenum pname, GLsizei bufSize, GLsizei * length, GLint64 * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetQueryObjecti64vRobustANGLE(id, pname, bufSize, length, params);
// }
// 
// GL_APICALL void GL_APIENTRY glGetQueryObjectui64vRobustANGLE(GLuint id, GLenum pname, GLsizei bufSize, GLsizei * length, GLuint64 * params) {
//     GET_CTX_V2();
//     ctx->dispatcher().glGetQueryObjectui64vRobustANGLE(id, pname, bufSize, length, params);
// }
// 
