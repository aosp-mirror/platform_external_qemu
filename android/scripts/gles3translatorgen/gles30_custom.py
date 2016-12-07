custom_preprocesses = {
        "glBindVertexArray" : """
    ctx->setVertexArrayObject(array);
""",
        
        "glDeleteVertexArrays" : """
    ctx->removeVertexArrayObjects(n, arrays);
""",
}

custom_postprocesses = {
        "glGenVertexArrays" : """
    ctx->addVertexArrayObjects(n, arrays);
""",
}

custom_share_processing = {

}
