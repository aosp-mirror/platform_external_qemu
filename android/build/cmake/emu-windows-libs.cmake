# This function discovers the standard windows libraries.
# Linking in these libraries is done differently in MSVC vs Clang/Mingw
#
# This function will declare the library ${NAME}::${NAME} that you can
# take a dependency on, turning it into to the correct dependency.
function(android_find_windows_library NAME)
  if(MSVC)
    find_library(${NAME}_LIBRARIES ${NAME})
  else()
    set(${NAME}_LIBRARIES "${NAME}")
  endif()

  if(NOT TARGET ${NAME}::${NAME})
    add_library(${NAME}::${NAME} INTERFACE IMPORTED GLOBAL)
    set_target_properties(${NAME}::${NAME} PROPERTIES INTERFACE_LINK_LIBRARIES ${${NAME}_LIBRARIES})
    android_license(TARGET "${NAME}::${NAME}" LIBNAME None SPDX None LICENSE None LOCAL None)
  endif()
endfunction()

set(WINDOWS_LIBS
    atls
    ws2_32
    d3d9
    ole32
    psapi
    iphlpapi
    wldap32
    shell32
    user32
    advapi32
    secur32
    mfuuid
    msdmo
    dmoguids
    wmcodecdspuuid
    amstrmid
    winmm
    shlwapi
    gdi32
    dxguid
    wininet
    diaguids
    dbghelp
    normaliz
    crypt32
    mincore
    imagehlp)
foreach(LIB ${WINDOWS_LIBS})
  android_find_windows_library(${LIB})
endforeach()

if(WIN32 AND MSVC)  
   # We expect all the visual studio variables to be set. The rebuild script will take care of this..
   # If you are building from 
  get_filename_component(VCTOOLS_PATH "$ENV{VCToolsInstallDir}" ABSOLUTE CACHE)
  get_filename_component(VSINSTALLDIR "$ENV{VSINSTALLDIR}" ABSOLUTE CACHE)

  # Find the DIA SDK path, it will typically look something like this.
  find_path(DIASDK_INCLUDE_DIR # Set variable DIASDK_INCLUDE_DIR
            dia2.h # Find a path with dia2.h
            HINTS "${VSINSTALLDIR}/DIA SDK/include"
            DOC "path to DIA SDK header files")
  message(STATUS "Found DIA SDK Include: ${DIASDK_INCLUDE_DIR}")

  find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib/amd64)


  set_target_properties(diaguids::diaguids
                        PROPERTIES INTERFACE_LINK_LIBRARIES ${DIASDK_GUIDS_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES
                                   ${DIASDK_INCLUDE_DIR})

  # Find the ATL path, it will typically look something like this.
  #C:\Program Files (x86)\Microsoft Visual Studio\2017\xxx\VC\Tools\MSVC\14.16.27023\atlmfc\include
  find_path(ATL_INCLUDE_DIR # Set variable DIASDK_INCLUDE_DIR
            atlbase.h # Find a path with atlbase.h
            HINTS "${VCTOOLS_PATH}/atlmfc/include"
            DOC "path to ATL header files")
  message(STATUS "Found ATL Include: ${ATL_INCLUDE_DIR}")

  find_library(ATL_LIBRARY NAMES atls.lib HINTS ${ATL_INCLUDE_DIR}/../lib/x64)
  set_target_properties(atls::atls
                        PROPERTIES INTERFACE_LINK_LIBRARIES ${ATL_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES
                                   ${ATL_INCLUDE_DIR})

endif()
