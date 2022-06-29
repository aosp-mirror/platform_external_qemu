android_add_library(
  TARGET _nimble_os_abstraction
  LICENSE Apache-2.0
  SRC # src/hal_timer.c # Unused we are using emulator loopers.
      src/endian.c
      src/mem.c
      # src/os_cputime_pwr2.c # Unused src/nimble_port.c # Functions have moved
      # to os_sysinit.cc
      src/os_mbuf.c
      src/os_mempool.c
      # src/os_cputime.c # Unused, we "fake time"
      src/os_msys_init.c)
target_include_directories(_nimble_os_abstraction PUBLIC include)
target_link_libraries(
  _nimble_os_abstraction PRIVATE _nimble_os_abstraction_headers android-emu-base
  PUBLIC android-emu-base-logging)
target_compile_options(_nimble_os_abstraction PRIVATE -std=c11
                                                      -Wno-pointer-to-int-cast)
