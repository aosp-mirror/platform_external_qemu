add_library(
  _nimble_os_abstraction
  # src/hal_timer.c
  src/os_mbuf.c
  src/nimble_port.c
  src/mem.c
  # src/os_cputime_pwr2.c
  src/os_mempool.c
  # src/os_cputime.c
  src/endian.c
  src/os_msys_init.c)
target_include_directories(_nimble_os_abstraction PUBLIC include)
target_link_libraries(_nimble_os_abstraction PRIVATE _nimble_os_abstraction_headers _nimble_host_layer _nimble_store_config android-emu-base)
target_compile_options(_nimble_os_abstraction PRIVATE -std=c11)
