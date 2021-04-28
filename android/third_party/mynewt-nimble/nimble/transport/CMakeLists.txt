android_add_library(
  TARGET _nimble_transport_socket LICENSE Apache-2.0
  SRC socket/src/android_sockets.cc socket/src/ble_hci_socket.c)
target_include_directories(_nimble_transport_socket PRIVATE socket/src
                           PUBLIC socket/include)
target_compile_options(_nimble_transport_socket
                       PRIVATE -Wno-implicit-function-declaration)
target_link_libraries(_nimble_transport_socket
                      PRIVATE _nimble_os_abstraction_headers android-emu-base)

if(WINDOWS_MSVC_X86_64)
  target_link_libraries(_nimble_transport_socket PRIVATE msvc-posix-compat)
endif()
