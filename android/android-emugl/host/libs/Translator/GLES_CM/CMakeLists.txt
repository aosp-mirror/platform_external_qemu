set(SOURCES
    GLEScmImp.cpp
    GLEScmUtils.cpp
    GLEScmContext.cpp
    GLEScmValidate.cpp)

add_library(GLES_CM_translator ${SOURCES})
target_link_libraries(GLES_CM_translator GLcommon ANDROID_BASE_LIBRARIES ${EXTRA_LIBS})
