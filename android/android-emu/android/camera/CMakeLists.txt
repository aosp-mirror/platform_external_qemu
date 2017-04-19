list(APPEND SOURCES
  camera-common.cpp
  camera-format-converters.c
  camera-list.cpp
  camera-service.c
  )



macro(ADD_OSX_FRAMEWORK fwname target)
    find_library(FRAMEWORK_${fwname}
    NAMES ${fwname}
    PATHS ${CMAKE_OSX_SYSROOT}/System/Library
    PATH_SUFFIXES Frameworks
    NO_DEFAULT_PATH)
    if( ${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
        MESSAGE(ERROR ": Framework ${fwname} not found")
    else()
        TARGET_LINK_LIBRARIES(${target} PUBLIC "${FRAMEWORK_${fwname}}/${fwname}")
        MESSAGE(STATUS "Framework ${fwname} found at ${FRAMEWORK_${fwname}}")
    endif()
endmacro(ADD_OSX_FRAMEWORK)

if(APPLE)
  #   list(APPEND SOURCES camera-capture-mac.m)
elseif(UNIX)
  list(APPEND SOURCES camera-capture-linux.c)
elseif(WIN32)
  list(APPEND SOURCES camera-capture-windows.cpp)
endif()

add_library(camera ${SOURCES})
ADD_OSX_FRAMEWORK(QtKit camera) 
target_link_libraries(camera ${EXTRA_LIBS})

