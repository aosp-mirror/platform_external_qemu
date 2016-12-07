custom_impls = {
        "glGenVertexArrays" : """
    ctx->addVertexArrayObjects(n, arrays);
""",

        "glBindVertexArray" : """
    ctx->setVertexArrayObject(array);
""",
        
        "glDeleteVertexArrays" : """
    ctx->removeVertexArrayObjects(n, arrays);
""",
}

custom_share_processing = {

}
