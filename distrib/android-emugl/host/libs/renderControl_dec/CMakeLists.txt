GenerateDecoder(renderControl RENDERCONTROL_DEC_GEN)

add_library(renderControl_dec STATIC
    ${RENDERCONTROL_DEC_GEN}
    )

target_include_directories(renderControl_dec
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_LIST_DIR}
    )

target_link_libraries(renderControl_dec OpenglCodecCommon emugl_common)


#[[
# For renderControl_types.h
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH))
]]

