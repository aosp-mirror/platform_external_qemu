set(SOURCES
    ChecksumCalculator.cpp
    ChecksumCalculator.h
    ChecksumCalculatorThreadInfo.cpp
    ChecksumCalculatorThreadInfo.h
    CMakeLists.txt
    ErrorLog.h
    gl_base_types.h
    GLDecoderContextData.h
    glUtils.cpp
    glUtils.h
    Makefile
    ProtocolUtils.h)

add_library(OpenglCodecCommon ${SOURCES})
target_link_libraries(OpenglCodecCommon emugl_common)
