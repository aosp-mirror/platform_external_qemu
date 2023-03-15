function(android_grpc_service_lib)
  # Parse arguments
  set(options)
  set(oneValueArgs TARGET SOURCE_DIR)
  set(multiValueArgs SOURCES)
  cmake_parse_arguments(grpc "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  if(NOT grpc_SOURCE_DIR)
    set(grpc_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
  endif()
  protobuf_generate_grpc_cpp(
    SOURCE_DIR ${grpc_SOURCE_DIR} SOURCES ${grpc_SOURCES}
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} GENERATED ${grpc_TARGET}_SOURCES)

  android_add_library(TARGET ${grpc_TARGET} LICENSE Apache-2.0
                      SRC ${${grpc_TARGET}_SOURCES})

  target_link_libraries(${grpc_TARGET} PUBLIC protobuf::libprotobuf grpc++)
  target_include_directories(${grpc_TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

add_subdirectory(client)
add_subdirectory(event-waiter)
add_subdirectory(interceptors)
add_subdirectory(security)
add_subdirectory(services-stack)
add_subdirectory(services)
