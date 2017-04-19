list(APPEND SOURCES
  aconfig-file.c
  #aconfig-file_unittest.cpp
  assert.c
  async.cpp
  bufprint.c
  bufprint_system.cpp
  #bufprint_unittest.cpp
  cbuffer.c
  debug.c
  debug_wrapper.cpp
  dirscanner.cpp
  #dirscanner_unittest.cpp
  dll.c
  dns.cpp
  #dns_unittest.cpp
  eintr_wrapper.c
  #eintr_wrapper_unittest.cpp
  exec.cpp
  fd.cpp
  file_data.c
  #file_data_unittest.cpp
  file_io.cpp
  filelock.cpp
  format.cpp
  #format_unittest.cpp
  host_bitness.cpp
  #host_bitness_unittest.cpp
  http_utils.cpp
  ini.cpp
  intmap.c
  iolooper.cpp
  ipaddr.cpp
  lineinput.c
  lock.cpp
  looper.cpp
  mapfile.c
  misc.c
  panic.c
  path.cpp
  path_system.cpp
  #path_unittest.cpp
  property_file.c
  #property_file_unittest.cpp
  reflist.c
  refset.c
  setenv.c
  socket_drainer.cpp
  sockets.c
  #sockets_unittest.cpp
  stralloc.c
  stream.cpp
  string.cpp
  #string_unittest.cpp
  system.c
  system_wrapper.cpp
  tempfile.c
  timezone.cpp
  #timezone_unittest.cpp
  uri.cpp
  utf8_utils.cpp
  vector.c
  x86_cpuid.cpp
  #x86_cpuid_unittest.cpp
  )

if (WIN32) 
  list(APPEND SOURCES
    win32_cmdline_quote.cpp
    #win32_cmdline_quote_unittest.cpp
    win32_unicode.cpp
    )
endif()


add_library(utils ${SOURCES})

