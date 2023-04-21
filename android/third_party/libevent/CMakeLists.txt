set(LIBEVENT_ROOT ${AOSP_ROOT}/external/webrtc/third_party/libevent)
android_add_library(
  TARGET libevent
  LICENSE BSD-3-Clause
  SOURCE_DIR ${LIBEVENT_ROOT}
  SRC buffer.c
      evbuffer.c
      evdns.c
      evdns.h
      event-config.h
      event-internal.h
      event.c
      event.h
      event_tagging.c
      evhttp.h
      evrpc-internal.h
      evrpc.c
      evrpc.h
      evsignal.h
      evutil.c
      evutil.h
      http-internal.h
      http.c
      log.c
      log.h
      min_heap.h
      poll.c
      select.c
      signal.c
      strlcpy-internal.h
      strlcpy.c
  LINUX epoll.c
  DARWIN kqueue.c)

target_include_directories(
  libevent PUBLIC ${LIBEVENT_ROOT}/compat ${LIBEVENT_ROOT}
                  ${LIBEVENT_ROOT}/../..)
android_target_include_directories(libevent linux PUBLIC ${LIBEVENT_ROOT}/linux)
android_target_include_directories(libevent darwin PUBLIC ${LIBEVENT_ROOT}/mac)
target_compile_options(libevent PRIVATE -Wno-implicit-function-declaration
                                        -Wno-deprecated-non-prototype
                                        -Wno-int-conversion)
target_compile_definitions(libevent PRIVATE -DHAVE_CONFIG_H=1)
