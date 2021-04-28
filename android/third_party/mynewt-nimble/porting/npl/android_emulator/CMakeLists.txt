add_library(
  _nimble_os_impl
  src/hci_bridge.cc
  src/os_atomic.cc
  src/os_callout.cc
  src/os_eventq.cc
  src/os_mutex.cc
  src/os_sem.cc
  src/os_sysinit.cc
  src/os_task.cc
  src/os_time.cc)

target_link_libraries(
  _nimble_os_impl
  PUBLIC _nimble_host_layer _nimble_os_abstraction
         _nimble_os_abstraction_headers _nimble_transport_socket
         android-emu-base)

if (WINDOWS_MSVC_X86_64)
    target_link_libraries(_nimble_os_impl PRIVATE msvc-posix-compat)
endif()


set(NPL_TESTS test_npl_callout test_npl_eventq test_npl_sem test_npl_task)
foreach(TEST ${NPL_TESTS})
  add_executable(${TEST} test/${TEST}.cc)
  add_test(NAME ${TEST} COMMAND ${TEST})
  target_link_libraries(${TEST} _nimble_os_impl)
endforeach()
