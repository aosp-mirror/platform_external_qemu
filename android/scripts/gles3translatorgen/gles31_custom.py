custom_preprocesses = {
"glBindVertexBuffer" : """
    ctx->bindIndexedBuffer(0, bindingindex, buffer, offset, 0, stride);
""",

"glVertexAttribBinding" : """
    ctx->setVertexAttribBindingIndex(attribindex, bindingindex);
""",

"glVertexAttribFormat" : """
    ctx->setVertexAttribFormat(attribindex, size, type, normalized, relativeoffset, false);
""",

"glVertexAttribIFormat" : """
    ctx->setVertexAttribFormat(attribindex, size, type, GL_FALSE, relativeoffset, true);
""",

"glVertexBindingDivisor" : """
    ctx->setVertexAttribDivisor(bindingindex, divisor);
""",

"glTexStorage2DMultisample" : """
    GLint err = GL_NO_ERROR;
    GLenum format, type;
    GLESv2Validate::getCompatibleFormatTypeForInternalFormat(internalformat, &format, &type);
    sPrepareTexImage2D(target, 0, (GLint)internalformat, width, height, 0, format, type, NULL, &type, (GLint*)&internalformat, &err);
    SET_ERROR_IF(err != GL_NO_ERROR, err);
""",
}

custom_postprocesses = {
}

custom_share_processing = {
}

no_passthrough = {
}
