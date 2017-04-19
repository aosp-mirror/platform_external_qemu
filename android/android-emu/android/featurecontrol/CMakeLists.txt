list(APPEND SOURCES
  feature_control.cpp
  FeatureControl.cpp
  #FeatureControl_unittest.cpp
  FeatureControlImpl.cpp
  HWMatching.cpp
  #HWMatching_unittest.cpp
  )

add_subdirectory(proto)

# The generated proto headers end up in a directory below us, so we need
# to make sure we pick them up
include_directories( $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>/android-emu)
add_library(featurecontrol  ${SOURCES})
