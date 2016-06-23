GenerateDecoder(gles1 GLESV1_DEC_GEN)

add_library(GLESv1_dec STATIC
    GLESv1Decoder.cpp
    ${GLESV1_DEC_GEN}
    )

target_include_directories(GLESv1_dec
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    )

target_link_libraries(GLESv1_dec OpenglCodecCommon emugl_common)


