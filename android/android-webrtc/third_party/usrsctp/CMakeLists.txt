android_add_library(
  TARGET usrsctp
  LICENSE
    BSD-2-Clause
    URL
    https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev
  REPO ${ANDROID_QEMU2_TOP_DIR}
  NOTICE REPO/android/android-webrtc/third_party/usrsctp/LICENSE
  SRC # cmake-format: sortable
      usrsctplib/usrsctplib/netinet/sctp_asconf.c
      usrsctplib/usrsctplib/netinet/sctp_auth.c
      usrsctplib/usrsctplib/netinet/sctp_bsd_addr.c
      usrsctplib/usrsctplib/netinet/sctp_callout.c
      usrsctplib/usrsctplib/netinet/sctp_cc_functions.c
      usrsctplib/usrsctplib/netinet/sctp_crc32.c
      usrsctplib/usrsctplib/netinet/sctp_indata.c
      usrsctplib/usrsctplib/netinet/sctp_input.c
      usrsctplib/usrsctplib/netinet/sctp_output.c
      usrsctplib/usrsctplib/netinet/sctp_pcb.c
      usrsctplib/usrsctplib/netinet/sctp_peeloff.c
      usrsctplib/usrsctplib/netinet/sctp_sha1.c
      usrsctplib/usrsctplib/netinet/sctp_ss_functions.c
      usrsctplib/usrsctplib/netinet/sctp_sysctl.c
      usrsctplib/usrsctplib/netinet/sctp_timer.c
      usrsctplib/usrsctplib/netinet/sctp_userspace.c
      usrsctplib/usrsctplib/netinet/sctp_usrreq.c
      usrsctplib/usrsctplib/netinet/sctputil.c
      usrsctplib/usrsctplib/netinet6/sctp6_usrreq.c
      usrsctplib/usrsctplib/user_environment.c
      usrsctplib/usrsctplib/user_mbuf.c
      usrsctplib/usrsctplib/user_recv_thread.c
      usrsctplib/usrsctplib/user_socket.c)

target_include_directories(usrsctp PUBLIC usrsctplib/usrsctplib)
target_link_libraries(usrsctp PUBLIC ssl)
target_compile_definitions(
  usrsctp PRIVATE SCTP_PROCESS_LEVEL_LOCKS SCTP_SIMPLE_ALLOCATOR
                  SCTP_USE_OPENSSL_SHA1 SCTP_USE_OPENSSL_RAND __Userspace__)
android_target_compile_definitions(
  usrsctp darwin-x86_64 PRIVATE __APPLE_USE_RFC_2292 HAVE_SCONN_LEN HAVE_SA_LEN
                                HAVE_SCONN_LEN NON_WINDOWS_DEFINE)
android_target_compile_definitions(
  usrsctp darwin-aarch64 PRIVATE __APPLE_USE_RFC_2292 HAVE_SCONN_LEN
                                 HAVE_SA_LEN HAVE_SCONN_LEN NON_WINDOWS_DEFINE)
android_target_compile_definitions(usrsctp linux-x86_64 PRIVATE _GNU_SOURCE
                                                                INET INET6)

android_target_compile_definitions(usrsctp linux-aarch64 PRIVATE _GNU_SOURCE
                                                                 INET INET6)
