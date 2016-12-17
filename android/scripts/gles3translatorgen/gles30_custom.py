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
}

custom_postprocesses = {
        "glGenVertexArrays" : """
    ctx->addVertexArrayObjects(n, arrays);
""",
        "glVertexAttribIPointer" : """
    }
""",
}

custom_share_processing = {

}
