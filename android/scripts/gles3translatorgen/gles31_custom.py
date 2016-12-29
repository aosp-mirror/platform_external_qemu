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

}

custom_postprocesses = {
}

custom_share_processing = {
}

no_passthrough = {
}
