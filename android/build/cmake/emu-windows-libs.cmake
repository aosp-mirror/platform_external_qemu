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
    message(STATUS "Discovering ${NAME} : ${${NAME}_LIBRARIES}")
    add_library(${NAME}::${NAME} INTERFACE IMPORTED GLOBAL)
    set_target_properties(${NAME}::${NAME} PROPERTIES INTERFACE_LINK_LIBRARIES ${${NAME}_LIBRARIES})
    get_target_property(VAR ${NAME}::${NAME} INTERFACE_LINK_LIBRARIES)
  endif()
endfunction()

set(WINDOWS_LIBS "ws2_32;d3d9;ole32;psapi;iphlpapi;wldap32;shell32;user32;advapi32;mfuuid;winmm;shlwapi;gdi32;dxguid")
foreach(LIB ${WINDOWS_LIBS})
  android_find_windows_library(${LIB})
endforeach()
