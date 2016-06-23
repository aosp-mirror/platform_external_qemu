
add_library(OpenglCodecCommon STATIC
    glUtils.cpp
    ChecksumCalculator.cpp
    ChecksumCalculatorThreadInfo.cpp
    )

target_link_libraries(OpenglCodecCommon PRIVATE emugl_common)
target_include_directories(OpenglCodecCommon PUBLIC
    ./
    ../../host/include/libOpenglRender/
    )
