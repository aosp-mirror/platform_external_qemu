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
}

custom_postprocesses = {
        "glGenVertexArrays" : """
    ctx->addVertexArrayObjects(n, arrays);
""",
}

custom_share_processing = {

}
