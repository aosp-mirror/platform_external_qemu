cmake_minimum_required(VERSION 3.23)

set(_CORROSION_TOP_LEVEL OFF)
option(
    CORROSION_DEV_MODE
    "Enables some additional features if you're developing Corrosion"
    ${_CORROSION_TOP_LEVEL}
)
set(CORROSION_INSTALL_EXECUTABLE_DEFAULT OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Corrosion)
