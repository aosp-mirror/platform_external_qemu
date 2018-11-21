function(android_find_windows_library NAME)

if(MSVC)
   find_library(${NAME}_LIBRARIES ${NAME})
else()
   set(${NAME}_LIBRARIES "${NAME}")
endif()

add_library(${NAME}::${NAME} INTERFACE IMPORTED GLOBAL)
set_target_properties(${NAME}::${NAME} PROPERTIES INTERFACE_LINK_LIBRARIES ${${NAME}_LIBRARIES})
get_target_property(VAR ${NAME}::${NAME} INTERFACE_LINK_LIBRARIES)
message(STATUS "${NAME} available from ${${NAME}_LIBRARIES} -- ${VAR}")

endfunction()

set(WINDOWS_LIBS "ws2_32;d3d9;ole32;psapi;Iphlpapi;Wldap32")
foreach(LIB ${WINDOWS_LIBS})
  android_find_windows_library(${LIB})
endforeach()
