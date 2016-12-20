custom_preprocesses = {
"glBindVertexArray" : """
    ctx->setVertexArrayObject(array);
""",
        
"glDeleteVertexArrays" : """
    ctx->removeVertexArrayObjects(n, arrays);
""",

"glBindBufferRange" : """
    ctx->bindBuffer(target, buffer);
    ctx->bindIndexedBuffer(target, index, buffer, offset, size);
""",

"glBindBufferBase" : """
    ctx->bindBuffer(target, buffer);
    ctx->bindIndexedBuffer(target, index, buffer);
""",

"glVertexAttribIPointer" : """
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->setPointer(index,size,type,stride,pointer,false,true);
    if (ctx->isBindedBuffer(GL_ARRAY_BUFFER)) {
""",

"glVertexAttribDivisor" : """
    SET_ERROR_IF((!GLESv2Validate::arrayIndex(ctx,index)),GL_INVALID_VALUE);
    ctx->setDivisor(index,divisor);
""",

"glRenderbufferStorageMultisample" : """
    GLint err = GL_NO_ERROR;
    internalformat = sPrepareRenderbufferStorage(internalformat, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
""",

"glFramebufferTextureLayer" : """
    GLenum textarget = GL_TEXTURE_2D_ARRAY;
    SET_ERROR_IF(!(GLESv2Validate::framebufferTarget(target) &&
                   GLESv2Validate::framebufferAttachment(
                       attachment, ctx->getMajorVersion())), GL_INVALID_ENUM);
    if (texture) {
        SET_ERROR_IF(ctx->getBindedTexture(GL_TEXTURE_2D_ARRAY) != texture &&
                     ctx->getBindedTexture(GL_TEXTURE_3D) != texture,
                     GL_INVALID_OPERATION);
        if (texture == ctx->getBindedTexture(GL_TEXTURE_2D_ARRAY))
            textarget = GL_TEXTURE_2D_ARRAY;
        if (texture == ctx->getBindedTexture(GL_TEXTURE_3D))
            textarget = GL_TEXTURE_3D;
        if (!ctx->shareGroup()->isObject(NamedObjectType::TEXTURE, texture)) {
            ctx->shareGroup()->genName(NamedObjectType::TEXTURE, texture);
        }
    }
""",

"glTexStorage2D" : """
    GLint err = GL_NO_ERROR;
    GLenum format, type;
    GLESv2Validate::getCompatibleFormatTypeForInternalFormat(internalformat, &format, &type);
    for (GLsizei i = 0; i < levels; i++)
        sPrepareTexImage2D(target, i, (GLint)internalformat, width, height, 0, format, type, NULL, &type, (GLint*)&internalformat, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
""",

"glGenSamplers" : """
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            samplers[i] = ctx->shareGroup()->genName(NamedObjectType::SAMPLER,
                                                     0, true);
        }
    }
""",

"glDeleteSamplers" : """
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            ctx->shareGroup()->deleteName(NamedObjectType::SAMPLER, samplers[i]);
        }
    }
""",

"glGenQueries" : """
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            ids[i] = ctx->shareGroup()->genName(NamedObjectType::QUERY,
                                                     0, true);
        }
    }
""",

"glDeleteQueries" : """
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            ctx->shareGroup()->deleteName(NamedObjectType::QUERY, ids[i]);
        }
    }
""",

"glTexImage3D" : """
    s_glInitTexImage3D(target, level, internalFormat, width, height, depth, border);
""",

"glTexStorage3D" : """
    for (int i = 0; i < levels; i++) {
        s_glInitTexImage3D(target, i, internalformat, width, height, depth, 0);
    }
""",

"glDrawArraysInstanced" : """
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode)),GL_INVALID_ENUM);
    s_glDrawPre(ctx, mode);
    if (ctx->isBindedBuffer(GL_ARRAY_BUFFER)) {
        ctx->dispatcher().glDrawArraysInstanced(mode, first, count, primcount);
    } else {
        GLESConversionArrays tmpArrs;
        s_glDrawSetupArraysPre(ctx, tmpArrs, first, count, 0, NULL, true);
        ctx->dispatcher().glDrawArrays(mode,first,count);
        s_glDrawSetupArraysPost(ctx);
    }
    s_glDrawPost(ctx, mode);
""",

"glDrawElementsInstanced" : """
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode) && GLESv2Validate::drawType(type)),GL_INVALID_ENUM);
    s_glDrawPre(ctx, mode);
    if (ctx->isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER)) {
        ctx->dispatcher().glDrawElementsInstanced(mode,count,type,indices, primcount);
    } else {
        s_glDrawEmulateClientArraysPre(ctx);
        GLESConversionArrays tmpArrs;
        s_glDrawSetupArraysPre(ctx,tmpArrs,0,count,type,indices,false);
        ctx->dispatcher().glDrawElementsInstanced(mode,count,type,indices, primcount);
        s_glDrawSetupArraysPost(ctx);
        s_glDrawEmulateClientArraysPost(ctx);
    }
    s_glDrawPost(ctx, mode);
""",

"glDrawRangeElements" : """
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode) && GLESv2Validate::drawType(type)),GL_INVALID_ENUM);
    s_glDrawPre(ctx, mode);
    if (ctx->isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER)) {
        ctx->dispatcher().glDrawRangeElements(mode,start,end,count,type,indices);
    } else {
        s_glDrawEmulateClientArraysPre(ctx);
        GLESConversionArrays tmpArrs;
        s_glDrawSetupArraysPre(ctx,tmpArrs,0,count,type,indices,false);
        ctx->dispatcher().glDrawRangeElements(mode,start,end,count,type,indices);
        s_glDrawSetupArraysPost(ctx);
        s_glDrawEmulateClientArraysPost(ctx);
    }
    s_glDrawPost(ctx, mode);
""",
}

custom_postprocesses = {
"glGenVertexArrays" : """
    ctx->addVertexArrayObjects(n, arrays);
""",
"glVertexAttribIPointer" : """
    }
""",
"glFramebufferTextureLayer" : """
    GLuint fbName = ctx->getFramebufferBinding();
    auto fbObj = ctx->shareGroup()->getObjectData(
            NamedObjectType::FRAMEBUFFER, fbName);
    if (fbObj) {
        FramebufferData *fbData = (FramebufferData *)fbObj;
        fbData->setAttachment(attachment, textarget,
                              texture, ObjectDataPtr());
    }
""",
}

custom_share_processing = {

}

no_passthrough = {
    "glGenSamplers": True,
    "glDeleteSamplers": True,
    "glGenQueries": True,
    "glDeleteQueries": True,
}
