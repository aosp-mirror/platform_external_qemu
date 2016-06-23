GenerateDecoder(gles2 GLESV2_DEC_GEN)

add_library(GLESv2_dec STATIC
    GLESv2Decoder.cpp
    ${GLESV2_DEC_GEN}
    )

target_include_directories(GLESv2_dec
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    )

target_link_libraries(GLESv2_dec OpenglCodecCommon emugl_common)


