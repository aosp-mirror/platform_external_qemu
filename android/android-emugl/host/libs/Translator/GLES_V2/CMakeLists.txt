list(APPEND SOURCES
  GLESv2Context.cpp
  GLESv2Imp.cpp
  GLESv2Validate.cpp
  ProgramData.cpp
  SamplerData.cpp
  ShaderParser.cpp
  ShaderValidator.cpp
  )

if(NOT WIN32)

  list(APPEND SOURCES
    ANGLEShaderParser.cpp)
endif()

include_directories(${ANGLE_INCLUDE_DIRS})
add_library(GLES_V2_translator ${SOURCES})
target_link_libraries(GLES_V2_translator GLcommon OpenglCodecCommon
  ANDROID_BASE_LIBRARIES ${ANGLE_LIBRARIES} ${EXTRA_LIBS})
