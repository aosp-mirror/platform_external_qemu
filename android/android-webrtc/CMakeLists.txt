cmake_minimum_required(VERSION 3.3)
project(goldfish-webrtc)

message(
  WARNING "Enabling WebRTC support. This will do a full checkout and build of WebRTC@62, this will take some time...")
include(ExternalProject)
set(WEBRTC_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/external/install)
set(WEBRTC_CMAKE_PREFIX_ROOT ${CMAKE_CURRENT_BINARY_DIR}/external/tmp/webrtc_modules)
set(WEBRTC_SRC_ROOT "${WEBRTC_CMAKE_PREFIX_ROOT}/src/project_webrtc-build/webrtc/src")

externalproject_add(project_webrtc
                    DOWNLOAD_COMMAND ""
                    PREFIX ${WEBRTC_CMAKE_PREFIX_ROOT}
                    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libwebrtc
                    LOG_BUILD TRUE
                    INSTALL_DIR ${WEBRTC_INSTALL_DIR}}
                    CMAKE_ARGS
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/external/install
                    -DCMAKE_BUILD_TYPE=Release)
add_subdirectory(videobridge)

