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
