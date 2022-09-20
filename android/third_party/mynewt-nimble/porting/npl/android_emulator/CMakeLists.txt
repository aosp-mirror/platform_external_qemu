android_add_library(
  TARGET _nimble_os_impl
  LICENSE Apache-2.0
  SRC src/os_atomic.cc
      src/os_callout.cc
      src/os_eventq.cc
      src/os_mutex.cc
      src/os_sem.cc
      src/os_task.cc
      src/os_time.cc
  DEPS android-emu-base-headers)

target_link_libraries(_nimble_os_impl PUBLIC _nimble_os_abstraction_headers
                                             android-emu-base)

set(NPL_TESTS test_npl_callout test_npl_eventq test_npl_sem test_npl_task)
foreach(TEST ${NPL_TESTS})
  android_add_executable(TARGET ${TEST} NODISTRIBUTE SRC test/${TEST}.cc)
  target_link_libraries(${TEST} PRIVATE _nimble_os_impl android-emu-base-headers)
  target_compile_options(${TEST} PRIVATE -O0)

  # Nimble is designed with the assumption that it is running as a low level stack
  # on top of the os and is expected to never shutdown (we are missing sysdown_release implementation)
  # due to this nimble leaks event queues on exit, for now we will not run these tests under
  # asan until we resolve b/186763119
  if (OPTION_ASAN)
      message(STATUS "Not running nimble test: ${TEST} under asan, see b/186763119")
  else()
      add_test(NAME ${TEST} COMMAND ${TEST})
  endif()
endforeach()

# This library contains the system initialization routines.
android_add_library(TARGET _nimble_os_sysinit LICENSE Apache-2.0
                    SRC src/os_sysinit.cc DEPS android-emu-base-headers)
target_link_libraries(
    _nimble_os_sysinit
    PUBLIC _nimble_os_abstraction_headers _nimble_host_layer
            _nimble_store_config android-emu-base)
target_compile_options(_nimble_os_sysinit PRIVATE -Wno-extern-c-compat)
