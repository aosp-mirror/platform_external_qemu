set(LIBEVENT_ROOT ${AOSP_ROOT}/external/libchrome/base/third_party/libevent)
android_add_library(
  TARGET libevent
  LICENSE BSD-3-Clause
  SOURCE_DIR ${LIBEVENT_ROOT}
  SRC # cmake-format: sortable
      buffer.c
      epoll.c
      evbuffer.c
      evdns.c
      event.c
      event_tagging.c
      evrpc.c
      evutil.c
      http.c
      log.c
      log.h
      poll.c
      select.c
      signal.c
      strlcpy.c)
target_include_directories(
  libevent PUBLIC ${LIBEVENT_ROOT}/compat ${LIBEVENT_ROOT}
                  ${LIBEVENT_ROOT}/../../../ ${LIBEVENT_ROOT}/linux)
target_compile_options(libevent PRIVATE -Wno-implicit-function-declaration
                                        -Wno-int-conversion)
target_compile_definitions(libevent PRIVATE -DHAVE_CONFIG_H=1)
