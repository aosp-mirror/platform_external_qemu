function(GenerateDecoder EMUGEN_VER RETURN_VALUE)
    set(EMUGEN_OUT_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/${EMUGEN_VER})
    set(EMUGEN_IN_PREFIX ${CMAKE_CURRENT_LIST_DIR}/${EMUGEN_VER})

    set(EMUGEN_IN
        ${EMUGEN_IN_PREFIX}.attrib
        ${EMUGEN_IN_PREFIX}.in
        ${EMUGEN_IN_PREFIX}.types
        )

    set(EMUGEN_OUT
        ${EMUGEN_OUT_PREFIX}_dec.cpp
        ${EMUGEN_OUT_PREFIX}_dec.h
        ${EMUGEN_OUT_PREFIX}_opcodes.h
        ${EMUGEN_OUT_PREFIX}_server_context.h
        ${EMUGEN_OUT_PREFIX}_server_context.cpp
        )
    set(${RETURN_VALUE} ${EMUGEN_OUT} PARENT_SCOPE)

    add_custom_command(
        OUTPUT ${EMUGEN_OUT}
        COMMAND ${CMAKE_BINARY_DIR}/host/emugen/emugen -D ${CMAKE_CURRENT_BINARY_DIR} -i ${CMAKE_CURRENT_LIST_DIR} ${EMUGEN_VER}
        DEPENDS emugen ${EMUGEN_IN}
        )
endfunction()

